// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <boost/iterator/iterator_concepts.hpp>
#include "leveldb/cache.h"
#include "port/port.h"
#include "util/hash.h"
#include "util/mutexlock.h"
#include "db/table_cache.h"
using namespace std;
bool multi_queue_init = true;
namespace leveldb
{

    namespace multiqueue_ns
    {
        struct LRUQueueCache;

        struct LRUQueueHandle
        {
            void *value;
            void (*deleter)(const Slice &, void *value);
            LRUQueueHandle *next_hash;
            LRUQueueHandle *next;
            LRUQueueHandle *prev;
            size_t charge;      // TODO(opt): Only allow uint32_t?
            size_t key_length;
            bool in_cache;      // Whether entry is in the cache.
            uint32_t refs;      // References, including cache reference, if present.
            uint32_t hash;      // Hash of key(); used for fast sharding and comparisons
            uint64_t fre_count;   //frequency count
            uint64_t expire_time; //expire_time = current_time_ + life_time_
            uint16_t queue_id;   // queue id  start from 0  and 0 means 0 filter
            bool type;      //"true” represents  tableandfile
            uint32_t value_id;
            uint32_t value_refs;
            bool is_split;
            LRUQueueHandle *table_handle, *prev_region, *next_region;

            char key_data[1];   // Beginning of key

            Slice key() const
            {
                // For cheaper lookups, we allow a temporary Handle object
                // to store a pointer to a key in "value".
                if (next == this)
                {
                    return *(reinterpret_cast<Slice *>(value));
                }
                else
                {
                    return Slice(key_data, key_length);
                }
            }
        };

        class HandleTable     // a list store LRUQueueHandle's address , don't care queue id
        {
        public:
            HandleTable() : length_(0), elems_(0), list_(NULL)
            {
                Resize();
            }
            ~HandleTable()
            {
                delete[] list_;
            }

            LRUQueueHandle *Lookup(const Slice &key, uint32_t hash)
            {
                return *FindPointer(key, hash);
            }

            LRUQueueHandle *Insert(LRUQueueHandle *h)
            {
                LRUQueueHandle **ptr = FindPointer(h->key(), h->hash);
                LRUQueueHandle *old = *ptr;
                h->next_hash = (old == NULL ? NULL : old->next_hash);
                *ptr = h;
                if (old == NULL)
                {
                    ++elems_;
                    if (elems_ > length_)
                    {
                        // Since each cache entry is fairly large, we aim for a small
                        // average linked list length (<= 1).
                        Resize();
                    }
                }
                return old;
            }

            LRUQueueHandle *Remove(const Slice &key, uint32_t hash)
            {
                LRUQueueHandle **ptr = FindPointer(key, hash);
                LRUQueueHandle *result = *ptr;
                if (result != NULL)
                {
                    *ptr = result->next_hash;
                    --elems_;
                }
                return result;
            }

        private:
            // The table consists of an array of buckets where each bucket is
            // a linked list of cache entries that hash into the bucket.
            uint32_t length_;
            uint32_t elems_;
            LRUQueueHandle **list_;

            // Return a pointer to slot that points to a cache entry that
            // matches key/hash.  If there is no such cache entry, return a
            // pointer to the trailing slot in the corresponding linked list.
            LRUQueueHandle **FindPointer(const Slice &key, uint32_t hash)
            {
                LRUQueueHandle **ptr = &list_[hash & (length_ - 1)];
                while (*ptr != NULL &&
                        ((*ptr)->hash != hash || key != (*ptr)->key()))
                {
                    ptr = &(*ptr)->next_hash;
                }
                return ptr;
            }

            void Resize()
            {
                uint32_t new_length = 131072;
                while (new_length < elems_)
                {
                    new_length *= 2;
                }
                LRUQueueHandle **new_list = new LRUQueueHandle*[new_length];
                memset(new_list, 0, sizeof(new_list[0]) * new_length);
                uint32_t count = 0;
                for (uint32_t i = 0; i < length_; i++)
                {
                    LRUQueueHandle *h = list_[i];
                    while (h != NULL)
                    {
                        LRUQueueHandle *next = h->next_hash;
                        uint32_t hash = h->hash;
                        LRUQueueHandle **ptr = &new_list[hash & (new_length - 1)];
                        h->next_hash = *ptr;
                        *ptr = h;
                        h = next;
                        count++;
                    }
                }
                assert(elems_ == count);
                delete[] list_;
                list_ = new_list;
                length_ = new_length;
            }
        };

        // class LRUQueueCache {
        //  public:
        //   LRUQueueCache();
        //   ~LRUQueueCache();

        //   // Separate from constructor so caller can easily make an array of LRUCache
        //   void SetCapacity(size_t capacity) { capacity_ = capacity; }

        //   // Like Cache methods, but with an extra "hash" parameter.
        //   Cache::Handle* Insert(const Slice& key, uint32_t hash,
        //                         void* value, size_t charge,
        //                         void (*deleter)(const Slice& key, void* value));
        //   Cache::Handle* Lookup(const Slice& key, uint32_t hash);
        //   void Release(Cache::Handle* handle);
        //   void Erase(const Slice& key, uint32_t hash);
        //   void Prune();
        //   size_t TotalCharge() const {
        //     MutexLock l(&mutex_);
        //     return usage_;
        //   }
        //   friend class LRUQueueCache;
        //  private:

        //   bool FinishErase(LRUQueueHandle* e);

        //   // Initialized before use.
        //   size_t capacity_;

        //   // mutex_ protects the following state.
        //   mutable port::Mutex mutex_;
        //   size_t usage_;

        //   // Dummy head of LRU list.
        //   // lru.prev is newest entry, lru.next is oldest entry.
        //   // Entries have refs==1 and in_cache==true.
        //   LRUQueueHandle lru_;

        //   // Dummy head of in-use list.
        //   // Entries are in use by clients, and have refs >= 2 and in_cache==true.


        // };

        // LRUQueueCache::LRUQueueCache()
        //     : usage_(0) {
        //   // Make empty circular linked lists.
        //   lru_.next = &lru_;
        //   lru_.prev = &lru_;
        // }

        // LRUQueueCache::~LRUQueueCache() { // TODO : deconstructor in MultiQueue
        //  // assert(in_use_.next == &in_use_);  // Error if caller has an unreleased handle //TODO: MultiQueue
        //   for (LRUQueueHandle* e = lru_.next; e != &lru_; ) {
        //     LRUQueueHandle* next = e->next;
        //     assert(e->in_cache);
        //     e->in_cache = false;
        //     assert(e->refs == 1);  // Invariant of lru_ list.
        //     Unref(e);
        //     e = next;
        //   }
        // }


        class MultiQueue: public Cache
        {
            int insert_count;
            int lrus_num_;
            size_t *charges_;
            std::atomic<uint64_t> last_id_;
            mutable leveldb::SpinMutex mutex_;  //for hashtable ,usage_ and e
            // Dummy head of in-use list.
            LRUQueueHandle in_use_;
            //Dummy heads of LRU list.
            LRUQueueHandle *lrus_;
            std::vector<size_t> lru_lens_;
            std::vector<size_t> sum_freqs_;
            atomic<size_t> sum_lru_len;
            HandleTable table_;
            size_t capacity_;
            std::atomic<size_t> usage_;
            std::atomic<bool> shrinking_;  //shrinking usage?
            uint64_t current_time_;
            uint64_t life_time_;
            int base_num_ ;
            std::atomic<bool> shutting_down_;
            double slow_shrink_ratio;
            double force_shrink_ratio;
            double change_ratio;
            double slow_ratio;
            double expection_;
            std::vector<double> fps;
            std::vector<size_t> bits_per_key_per_filter_, bits_per_key_per_filter_sum;  //begin from 0 bits
            const double log_base;
            bool need_adjust;
            uint64_t dynamic_merge_counter[2];
        public:
            MultiQueue(size_t capacity, int lrus_num = 1, int base_num = 64, uint64_t life_time = 50, double fr = 1.1, double sr = .95, double cr = 0.001, int lg_b = 3, double s_r = 0.5);
            ~MultiQueue();
            void Ref(LRUQueueHandle *e, bool addFreCount = false);
            void Unref(LRUQueueHandle *e) ;
            void LRU_Remove(LRUQueueHandle *e);
            void LRU_Append(LRUQueueHandle *list, LRUQueueHandle *e);
            Cache::Handle *Lookup(const Slice &key, uint32_t hash, bool Get);
            Cache::Handle *Lookup(const Slice &key);
            Cache::Handle *Lookup(const Slice &key, bool Get);
            uint64_t LookupFreCount(const Slice &key);
            void SetFreCount(const Slice &key, uint64_t freCount);
            int AllocFilterNums(int freq);

            void Release(Cache::Handle *handle);
            virtual Handle *Insert(const Slice &key, void *value, size_t charge,
                                   void (*deleter)(const Slice &key, void *value)) ;
            virtual Handle *Insert(const Slice &key, void *value, size_t charge,
                                   void (*deleter)(const Slice &key, void *value), bool type) ;

            virtual void *Value(Handle *handle);
            virtual void Erase(const Slice &key);
            virtual uint64_t NewId();
            virtual size_t TotalCharge() const;
            virtual uint64_t GetLRUFreCount() const;
            virtual bool IsCacheFull() const;
            virtual void TurnOnAdjustment();
            virtual void TurnOffAdjustment();
            //charge must >= 0
            Cache::Handle *Insert(const Slice &key, uint32_t hash, void *value, size_t charge,
                                  void (*deleter)(const Slice &key, void *value), bool type) ;

            bool FinishErase(LRUQueueHandle *e);
            void Erase(const Slice &key, uint32_t hash);
            void Prune() {} //do nothing
            bool ShrinkLRU(int k, int64_t remove_charge[], bool force = false);
            // int Queue_Num(uint64_t fre_count);
            uint64_t Num_Queue(int queue_id, uint64_t fre_count);
            std::string LRU_Status();
            void inline addCurrentTime()
            {
                ++current_time_;
            }
            static inline uint32_t HashSlice(const Slice &s)
            {
                return Hash(s.data(), s.size(), 0);
            }
            static void BGShrinkUsage(void *mq);
            void MayBeShrinkUsage();
            void BackgroudShrinkUsage();
            void SlowShrinking();
            void QuickShrinking();
            void ForceShrinking();
            void RecomputeExp(LRUQueueHandle *e);
            void RecomputeExpTable(LRUQueueHandle *e);
            double FalsePositive(LRUQueueHandle *e);
            static uint64_t base_fre_counts[10];
            static Env *mq_env;
        };

        MultiQueue::MultiQueue(size_t capacity, int lrus_num, int base_num, uint64_t life_time, double fr, double sr, double cr, int lg_b, double s_r): capacity_(capacity), lrus_num_(lrus_num), base_num_(base_num), life_time_(life_time), shrinking_(false)
            , force_shrink_ratio(fr), slow_shrink_ratio(sr), change_ratio(cr), sum_lru_len(0), log_base(log(lg_b)), slow_ratio(sr), expection_(0), usage_(0), shutting_down_(false), insert_count(0), need_adjust(true)
        {
            //TODO: declare outside  class  in_use and lrus parent must be Initialized,avoid lock crush
            uint64_t base_sum = lg_b;
            in_use_.next = &in_use_;
            in_use_.prev = &in_use_;
            in_use_.queue_id = lrus_num; //lrus_num = filter_num + 1
            lrus_ = new LRUQueueHandle[lrus_num];
            lru_lens_.resize(lrus_num);
            sum_freqs_.resize(lrus_num);
            for(int i = 0 ; i  < lrus_num ; ++i)
            {
                lrus_[i].next = &lrus_[i];
                lrus_[i].prev = &lrus_[i];
                lrus_[i].queue_id = i;
                lru_lens_[i] = 0;
                sum_freqs_[i] = 0;
                //base_fre_counts[i] = base_sum + 1 ;
                //base_sum = base_sum*lg_b;
            }
            //base_fre_counts[0] = base_num / 2;
            current_time_ = 0;
            cout << "Multi-Queue Capacity:" << capacity_ << endl;
            int sum_bits = 0;
            fps.push_back(1.0); // 0 filters
            bits_per_key_per_filter_.push_back(0);
            bits_per_key_per_filter_sum.push_back(0);
            for(int i = 1 ; i  < lrus_num ; ++i)
            {
                //cout<<"Base "<< i <<" fre count: "<<base_fre_counts[i]<<endl;
                sum_bits += FilterPolicy::bits_per_key_per_filter_[i - 1];
                fps.push_back( pow(0.6185, sum_bits) );
                bits_per_key_per_filter_.push_back(FilterPolicy::bits_per_key_per_filter_[i - 1]);
                bits_per_key_per_filter_sum.push_back(sum_bits);
            }

            dynamic_merge_counter[0] = dynamic_merge_counter[1] = 0;
        }

        std::string MultiQueue::LRU_Status()
        {
            mutex_.lock();
            int count = 0;
            char buf[300];
            std::string value;
            for(int i = 0 ; i < lrus_num_ ;  i++)
            {
                count = 0;
                for (LRUQueueHandle *e = lrus_[i].next; e != &lrus_[i]; )
                {
                    count++;
                    LRUQueueHandle *next = e->next;
                    e = next;
                }
                snprintf(buf, sizeof(buf), "lru %d count %d lru_lens_count:%lu \n", i, count, lru_lens_[i]);
                value.append(buf);
            }
            snprintf(buf, sizeof(buf), "lru insert_count %d\n", insert_count);
            mutex_.unlock();
            return value;
        }

        MultiQueue::~MultiQueue()
        {
            assert(in_use_.next == &in_use_);  // Error if caller has an unreleased handle
            shutting_down_ = true;
            mutex_.lock();
            fprintf(stderr, "optimized expection_ is %lf\n", expection_);
            for(int i = 0 ; i < lrus_num_ ;  i++)
            {
                for (LRUQueueHandle *e = lrus_[i].next; e != &lrus_[i]; )
                {
                    LRUQueueHandle *next = e->next;
                    assert(e->in_cache);
                    e->in_cache = false;
                    assert(e->refs == 1);  // Invariant of lru_ list.
                    Unref(e);
                    e = next;
                }
            }
            double avg_merge_region_nums = dynamic_merge_counter[0] == 0 ? 0 : dynamic_merge_counter[1] / dynamic_merge_counter[0];
            fprintf(stderr, "multi_queue_init is %s, expection_ is %lf, avg_merge_region_nums is %lf\n", multi_queue_init ? "true" : "false", expection_, avg_merge_region_nums);
            mutex_.unlock();
            delete []lrus_;
        }
        /*#define ln4 1.38629436
        #define ln3 1.09861229
        int MultiQueue::Queue_Num(uint64_t fre_count)
        {
            if(fre_count <= base_num_){
          return 0;
            }
            return std::max(1,std::min(lrus_num_-1,static_cast<int>(log(fre_count)/log_base) - 1));
        }
        */

        inline uint64_t MultiQueue::Num_Queue(int queue_id, uint64_t fre_count)
        {
            //mutex_.assertHeld();
            if(&lrus_[queue_id] == lrus_[queue_id].next)
            {
                return fre_count >> 1;
            }
            else
            {
                LRUQueueHandle *lru_handle = lrus_[queue_id].next;
                LRUQueueHandle *mru_handle = lrus_[queue_id].prev;
                return (lru_handle->fre_count + mru_handle->fre_count) / 2;
            }
        }

        inline double MultiQueue::FalsePositive(LRUQueueHandle *e)
        {
            leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(e->value);
            return fps[tf->table->getCurrFilterNum(e->value_id - 1)];
        }

        void MultiQueue::RecomputeExpTable(LRUQueueHandle *e)
        {
            if(multi_queue_init || (e->queue_id + 1) == lrus_num_)
            {
                ++e->fre_count;
                sum_freqs_[e->queue_id]++;
                expection_ += FalsePositive(e);
            }
            else
            {
                if (!need_adjust) {
                    ++e->fre_count;
                    sum_freqs_[e->queue_id]++;
                    expection_ += FalsePositive(e);
                    return;
                }
                uint64_t start_micros = Env::Default()->NowMicros();

                ++e->fre_count;
                double now_expection  = expection_ + FalsePositive(e) ;
                double min_expection = now_expection, change_expection;
                double new_expection = expection_ - (e->fre_count - 1) * FalsePositive(e) + e->fre_count * fps[e->queue_id + 1]; //TODO: OPTIMIZE
                int need_bits = usage_ - capacity_;// + bits_per_key_per_filter_[e->queue_id + 1];
                if (e->queue_id == 0) {
                    need_bits += lrus_[1].next->charge;
                }
                else {
                    need_bits += e->charge / bits_per_key_per_filter_sum[e->queue_id] * bits_per_key_per_filter_[e->queue_id + 1];
                }
double log_diff_const = 0.8;
                LRUQueueHandle *forward = e, *backward = e;
                while (forward != NULL && forward->next_region != NULL) {
                    forward = forward->next_region;
                    if (forward->queue_id != e->queue_id || forward->fre_count == 0 || forward->expire_time < current_time_) {
                        forward = forward->prev_region;
                        break;
                    }
            leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(forward->value);
            assert(forward->queue_id == tf->table->getCurrFilterNum(forward->value_id - 1));
                    double log_diff = log10(e->fre_count) - log10(forward->fre_count);
                    if (log_diff > log_diff_const || log_diff < -log_diff_const) {
                        forward = forward->prev_region;
                        break;
                    }

                    new_expection = new_expection - (forward->fre_count - 1) * FalsePositive(forward) + forward->fre_count * fps[forward->queue_id + 1];
                    if (forward->queue_id == 0) {
                        need_bits += lrus_[1].next->charge;
                    }
                    else {
                        need_bits += forward->charge / bits_per_key_per_filter_sum[forward->queue_id] * bits_per_key_per_filter_[forward->queue_id + 1];
                    }
                }
                while (backward != NULL && backward->prev_region != NULL) {
                    backward = backward->prev_region;
                    if (backward->queue_id != e->queue_id || backward->fre_count == 0 || backward->expire_time < current_time_) {
                        backward = backward->next_region;
                        break;
                    }
            leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(backward->value);
            assert(backward->queue_id == tf->table->getCurrFilterNum(backward->value_id - 1));
                    double log_diff = log10(e->fre_count) - log10(backward->fre_count);
                    if (log_diff > log_diff_const || log_diff < -log_diff_const) {
                        backward = backward->next_region;
                        break;
                    }

                    new_expection = new_expection - (backward->fre_count - 1) * FalsePositive(backward) + backward->fre_count * fps[backward->queue_id + 1];
                    if (backward->queue_id == 0) {
                        need_bits += lrus_[1].next->charge;
                    }
                    else {
                        need_bits += backward->charge / bits_per_key_per_filter_sum[backward->queue_id] * bits_per_key_per_filter_[backward->queue_id + 1];
                    }
                }

                int remove_bits, min_i = -1 ;

                if(need_bits > 0)
                {
                    for(int i = 1 ; i < lrus_num_ ; i++)
                    {
                        remove_bits = 0;
                        change_expection =  new_expection;
                        LRUQueueHandle *old = lrus_[i].next;
                        while(old != &lrus_[i] && remove_bits < need_bits)
                        {
                            if(old->expire_time < current_time_ )  // expired
                            {
                                // remove_bits += bits_per_key_per_filter_[i];
                                remove_bits += old->charge / bits_per_key_per_filter_sum[old->queue_id] * bits_per_key_per_filter_[i];
                                change_expection += (old->fre_count * fps[i - 1] - old->fre_count * FalsePositive(old));
                            }
                            else
                            {
                                break;
                            }
                            old = old->next;
                        }
                        if(remove_bits >= need_bits && change_expection < min_expection)
                        {
                            min_expection = change_expection;
                            min_i = i;
                        }
                    }
                    // if(min_i != -1 && now_expection - min_expection > now_expection * change_ratio * (forward->value_id - backward->value_id + 1)) //todo
                    if(min_i != -1 && now_expection - min_expection > now_expection * change_ratio) //todo
                    {
                        assert(now_expection > min_expection);
                        remove_bits = 0;
                        while(lrus_[min_i].next != &lrus_[min_i] && remove_bits < need_bits)
                        {
                            LRUQueueHandle *old = lrus_[min_i].next;
                            leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(old->value);
                            // remove_bits += bits_per_key_per_filter_[min_i];
                            remove_bits += old->charge / bits_per_key_per_filter_sum[old->queue_id] * bits_per_key_per_filter_[min_i];
                            size_t delta_charge = tf->table->RemoveFilters(1, old->value_id - 1);
                            usage_ -= delta_charge;
                            MeasureTime(Statistics::GetStatistics().get(), Tickers::REMOVE_EXPIRED_FILTER_TIME_0 + min_i, Env::Default()->NowMicros() - start_micros);
                            --lru_lens_[min_i];
                            sum_freqs_[min_i] -= old->fre_count;

                            LRU_Remove(old);
                            ++lru_lens_[min_i - 1];
                            sum_freqs_[min_i - 1] += old->fre_count;
                            LRU_Append(&lrus_[min_i - 1], old);
                        }

                        leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(e->value);
                        size_t* delta_charge = tf->table->AdjustFilters(e->queue_id + 1, backward->value_id - 1, forward->value_id - 1);

                        LRUQueueHandle *p = backward;
                        int i = 0;
                        while (p != forward) {
                            sum_freqs_[p->queue_id] -= p->fre_count;
                            sum_freqs_[p->queue_id + 1] += p->fre_count;
                            if (p->refs == 1) {
                                assert(p->in_cache);
                                LRU_Remove(p);
                                --lru_lens_[p->queue_id];
                                ++lru_lens_[p->queue_id + 1];                                
                                LRU_Append(&lrus_[p->queue_id + 1], p);
                            }
                            else
                                ++p->queue_id;
                            p->charge += delta_charge[i];
                            usage_ += delta_charge[i];
                            p = p->next_region;
                            i++;
                        }
                        sum_freqs_[p->queue_id] -= p->fre_count;
                        sum_freqs_[p->queue_id + 1] += p->fre_count;
                        if (p->refs == 1) {
                            assert(p->in_cache);
                            LRU_Remove(p);
                            --lru_lens_[p->queue_id];
                            ++lru_lens_[p->queue_id + 1];                                
                            LRU_Append(&lrus_[p->queue_id + 1], p);
                        }
                        else
                            ++p->queue_id;
                        p->charge += delta_charge[i];
                        usage_ += delta_charge[i];

                        dynamic_merge_counter[0]++;
                        dynamic_merge_counter[1] += forward->value_id - backward->value_id + 1;

                        expection_ = min_expection;
                    }
                    else
                    {
                        expection_ = now_expection;
                    }
                }
                else
                {
                    // if(now_expection - new_expection > now_expection * change_ratio * (forward->value_id - backward->value_id + 1))
                    if(now_expection - new_expection > now_expection * change_ratio)
                    {
                        leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(e->value);
                        size_t* delta_charge = tf->table->AdjustFilters(e->queue_id + 1, backward->value_id - 1, forward->value_id - 1);

                        LRUQueueHandle *p = backward;
                        int i = 0;
                        while (p != forward) {
                            sum_freqs_[p->queue_id] -= p->fre_count;
                            sum_freqs_[p->queue_id + 1] += p->fre_count;
                            if (p->refs == 1) {
                                assert(p->in_cache);
                                LRU_Remove(p);
                                --lru_lens_[p->queue_id];
                                sum_freqs_[p->queue_id] -= p->fre_count;
                                sum_freqs_[p->queue_id + 1] += p->fre_count;
                                ++lru_lens_[p->queue_id + 1];                                
                                LRU_Append(&lrus_[p->queue_id + 1], p);
                            }
                            else
                                ++p->queue_id;
                            p->charge += delta_charge[i];
                            usage_ += delta_charge[i];
                            p = p->next_region;
                            i++;
                        }
                        sum_freqs_[p->queue_id] -= p->fre_count;
                        sum_freqs_[p->queue_id + 1] += p->fre_count;
                        if (p->refs == 1) {
                            assert(p->in_cache);
                            LRU_Remove(p);
                            --lru_lens_[p->queue_id];
                            ++lru_lens_[p->queue_id + 1];                                
                            LRU_Append(&lrus_[p->queue_id + 1], p);
                        }
                        else
                            ++p->queue_id;
                        p->charge += delta_charge[i];
                        usage_ += delta_charge[i];

                        dynamic_merge_counter[0]++;
                        dynamic_merge_counter[1] += forward->value_id - backward->value_id + 1;

                        expection_ = new_expection;
                    }
                    else
                    {
                        expection_ = now_expection;
                    }
                }
            }
        }

        void MultiQueue::RecomputeExp(LRUQueueHandle *e)
        {
            if(multi_queue_init || (e->queue_id + 1) == lrus_num_)
            {
                ++e->fre_count;
                sum_freqs_[e->queue_id]++;
                expection_ += FalsePositive(e);
            }
            else
            {
                uint64_t start_micros = Env::Default()->NowMicros();
                double now_expection  = expection_ + FalsePositive(e) ;
                ++e->fre_count;
                double min_expection = now_expection, change_expection;
                const double new_expection = expection_ - (e->fre_count - 1) * FalsePositive(e) + e->fre_count * fps[e->queue_id + 1]; //TODO: OPTIMIZE
                int need_bits = usage_ - capacity_;// + bits_per_key_per_filter_[e->queue_id + 1];
                if (e->queue_id == 0) {
                    need_bits += lrus_[1].next->charge;
                }
                else {
                    need_bits += e->charge / bits_per_key_per_filter_sum[e->queue_id] * bits_per_key_per_filter_[e->queue_id + 1];
                }
                int remove_bits, min_i = -1 ;
                if(need_bits > 0)
                {
                    for(int i = 1 ; i < lrus_num_ ; i++)
                    {
                        remove_bits = 0;
                        change_expection =  new_expection;
                        LRUQueueHandle *old = lrus_[i].next;
                        while(old != &lrus_[i] && remove_bits < need_bits)
                        {
                            if(old->expire_time < current_time_ )  // expired
                            {
                                // remove_bits += bits_per_key_per_filter_[i];
                                remove_bits += old->charge / bits_per_key_per_filter_sum[old->queue_id] * bits_per_key_per_filter_[i];
                                change_expection += (old->fre_count * fps[i - 1] - old->fre_count * FalsePositive(old));
                            }
                            else
                            {
                                break;
                            }
                            old = old->next;
                        }
                        if(remove_bits >= need_bits && change_expection < min_expection)
                        {
                            min_expection = change_expection;
                            min_i = i;
                        }
                    }
                    if(min_i != -1 && now_expection - min_expection > now_expection * change_ratio)
                    {
                        assert(now_expection > min_expection);
                        remove_bits = 0;
                        while(lrus_[min_i].next != &lrus_[min_i] && remove_bits < need_bits)
                        {
                            LRUQueueHandle *old = lrus_[min_i].next;
                            leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(old->value);
                            // remove_bits += bits_per_key_per_filter_[min_i];
                            remove_bits += old->charge / bits_per_key_per_filter_sum[old->queue_id] * bits_per_key_per_filter_[min_i];
                            size_t delta_charge = tf->table->RemoveFilters(1, old->value_id - 1);
                            usage_ -= delta_charge;
                            MeasureTime(Statistics::GetStatistics().get(), Tickers::REMOVE_EXPIRED_FILTER_TIME_0 + min_i, Env::Default()->NowMicros() - start_micros);
                            --lru_lens_[min_i];
                            sum_freqs_[min_i] -= old->fre_count;

                            LRU_Remove(old);
                            ++lru_lens_[min_i - 1];
                            sum_freqs_[min_i - 1] += old->fre_count;
                            LRU_Append(&lrus_[min_i - 1], old);
                        }
                        sum_freqs_[e->queue_id] -= e->fre_count;
                        sum_freqs_[e->queue_id + 1] += e->fre_count;
                        ++e->queue_id;
                        expection_ = min_expection;
                    }
                    else
                    {
                        expection_ = now_expection;
                    }
                }
                else
                {
                    if(now_expection - new_expection > now_expection * change_ratio)
                    {
                        //if(now_expection > new_expection){
                        sum_freqs_[e->queue_id] -= e->fre_count;
                        sum_freqs_[e->queue_id + 1] += e->fre_count;
                        ++e->queue_id;
                        expection_ = new_expection;
                    }
                    else
                    {
                        expection_ = now_expection;
                    }
                }
            }
        }

        void MultiQueue::Ref(LRUQueueHandle *e, bool addFreCount)
        {
            //mutex_.assert_held();
            if (e->refs == 1 && e->in_cache)    // If on lru_ list, move to in_use_ list.
            {
                LRU_Remove(e);
                LRU_Append(&in_use_, e);
                --lru_lens_[e->queue_id];
                --sum_lru_len;
            }
            e->refs++;
            if(addFreCount)
            {
                leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(e->value);
                tf->table->setAccess(e->value_id - 1);
                if(e->expire_time > current_time_ )  //not expired
                {
                    RecomputeExp(e);
                    // RecomputeExpTable(e);
                }// else{ // if(e->expire_time < current_time_){   //expired
                //   if(e->fre_count > 0){
                //  expection_ -= (e->fre_count*1.0/2.0)*FalsePositive(e);
                //  e->fre_count /= 2;
                //   }
                // }
            }
            e->expire_time = current_time_ + life_time_;
        }

        void MultiQueue::Unref(LRUQueueHandle *e)
        {
            //mutex_.assert_held();
            assert(e->refs > 0);
            e->refs--;
            if (e->refs == 0)   // Deallocate.
            {
deallocate:
                assert(!e->in_cache);
                expection_ -= e->fre_count * fps[e->queue_id];

                leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(e->value);

                if (e->value_id == 0) {
                    //todo
                    Slice key(e->key_data, sizeof(uint64_t));
                    (*e->deleter)(key, e->value);
                }
                free(e);
            }
            else if (e->in_cache && e->refs == 1)      // note:No longer in use; move to lru_ list.
            {
                //int qn = Queue_Num(e->fre_count);
                leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(e->value);
                if(e->value_id > 0 && tf->table->getCurrFilterNum(e->value_id - 1) < e->queue_id && e->type)   //only add;
                {
                    ++e->refs;
                    // mutex_.unlock();
                    size_t delta_charge = tf->table->AdjustFilters(e->queue_id, e->value_id - 1);  // not in lru list, so need to worry will be catched by ShrinkUsage
                    // mutex_.lock();
                    --e->refs;
                    if(e->refs == 0)
                    {
                        goto deallocate;
                    }
                    e->charge += delta_charge;
                    usage_ += delta_charge;
                    MayBeShrinkUsage();
                }
                else if (e->value_id > 0 && tf->table->getCurrFilterNum(e->value_id - 1) > e->queue_id && e->type) {
                    e->queue_id = tf->table->getCurrFilterNum(e->value_id - 1);
                #ifdef USE_REAL_SIZE
                    size_t delta_charge = tf->table->getCurrFiltersSize(e->value_id - 1);
                #else
                    size_t delta_charge = bits_per_key_per_filter_sum[e->queue_id];
                #endif
                    e->charge = delta_charge;
                    usage_ += delta_charge;
                    MayBeShrinkUsage();
                }
                LRU_Remove(e);
                LRU_Append(&lrus_[e->queue_id], e);
                ++lru_lens_[e->queue_id];
                ++sum_lru_len;
            }
            mutex_.unlock();

        }

        void MultiQueue::LRU_Remove(LRUQueueHandle *e)
        {
            e->next->prev = e->prev;
            e->prev->next = e->next;
        }

        void MultiQueue::LRU_Append(LRUQueueHandle *list, LRUQueueHandle *e)
        {
            // Make "e" newest entry by inserting just before *list
            e->next = list;
            e->prev = list->prev;
            e->prev->next = e;
            e->next->prev = e;
            //if append to in_use , no need to remember queue_id,thus in_use_mutex used independently
            e->queue_id = list->queue_id == lrus_num_ ? e->queue_id : list->queue_id;
        }

        Cache::Handle *MultiQueue::Lookup(const Slice &key, uint32_t hash, bool Get)
        {
            mutex_.lock();
            LRUQueueHandle *e = table_.Lookup(key, hash);
            if (e != NULL)
            {
                if(e->in_cache && e->refs == 1)
                {
                    Ref(e, Get);
                }
                else
                {
                    Ref(e, Get); //on in-use list or not in cache in the short time
                }
            }
            mutex_.unlock();
            return reinterpret_cast<Cache::Handle *>(e);
        }

        uint64_t MultiQueue::LookupFreCount(const Slice &key)
        {
            const uint32_t hash = HashSlice(key);
            mutex_.lock();
            LRUQueueHandle *e = table_.Lookup(key, hash);
            if (e != NULL)
            {
                mutex_.unlock();
                return e->fre_count;
            }
            mutex_.unlock();
            return 0;
        }

        void MultiQueue::SetFreCount(const Slice &key, uint64_t freCount)
        {
            const uint32_t hash = HashSlice(key);
            mutex_.lock();
            LRUQueueHandle *e = table_.Lookup(key, hash);
            if (e != NULL)
            {
                expection_ -= e->fre_count * FalsePositive(e);
                e->fre_count = freCount;
                expection_ += e->fre_count * FalsePositive(e);
            }
            mutex_.unlock();

        }

        Cache::Handle *MultiQueue::Lookup(const Slice &key, bool Get)
        {
            const uint32_t hash = HashSlice(key);
            return  Lookup(key, hash, Get);
        }

        Cache::Handle *MultiQueue::Lookup(const Slice &key)
        {
            const uint32_t hash = HashSlice(key);
            return  Lookup(key, hash, false);
        }
        void MultiQueue::Release(Cache::Handle *handle)
        {
            auto lru_queue_handle = reinterpret_cast<LRUQueueHandle *>(handle);
            mutex_.lock();
            Unref(lru_queue_handle);
        }

        void MultiQueue::MayBeShrinkUsage()
        {
            //mutex_.assertheld
            // if(usage_ >= 2 * capacity_ / 3)
            //     multi_queue_init = false;

            if(usage_ >= capacity_)
            {
                multi_queue_init = false;
                int64_t overflow_charge = usage_ - capacity_;
                if(!ShrinkLRU(lrus_num_ - 1, &overflow_charge, false))
                {
                    overflow_charge = usage_ - capacity_;
                    ShrinkLRU(1, &overflow_charge, true);
                }
            }
        }

        void MultiQueue::BGShrinkUsage(void *mq)
        {
            reinterpret_cast<MultiQueue *>(mq)->BackgroudShrinkUsage();
        }

        inline bool MultiQueue::ShrinkLRU(int k, int64_t remove_charge[], bool force)
        {
            //mutex_.assertHeld
            int64_t removed_usage = 0;
            if(!force)
            {
                while (usage_ > capacity_ && k >= 1)
                {
                    while(lrus_[k].next != &lrus_[k]  && removed_usage < remove_charge[0])
                    {
                        LRUQueueHandle *old = lrus_[k].next;
                        // assert(old->refs >= 1);
                        //TODO: old->fre_count = queue_id
                        if(old->refs == 1 && old->expire_time < current_time_)
                        {
                            leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(old->value);
                            uint64_t start_micros = Env::Default()->NowMicros();
                            size_t delta_charge = tf->table->RemoveFilters(1, old->value_id - 1);
                            MeasureTime(Statistics::GetStatistics().get(), Tickers::REMOVE_EXPIRED_FILTER_TIME_0 + k, Env::Default()->NowMicros() - start_micros);
                            old->charge -= delta_charge;
                            usage_ -= delta_charge;
                            removed_usage += delta_charge;
                            expection_ -= old->fre_count * fps[k];
                            old->fre_count = Num_Queue(k - 1, old->fre_count); // also decrease fre count
                            expection_ += old->fre_count * fps[k - 1];
                            --lru_lens_[k];
                            sum_freqs_[k] -= old->fre_count;
                            LRU_Remove(old);
                            ++lru_lens_[k - 1];
                            sum_freqs_[k - 1] += old->fre_count;
                            LRU_Append(&lrus_[k - 1], old);
                        }
                        else
                        {
                            break;
                        }
                    }
                    k--;
                }
                if(removed_usage >= remove_charge[0])
                {
                    return true;
                }
                return false;
            }
            else
            {
                size_t max_lru_lens = 0, max_i = -1;
                for(int i = 1 ; i < lrus_num_ ; i++)
                {
                    if(lru_lens_[i] > max_lru_lens)
                    {
                        max_lru_lens = lru_lens_[i];
                        max_i = i;
                    }
                }
                if(max_i != -1)
                {
                    k = max_i;
                    while(removed_usage < remove_charge[0])
                    {
                        uint64_t start_micros = Env::Default()->NowMicros();
                        LRUQueueHandle *old = lrus_[k].next;
                        // assert(old != &lrus_[k]);
                        if (old == &lrus_[k]) {
                            int i = 1;
                            while (i) {
                                int b = 0;
                                b += 1;
                            }
                        }
                        leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(old->value);
                        size_t delta_charge = tf->table->RemoveFilters(1, old->value_id - 1); //  remove 1 filter
                        old->charge -= delta_charge;
                        usage_ -= delta_charge;
                        removed_usage += delta_charge;
                        expection_ -= old->fre_count * fps[k];
                        old->fre_count = Num_Queue(k - 1, old->fre_count); // also decrease fre count
                        expection_ += old->fre_count * fps[k - 1];
                        --lru_lens_[k];
                        sum_freqs_[k] -= old->fre_count;
                        LRU_Remove(old);
                        LRU_Append(&lrus_[k - 1], old);
                        ++lru_lens_[k - 1];
                        sum_freqs_[k - 1] += old->fre_count;
                        MeasureTime(Statistics::GetStatistics().get(), Tickers::REMOVE_HEAD_FILTER_TIME_0 + k, Env::Default()->NowMicros() - start_micros);
                    }
                }
                return true;
            }
        }

        inline uint64_t MultiQueue::GetLRUFreCount() const
        {
            int i = 1;
            while(i < lrus_num_)
            {
                if(lrus_[i].next != &lrus_[i])
                {
                    return lrus_[i].next->fre_count;
                }
                ++i;
            }

        }

        int MultiQueue::AllocFilterNums(int freq) {
            int i = 1;
            int last_ = 1;
            while(i < lrus_num_)
            {
                if(lrus_[i].next != &lrus_[i])
                {
                    if (freq < sum_freqs_[i] / lru_lens_[i])
                        return i;
                    last_ = i;
                }
                ++i;
            }
            // return last_ + 1 < lrus_num_ ? last_ + 1 : last_;
            return last_;
        }

        inline bool MultiQueue::IsCacheFull() const
        {
            return usage_ >= capacity_;
        }

        void MultiQueue::TurnOnAdjustment() {
            mutex_.lock();
            need_adjust = true;
            mutex_.unlock();
        }

        void MultiQueue::TurnOffAdjustment() {
            mutex_.lock();
            need_adjust = false;
            mutex_.unlock();  
        }

        void MultiQueue::SlowShrinking()
        {
            int64_t remove_charge = (usage_ - capacity_ * slow_shrink_ratio);
            const int interval = 8;
            if(remove_charge <= 0)
            {
                return ;
            }
            remove_charge = (remove_charge + 7) / 8;
            for(int i = 0 ; i < interval  && usage_ > capacity_ * slow_shrink_ratio ; i++)
            {
                mutex_.lock();
                if(!ShrinkLRU(0, &remove_charge))
                {
                    mutex_.unlock();
                    break;
                }
                mutex_.unlock();
                remove_charge = remove_charge * 1.05;
                //   for (uint32_t tries = 0; tries < 200; ++tries);
            }

        }
        /*
        void MultiQueue::QuickShrinking()
        {
          int64_t remove_charges[8];
          int64_t overflow_charge = usage_ - capacity_*quick_shrink_ratio;
          int interval = 8;
          if(overflow_charge < 0){
            return;
          }
          bool shrinkLRU0_flag = true;
          for(int i = 0 ; i < lrus_num_ ; i++){
              remove_charges[i] = overflow_charge * (lru_lens_[i]*1.0/sum_lru_len);
          }
          for(int i = 0 ; i < 1  && usage_ > capacity_*quick_shrink_ratio ; i++){
              mutex_.lock();
              if(shrinkLRU0_flag){
            shrinkLRU0_flag = ShrinkLRU(0,&overflow_charge);
              }
              //      ShrinkLRU(1,remove_charges);
              mutex_.unlock();
              if(shrinkLRU0_flag){
            remove_charges[0] = remove_charges[0]*1.05;
              }
           //   for (uint32_t tries = 0; tries < 100; ++tries);
          }
        }*/

        void MultiQueue::ForceShrinking()
        {
            int64_t remove_charges[8];
            int64_t overflow_charge = usage_ - capacity_;
            for(int i = 0 ; i < lrus_num_ ; i++)
            {
                remove_charges[i] = overflow_charge * (lru_lens_[i] * 1.0 / sum_lru_len);
            }
            while(usage_ > capacity_ * force_shrink_ratio)
            {
                mutex_.lock();
                ShrinkLRU(0, remove_charges, true);
                ShrinkLRU(1, remove_charges, true);
                mutex_.unlock();
            }
        }

        void MultiQueue::BackgroudShrinkUsage()
        {
            if(shutting_down_ || usage_ < capacity_ * slow_shrink_ratio)
            {
                return;
            }
            uint64_t start_micros = Env::Default()->NowMicros();
            if(usage_ > capacity_ * force_shrink_ratio)
            {
                ForceShrinking();
                MeasureTime(Statistics::GetStatistics().get(), Tickers::FORCE_SHRINKING, Env::Default()->NowMicros() - start_micros);
            }
            else
            {
                SlowShrinking();  //also call SlowShrinking
                //QuickShrinking();
                MeasureTime(Statistics::GetStatistics().get(), Tickers::QUICK_SHRINKING, Env::Default()->NowMicros() - start_micros);
            }
            shrinking_ = false;
            //    multi_queue_shrinking = false;
            mutex_.lock();
            MayBeShrinkUsage();
            mutex_.unlock();
        }

        Cache::Handle *MultiQueue::Insert(const Slice &key, void *value, size_t charge, void (*deleter)(const Slice &key, void *value))
        {
            const uint32_t hash = HashSlice(key);
            return Insert(key, hash, value, charge, deleter, false);
        }

        Cache::Handle *MultiQueue::Insert(const Slice &key, void *value, size_t charge, void (*deleter)(const Slice &key, void *value), bool type)
        {
            const uint32_t hash = HashSlice(key);
            return Insert(key, hash, value, charge, deleter, type);
        }


        Cache::Handle *MultiQueue::Insert(const Slice &key, uint32_t hash, void *value, size_t charge, void (*deleter)(const Slice &key, void *value), bool type)
        {
            // MutexLock l(&mutex_);
            mutex_.lock();
            insert_count++;

            // leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(value);
            // int regionNum = tf->table->getRegionNum();

            LRUQueueHandle *e = reinterpret_cast<LRUQueueHandle *>(
                                    malloc(sizeof(LRUQueueHandle) - 1 + key.size()));
            e->value = value;
            e->deleter = deleter;
            e->charge = charge;
            e->key_length = key.size();
            e->hash = hash;
            e->in_cache = false;
            e->refs = 1;  // for the returned handle.
            e->type = type;
            e->fre_count = 0;
            leveldb::TableAndFile *tf = reinterpret_cast<leveldb::TableAndFile *>(e->value);
            uint32_t *regionId_ = (uint32_t *) (key.data() + sizeof(uint64_t));
            e->value_id = *regionId_;
            e->queue_id = tf->table->getCurrFilterNum(e->value_id - 1) ;   //

            e->expire_time = current_time_ + life_time_;
            memcpy(e->key_data, key.data(), key.size());

            if (e->value_id == 0)
            {
                e->queue_id = 0;
                e->refs++;
                e->value_refs = 0;
                e->in_cache = true;
                e->table_handle = e;
                // mutex_.lock();
                LRU_Append(&in_use_, e);
                auto redun_handle = table_.Insert(e);
                if(redun_handle != NULL)
                {
                    FinishErase(redun_handle);
                }
                // mutex_.unlock();

            }
            else if (capacity_ > 0 && e->value_id > 0)
            {
                char buf[sizeof(uint64_t) + sizeof(uint32_t)];
                memcpy(buf, e->key_data, sizeof(uint64_t));
                regionId_ = (uint32_t *) (buf + sizeof(uint64_t));
                *regionId_ = 0;
                Slice key_(buf, sizeof(buf));
                uint32_t hash_ = HashSlice(key_);
                LRUQueueHandle *table_handle = table_.Lookup(key_, hash_);
                assert(table_handle);
                table_handle->value_refs++;
                e->table_handle = table_handle;
                if (e->value_id == 1)
                    e->prev_region = NULL;
                else {
                    *regionId_ = e->value_id - 1;
                    Slice key2(buf, sizeof(buf));
                    hash_ = HashSlice(key2);
                    LRUQueueHandle *prev_region = table_.Lookup(key2, hash_);
                    e->prev_region = prev_region;
                    prev_region->next_region = e;
                }
                e->next_region = NULL;

                // e->refs++;  // for the cache's reference.
                // ++e->fre_count; //for the first access
                // expection_ += fps[e->queue_id];     //insert a new element ,expected number should be updated
                e->in_cache = true;
                // mutex_.lock();
                LRU_Append(&lrus_[e->queue_id], e);
                ++lru_lens_[e->queue_id];
                ++sum_lru_len;

                usage_ += charge;
                auto redun_handle = table_.Insert(e);
                if(redun_handle != NULL)
                {
                    FinishErase(redun_handle);
                }
                // mutex_.unlock();

            } // else don't cache.  (Tests use capacity_==0 to turn off caching.)
                
            // mutex_.lock();
            MayBeShrinkUsage();
            mutex_.unlock();
            return reinterpret_cast<Cache::Handle *>(e);
        }

        bool MultiQueue::FinishErase(LRUQueueHandle *e)
        {
            if (e != NULL)
            {
                assert(e->in_cache);
                LRU_Remove(e);
                if(e->refs == 1)     //means remove from LRU
                {
                    --lru_lens_[e->queue_id];
                    sum_freqs_[e->queue_id] -= e->fre_count;
                    --sum_lru_len;
                }
                e->in_cache = false;
                usage_ -= e->charge;
                Unref(e);
            }
            return e != NULL;
        }
//todo
        void MultiQueue::Erase(const Slice &key, uint32_t hash)
        {
            mutex_.lock();
            auto obsolete_handle = table_.Remove(key, hash);
            if(obsolete_handle != NULL)
            {
                FinishErase(obsolete_handle);
            }
            mutex_.unlock();

        }

        void MultiQueue::Erase(const Slice &key)
        {
            const uint32_t hash = HashSlice(key);
            Erase(key, hash);
        }

        uint64_t MultiQueue::NewId()
        {
            return ++(last_id_);
        }

        size_t MultiQueue::TotalCharge() const
        {
            return usage_;
        }

        void *MultiQueue::Value(Cache::Handle *handle)
        {
            return reinterpret_cast<LRUQueueHandle *>(handle)->value;
        }



    };
    Env *multiqueue_ns::MultiQueue::mq_env;
    uint64_t multiqueue_ns::MultiQueue::base_fre_counts[10] = {4, 10, 28, 82, 243, 730};


    Cache *NewMultiQueue(size_t capacity, int lrus_num, int base_num, uint64_t life_time, double force_shrink_ratio, double slow_shrink_ratio, double change_ratio, int lg_b, double s_r)
    {
        multiqueue_ns::MultiQueue::mq_env = newEnv();
        return new multiqueue_ns::MultiQueue(capacity, lrus_num, base_num, life_time, force_shrink_ratio, slow_shrink_ratio, change_ratio, lg_b, s_r);
    }

};

