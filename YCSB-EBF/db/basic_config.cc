#include"basic_config.hh"
#include<iostream>
template<typename T>
void Basic_ConfigMod<T>::setConfigPath (const char* path) {
    boost::property_tree::ini_parser::read_ini(path, _pt);
    assert (!_pt.empty());
}
template<typename T>
bool Basic_ConfigMod<T>::readBool (const char* key) {
    return _pt.get<bool>(key);
}
template<typename T>
int Basic_ConfigMod<T>::readInt (const char* key) {
    return _pt.get<int>(key);
}
template<typename T>
std::string Basic_ConfigMod<T>::readString (const char* key) {
    return _pt.get<std::string>(key);
}

template<typename T>
double Basic_ConfigMod<T>::readFloat(const char *key){
    return _pt.get<double>(key);
}

template<typename T>
ULL Basic_ConfigMod<T>::readULL(const char *key){
    return _pt.get<uint64_t>(key);
}

template<typename T>
size_t Basic_ConfigMod<T>::readSize_t(const char *key){
    return _pt.get<size_t>(key);
}

void LevelDB_ConfigMod::setConfigPath(const char*path){
    boost::property_tree::ini_parser::read_ini(path, _pt);
    assert (!_pt.empty());
    _bloom_filename = readString("basic.bloomFileName");
    _bloom_bits = readInt("basic.bloomBits");
    std::cout<<"_bloom_bits_"<<_bloom_bits<<std::endl;
    _max_file_size = readInt("basic.maxFileSize");
    _region_divide_size = readULL("basic.regionDivideSize");
    _max_open_files = readInt("basic.maxOpenFiles");
    _bloom_type = readInt("basic.bloomType");
    _open_log = readBool("basic.openLog");
    _compression_flag = readBool("basic.compressionFlag");
    _directIO_flag = readBool("basic.directIOFlag");
    _seek_compaction_flag = readBool("basic.seekCompactionFlag");
    _force_disable_compaction_flag = readBool("basic.forceDisableCompactionFlag");
    _statistics_open = readBool("basic.statisticsOpen");
    _bloom_bits_array_filename = readString("basic.bitsArrayFileName");
    _setFreCountInCompaction = readBool("basic.setFreCountInCompaction");
    _l0_base_ratio = readFloat("basic.L0BaseRatio");
    _lrus_num = readInt("LRU.LRUNum");
    _filters_capacity_ratio = readFloat("LRU.filterCapacityRatio");
    _base_num = readInt("LRU.baseNum");
    _life_time = readULL("LRU.lifeTime");
    _logBase = readInt("LRU.logBase");
    _slowRatio = readFloat("LRU.slowRatio");
    _changeRatio = readFloat("LRU.changeRatio");
    _sizeRatio = readInt("basic.sizeRatio");
    _valueSize = readInt("basic.valueSize");
    _filterBaseLg = readInt("basic.filterBaseLg");
    _forceDeleteLevel0File = readBool("basic.forceDeleteLevel0File");
    _init_filter_num = readFloat("LRU.initFilterNum");
    _blockCacheSize = readSize_t("basic.blockCacheSize");
}
/*template<typename T>
boost::shared_ptr<T> Basic_ConfigMod<T>::instance= nullptr;*/
std::string LevelDB_ConfigMod::getBloom_filename(){
    assert(!_pt.empty());
    return _bloom_filename;
}

int LevelDB_ConfigMod::getBloom_bits(){
    assert(!_pt.empty());
    return _bloom_bits;
}

int LevelDB_ConfigMod::getMax_file_size(){
    assert(!_pt.empty());
    return _max_file_size;
}

uint64_t LevelDB_ConfigMod::getRegion_divide_size(){
    assert(!_pt.empty());
    return _region_divide_size;
}

int LevelDB_ConfigMod::getMax_open_files(){
    assert(!_pt.empty());
    return _max_open_files;
}

int LevelDB_ConfigMod::getBloomType(){
    assert(!_pt.empty());
    return _bloom_type;
}

bool LevelDB_ConfigMod::getOpen_log(){
     assert(!_pt.empty());
     return _open_log;
}

bool LevelDB_ConfigMod::getCompression_flag(){
    assert(!_pt.empty());
    return _compression_flag;
}

bool LevelDB_ConfigMod::getDirectIOFlag(){
    assert(!_pt.empty());
    return _directIO_flag;
}

bool LevelDB_ConfigMod::getSeekCompactionFlag()
{
   assert(!_pt.empty());
   return _seek_compaction_flag;
}

bool LevelDB_ConfigMod::getForceDisableCompactionFlag()
{
   assert(!_pt.empty());
   return _force_disable_compaction_flag;
}

bool LevelDB_ConfigMod::getStatisticsOpen()
{
   assert(!_pt.empty());
   return _statistics_open;
}

std::string LevelDB_ConfigMod::getBitsArrayFilename(){
    assert(!_pt.empty());
    return _bloom_bits_array_filename;
}

int LevelDB_ConfigMod::getLRUsNum()
{
    assert(!_pt.empty());
    return _lrus_num;
}

double LevelDB_ConfigMod::getFiltersCapacityRatio()
{
    assert(!_pt.empty());
    return _filters_capacity_ratio;
}

double LevelDB_ConfigMod::getL0BaseRatio()
{
    assert(!_pt.empty());
    return _l0_base_ratio;
}

int LevelDB_ConfigMod::getBaseNum()
{
    assert(!_pt.empty());
    return _base_num;
}

uint64_t LevelDB_ConfigMod::getLifeTime()
{
    assert(!_pt.empty());
    return _life_time;
}

bool LevelDB_ConfigMod::getSetFreCountInCompaction(){
    assert(!_pt.empty());
    return _setFreCountInCompaction;
}

int LevelDB_ConfigMod::getLogBase(){
    assert(!_pt.empty());
    return _logBase;
}

double LevelDB_ConfigMod::getSlowRatio(){
    assert(!_pt.empty());
    return _slowRatio;
}

double LevelDB_ConfigMod::getChangeRatio(){
    assert(!_pt.empty());
    return _changeRatio;
}


int LevelDB_ConfigMod::getInitFilterNum(){
    assert(!_pt.empty());
    return _init_filter_num;
}
size_t LevelDB_ConfigMod::getBlockCacheSize(){
    assert(!_pt.empty());
    return _blockCacheSize;
}

int LevelDB_ConfigMod::getSizeRatio()
{
     assert(!_pt.empty());
    return _sizeRatio;
}

int LevelDB_ConfigMod::getValueSize()
{
     assert(!_pt.empty());
    return _valueSize;
}

int LevelDB_ConfigMod::getFilterBaseLg()
{
     assert(!_pt.empty());
    return _filterBaseLg;
}


bool LevelDB_ConfigMod::getForceDeleteLevel0File(){
    assert(!_pt.empty());
    return _forceDeleteLevel0File;
}

template<>
boost::shared_ptr<LevelDB_ConfigMod> Basic_ConfigMod<LevelDB_ConfigMod>::instance = nullptr;	
