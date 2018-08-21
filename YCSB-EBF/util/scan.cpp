#include<leveldb/db.h>
#include <leveldb/env.h>
#include<leveldb/filter_policy.h>
#include<iostream>
#include<string>
#include<leveldb/statistics.h>
using namespace std;
int main(int argc,char *argv[]){
  char *dbfilename = argv[1];
  leveldb::Options options;
  leveldb::DB *db;
  int i, iter_nums;
  options.opEp_.stats_ = leveldb::CreateDBStatistics();
  int bits_per_key_per_filter[10]={5,5,5,0};
  int bloom_bits=8;
  options.filter_policy = leveldb::NewBloomFilterPolicy(bits_per_key_per_filter,bloom_bits);
  options.opEp_.lrus_num_ = 4;
  options.opEp_.size_ratio = 10;
  leveldb::Status status = leveldb::DB::Open(options,dbfilename, &db);
  if(!status.ok()){
    fprintf(stderr,"can't open leveldb\n");
    cerr<<status.ToString()<<endl;
    exit(0);
  }
  leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
  iter_nums = atoi(argv[2]);
  i = 0; 
  for (it->SeekToFirst(); it->Valid() && i < iter_nums; it->Next()) {
    cout << it->key().ToString() << ": "  << it->value().ToString() << endl;
    i++;
  }
  delete it;
  delete db;
  return 0;
}
