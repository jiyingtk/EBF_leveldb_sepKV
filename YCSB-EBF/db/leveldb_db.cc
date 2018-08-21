#include "leveldb_db.h"
#include <cstring>
#include"basic_config.hh"
#include<iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>
#include <vector>
#include <leveldb/cache.h>
using namespace std;

namespace ycsbc {
LevelDB::LevelDB(const char* dbfilename,const char* configPath)
{
    leveldb::Options options;
    LevelDB_ConfigMod::getInstance().setConfigPath(configPath);
    std::string bloom_filename;
    char *bloom_filename_char;
    int bloom_bits = LevelDB_ConfigMod::getInstance().getBloom_bits();;
    int max_open_files = LevelDB_ConfigMod::getInstance().getMax_open_files();
    uint64_t region_divide_size = LevelDB_ConfigMod::getInstance().getRegion_divide_size();
    int max_File_sizes = LevelDB_ConfigMod::getInstance().getMax_file_size();
    int bloom_type = LevelDB_ConfigMod::getInstance().getBloomType();
    bool seek_compaction_flag = LevelDB_ConfigMod::getInstance().getSeekCompactionFlag();
    bool force_disable_compaction_flag = LevelDB_ConfigMod::getInstance().getForceDisableCompactionFlag();
    double filter_capacity_ratio = LevelDB_ConfigMod::getInstance().getFiltersCapacityRatio();
    double l0_base_ratio = LevelDB_ConfigMod::getInstance().getL0BaseRatio();
    int base_num = LevelDB_ConfigMod::getInstance().getBaseNum();
    uint64_t life_time = LevelDB_ConfigMod::getInstance().getLifeTime();
    bool setFreCountInCompaction = LevelDB_ConfigMod::getInstance().getSetFreCountInCompaction();
    double slow_ratio = LevelDB_ConfigMod::getInstance().getSlowRatio();
    double change_ratio = LevelDB_ConfigMod::getInstance().getChangeRatio();
    int init_filter_num = LevelDB_ConfigMod::getInstance().getInitFilterNum();
    size_t block_cache_size = LevelDB_ConfigMod::getInstance().getBlockCacheSize();
    int size_ratio = LevelDB_ConfigMod::getInstance().getSizeRatio();
    int value_size = LevelDB_ConfigMod::getInstance().getValueSize();
    int kFilterBaseLg = LevelDB_ConfigMod::getInstance().getFilterBaseLg();
    bool force_delete_level0_file = LevelDB_ConfigMod::getInstance().getForceDeleteLevel0File();
    cout<<"seek compaction flag:";
    if(seek_compaction_flag){
      cout<<"true"<<endl;
    }else{
      cout<<"false"<<endl;
    }
    cout<<"force disable compaction flag:";
    if(force_disable_compaction_flag){
      cout<<"true"<<endl;
    }else{
      cout<<"false"<<endl;
    }
    /*bool log_open = LevelDB_ConfigMod::getInstance().getOpen_log();
    if(!log_open){
      options.log_open = log_open;
    }*/
    bool compression_Open = LevelDB_ConfigMod::getInstance().getCompression_flag();
    bool directIO_flag = LevelDB_ConfigMod::getInstance().getDirectIOFlag();
    if(directIO_flag){
	   options.opEp_.no_cache_io_ = true;
	   fprintf(stderr,"directIO\n");
	   //    leveldb::setDirectIOFlag(directIO_flag);
    }
    if(bloom_type == 1){
	bloom_filename = LevelDB_ConfigMod::getInstance().getBloom_filename();
	bloom_filename_char = (char *)malloc(sizeof(char)*bloom_filename.size()+1);
	strncpy(bloom_filename_char,bloom_filename.c_str(),bloom_filename.size());
	bloom_filename_char[bloom_filename.size()] = 0;
	void *bloom_filename_ptr = (void *)bloom_filename_char;
	options.filter_policy = leveldb::NewBloomFilterPolicy(reinterpret_cast<int*>(bloom_filename_ptr));
    }else if(bloom_type == 0){
	options.filter_policy = leveldb::NewBloomFilterPolicy(bloom_bits);
    }else if(bloom_type == 2){
	int bits_per_key_per_filter[32] = {0};
	int i = 0;
	std::string bits_array_filename = LevelDB_ConfigMod::getInstance().getBitsArrayFilename();
	FILE *fp = fopen(bits_array_filename.c_str(),"r");
	FILE *fpout = fopen("bits_array.txt","w");
	if(fp == NULL){
	    perror("open bits_array_filename error: ");
	}
	char c;
	while( (c=fgetc(fp)) != EOF){
	    if(!(c >= '0' && c <= '9')){
		continue;
	    }
	    bits_per_key_per_filter[i++] = c-'0';
	}
	fprintf(stderr,"bits_per_key_per_filter: ");
	fprintf(stdout,"\nbits_per_key_per_filter: ");
	fprintf(fpout,"\nbits_per_key_per_filter: ");
	for(i = 0 ; bits_per_key_per_filter[i] ; i++){
	    fprintf(stderr,"%d ",bits_per_key_per_filter[i]);
	    fprintf(stdout,"%d ",bits_per_key_per_filter[i]);
	    fprintf(fpout,"%d ",bits_per_key_per_filter[i]);
	}
	//options.opEp_.lrus_num_ = LevelDB_ConfigMod::getInstance().getLRUsNum();
	options.opEp_.lrus_num_ = i+1;
	options.filter_policy = leveldb::NewBloomFilterPolicy(bits_per_key_per_filter,bloom_bits);
	fprintf(stderr,"\n");
	printf("Counterpart bloom_bits from config:%d\n",bloom_bits);
	fclose(fpout);
    }else{
	fprintf(stderr,"Wrong filter type!\n");
    }
    
    options.opEp_.region_divide_size = region_divide_size;

    options.opEp_.freq_divide_size = options.opEp_.region_divide_size;
    // options.opEp_.freq_divide_size = 2097152 / 8;

    options.opEp_.kFilterBaseLg = kFilterBaseLg;
    options.opEp_.key_value_size = value_size;
    options.opEp_.force_delete_level0_file = force_delete_level0_file;

    options.create_if_missing = true;
    options.compression = compression_Open?leveldb::kSnappyCompression:leveldb::kNoCompression;  //compression is disabled.
    options.max_file_size = max_File_sizes;
    options.max_open_files = max_open_files;
    options.opEp_.seek_compaction_ = seek_compaction_flag;
    options.opEp_.force_disable_compaction = force_disable_compaction_flag;
    options.opEp_.filter_capacity_ratio = filter_capacity_ratio;
    options.opEp_.l0_base_ratio = l0_base_ratio;
    options.opEp_.life_time = life_time;
    options.opEp_.base_num = base_num;
    options.opEp_.setFreCountInCompaction = setFreCountInCompaction;
    options.opEp_.slow_ratio = slow_ratio;
    options.opEp_.change_ratio = change_ratio;

    options.opEp_.size_ratio = size_ratio;
 
    options.opEp_.init_filter_nums = init_filter_num;
    options.block_cache = leveldb::NewLRUCache(block_cache_size);
    fprintf(stderr,"filter_capacity_ratio: %.3lf, init_filter_num:%d change_ratio %.5lf block_cache_size %lu MB size_ratio:%d l0_base_ratio:%lf\n",filter_capacity_ratio,init_filter_num,change_ratio,block_cache_size/1024/1024,size_ratio,l0_base_ratio);

    if(LevelDB_ConfigMod::getInstance().getStatisticsOpen()){
      options.opEp_.stats_ = leveldb::CreateDBStatistics();
    }
    //    options.paranoid_checks = true;
   // leveldb::setDirectIOFlag(directIO_flag);
    leveldb::Status status = leveldb::DB::Open(options,dbfilename, &db_);
    if(!status.ok()){
	fprintf(stderr,"can't open leveldb\n");
	cerr<<status.ToString()<<endl;
	exit(0);
    }

}
bool  LevelDB::hasRead = false;
int LevelDB::Read(const string& table, const string& key, const vector< string >* fields, vector< DB::KVPair >& result)
{
    // static FILE* fexist = fopen("exist.txt", "w");
    // static FILE* fnotexist = fopen("notexist.txt", "w");
    std::string value;
    leveldb::Status s = db_->Get(leveldb::ReadOptions(), key, &value);
    if(s.IsNotFound()){
        // fprintf(fnotexist, "%s\n", key.c_str());
	// fprintf(stdout,"ycsb not found! %s\n", key.c_str());
    	return DB::kErrorNoData;
    }
    // else
    //     fprintf(fexist, "%s\n", key.c_str());

    if(!s.ok()){
         cerr<<s.ToString()<<endl;
	 fprintf(stderr,"read error\n");
	 exit(0);
    }
    hasRead = true;
    return DB::kOK;
}

int LevelDB::Insert(const string& table, const string& key, vector< DB::KVPair >& values)
{
    leveldb::Status s;
    int count = 0;
    // cout<<key<<endl;
    for(KVPair &p : values){
	//cout<<p.second.length()<<endl;
	s = db_->Put(leveldb::WriteOptions(), key, p.second);
	count++;
	if(!s.ok()){
	   fprintf(stderr,"insert error!\n");
	   cout<<s.ToString()<<endl;
	   exit(0);
	}
    }
    if(count != 1){
	  fprintf(stderr,"insert error\n");
	  exit(0);
    }
    return DB::kOK;
}

int LevelDB::Delete(const string& table, const string& key)
{
    vector<DB::KVPair> values;
    return Insert(table,key,values);
}

int LevelDB::Scan(const string& table, const string& key, int len, const vector< string >* fields, vector< vector< DB::KVPair > >& result)
{
    fprintf(stderr,"not implement yet");
    return DB::kOK;
}

int LevelDB::Update(const string& table, const string& key, vector< DB::KVPair >& values)
{
    return Insert(table,key,values);
}

void LevelDB::Close()
{
    // cerr<<"close is called"<<endl;
    // cerr<<"hasRead value: "<<hasRead<<endl;
    // if(hasRead){
    // 	printAccessFreq();
    // }
}

void LevelDB::printAccessFreq()
{
    int fd[7],i;
    int levels = 0;
    char buf[100];
    std::string num_files_str;
    snprintf(buf,sizeof(buf),"leveldb.num-files-at-level%d",levels);
    while(levels == 0 || (db_->GetProperty(buf,&num_files_str) && std::stoi(num_files_str)!=0) ){
	levels++;
	snprintf(buf,sizeof(buf),"leveldb.num-files-at-level%d",levels);
    }

    std::string acc_str;
    for(i = 0 ; i <= levels ; i++){
	    snprintf(buf,sizeof(buf),"level%d_access_frequencies.txt",i);
	    fd[i] = open(buf,O_RDWR|O_CREAT);
	    if(fd[i] < 0){
		perror("open :");
	    }
	    snprintf(buf,sizeof(buf),"leveldb.files-access-frequencies%d",i);
	    db_->GetProperty(buf,&acc_str);
	    if(write(fd[i],acc_str.c_str(),acc_str.size()) != (ssize_t)acc_str.size()){
		perror("write :");
	    }
	    close(fd[i]);


        snprintf(buf,sizeof(buf),"level%d_extra_infos.txt",i);
        fd[i] = open(buf,O_RDWR|O_CREAT);
        if(fd[i] < 0){
        perror("open :");
        }
        snprintf(buf,sizeof(buf),"leveldb.files-extra-infos%d",i);
        db_->GetProperty(buf,&acc_str);
        if(write(fd[i],acc_str.c_str(),acc_str.size()) != (ssize_t)acc_str.size()){
        perror("write :");
        }
        close(fd[i]);
    }
}

void LevelDB::printFilterCount()
{
    int fd[7],i;
    int levels = 0;
    char buf[100];
    static int call_count = 0;
    std::string num_files_str;
    snprintf(buf,sizeof(buf),"leveldb.num-files-at-level%d",levels);
    while(levels == 0 || (db_->GetProperty(buf,&num_files_str) && std::stoi(num_files_str)!=0) ){
	levels++;
	snprintf(buf,sizeof(buf),"leveldb.num-files-at-level%d",levels);
    }

    std::string filter_str;
    for(i = 0 ; i < levels ; i++){
            snprintf(buf,sizeof(buf),"level%d_filter_count_%d.txt",i,call_count);
	    fd[i] = open(buf,O_RDWR|O_CREAT);
	    if(fd[i] < 0){
		perror("open :");
	    }
	    snprintf(buf,sizeof(buf),"leveldb.file_filter_size%d",i);
	    db_->GetProperty(buf,&filter_str);
	    if(write(fd[i],filter_str.c_str(),filter_str.size()) != (ssize_t)filter_str.size()){
		perror("write :");
	    }
	    close(fd[i]);
    }
    call_count++;
}

void LevelDB::doSomeThing(const char* thing_str)
{
  if(strncmp(thing_str,"adjust_filter",strlen("adjust_filter")) == 0){
    db_->DoSomeThing((void*)"adjust_filter");
  }else if(strncmp(thing_str,"printFilterCount",strlen("printFilterCount")) == 0){
    printFilterCount();
  }else if(strncmp(thing_str,"printStats",strlen("printStats")) == 0){
    std::string stat_str;
    db_->GetProperty("leveldb.stats",&stat_str);
    cout<<stat_str<<endl;
  }else if(strncmp(thing_str,"printAccessFreq",strlen("printAccessFreq")) == 0){
	printAccessFreq();
  }else if(strncmp(thing_str,"printFP",strlen("printFP")) == 0){
    int fd = open("fp_access_file.txt",O_RDWR|O_CREAT);
    if(fd < 0){
        perror("open :");
    }
    std::string stat_str;
    db_->GetProperty("leveldb.fp-stat-access_file",&stat_str);
    if(write(fd,stat_str.c_str(),stat_str.size()) != (ssize_t)stat_str.size()){
        perror("write :");
    }
    close(fd);

    fd = open("fp_calc_fpr.txt",O_RDWR|O_CREAT);
    if(fd < 0){
        perror("open :");
    }
    stat_str.clear();
    db_->GetProperty("leveldb.fp-stat-calc_fpr",&stat_str);
    if(write(fd,stat_str.c_str(),stat_str.size()) != (ssize_t)stat_str.size()){
        perror("write :");
    }
    close(fd);

    fd = open("fp_real_fpr.txt",O_RDWR|O_CREAT);
    if(fd < 0){
        perror("open :");
    }
    stat_str.clear();
    db_->GetProperty("leveldb.fp-stat-real_fpr",&stat_str);
    if(write(fd,stat_str.c_str(),stat_str.size()) != (ssize_t)stat_str.size()){
        perror("write :");
    }
    close(fd);

    fd = open("fp_real_io.txt",O_RDWR|O_CREAT);
    if(fd < 0){
        perror("open :");
    }
    stat_str.clear();
    db_->GetProperty("leveldb.fp-stat-real_io",&stat_str);
    if(write(fd,stat_str.c_str(),stat_str.size()) != (ssize_t)stat_str.size()){
        perror("write :");
    }
    close(fd);
  }
}

void LevelDB::openStatistics(){
    std::string stat_str;
    db_->GetProperty("leveldb.stats",&stat_str);
    cout<<"--------------------------- Before Do Transaction -----------------------------------------"<<endl;
    cout<<stat_str<<endl;
    cout<<"----------------------------Transaction Output------------------"<<endl;
}

LevelDB::~LevelDB()
{
    delete db_;
}




}
