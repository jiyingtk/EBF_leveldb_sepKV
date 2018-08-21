// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Thread-safe (provides internal synchronization)

#ifndef STORAGE_LEVELDB_DB_TABLE_CACHE_H_
#define STORAGE_LEVELDB_DB_TABLE_CACHE_H_

#include <string>
#include <stdint.h>
#include "db/dbformat.h"
#include "leveldb/cache.h"
#include "leveldb/table.h"
#include "port/port.h"
#include "leveldb/env.h"
#include "leveldb/table.h"
namespace leveldb {

struct TableAndFile {
  RandomAccessFile* file;
  Table* table;
  uint32_t refs;
};
class Env;
class VersionSet;
class Version;
class TableCache {
 public:
  TableCache(const std::string& dbname, const Options* options, size_t entries);
  ~TableCache();

  // Return an iterator for the specified file number (the corresponding
  // file length must be exactly "file_size" bytes).  If "tableptr" is
  // non-NULL, also sets "*tableptr" to point to the Table object
  // underlying the returned iterator, or NULL if no Table object underlies
  // the returned iterator.  The returned "*tableptr" object is owned by
  // the cache and should not be deleted, and is valid for as long as the
  // returned iterator is live.
  Iterator* NewIterator(const ReadOptions& options,
                        uint64_t file_number,
                        uint64_t file_size,
                        Table** tableptr = NULL, TableMetaData *tableMetaData_ = NULL, std::vector<uint64_t> *input_0_numbers = NULL, std::vector<uint64_t> *input_1_numbers = NULL);
   
 Iterator* NewBufferedIterator(const ReadOptions& options,
                        uint64_t file_number,
                        uint64_t file_size,
                        Table** tableptr = NULL);

  // If a seek to internal key "k" in specified file finds an entry,
  // call (*handle_result)(arg, found_key, found_value).
  Status Get(const ReadOptions& options,
             uint64_t file_number,
             uint64_t file_size,
             const Slice& k,
             void* arg,
             void (*handle_result)(void*, const Slice&, const Slice&),uint64_t file_access_time=10);

  // Evict any entry for the specified file number
  void Evict(uint64_t file_number);
  void adjustFilters(uint64_t file_number,uint64_t file_size,int n);
  size_t GetTableCurrFiltersSize(uint64_t file_number,uint64_t file_size);
  uint64_t LookupFreCount(uint64_t file_number);
  void SetFreCount(uint64_t file_number,uint64_t freCount);
  std::string LRU_Status();
  void SaveLevel0Freq(uint64_t file_number);
  void TurnOnAdjustment();
  void TurnOffAdjustment();

  void *getCacheValue(Cache::Handle* handle) {return cache_->Value(handle);}
  void releaseCacheHandle(Cache::Handle* handle) {cache_->Release(handle);}
  
 private:
  Env* const env_;
  const std::string dbname_;
  const Options* options_;
  Cache* cache_;
  friend class VersionSet;
  friend class Version;
  Status FindBufferedTable(uint64_t file_number, uint64_t file_size, Cache::Handle**,TableAndFile *rtf);
  Status FindTable(uint64_t file_number, uint64_t file_size, Cache::Handle**,bool Get=false,int file_level=false, TableMetaData *tableMetaData=NULL, std::vector<uint64_t> *input_0_numbers = NULL, std::vector<uint64_t> *input_1_numbers = NULL);
  uint64_t level0_freq;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_TABLE_CACHE_H_
