//
// Created by wujy on 8/23/18.
//

#ifndef YCSB_EBF_LEVELDBVLOG_DB_H
#define YCSB_EBF_LEVELDBVLOG_DB_H

#include "core/db.h"
#include <iostream>
#include <string>
#include "core/properties.h"
#include <leveldb/vlog.h>
#include <leveldb/env.h>
#include<leveldb/filter_policy.h>
#include<leveldb/statistics.h>
using std::cout;
using std::endl;

namespace ycsbc {
    class LeveldbVlog : public DB{
    public :
        LeveldbVlog(const char *dbfilename,const char* configPath);
        int Read(const std::string &table, const std::string &key,
                 const std::vector<std::string> *fields,
                 std::vector<KVPair> &result);

        int Scan(const std::string &table, const std::string &key,
                 int len, const std::vector<std::string> *fields,
                 std::vector<std::vector<KVPair>> &result) ;

        int Update(const std::string &table, const std::string &key,
                   std::vector<KVPair> &values);

        int Insert(const std::string &table, const std::string &key,
                   std::vector<KVPair> &values) ;

        int Delete(const std::string &table, const std::string &key);
        void openStatistics();
        void printAccessFreq();
        virtual ~LeveldbVlog();
        virtual void doSomeThing(const char *thing_str="adjust_filter");
        void Close();
    private:
        leveldb::SepDB *db_;
        static bool hasRead;
        void printFilterCount();
    };
}


#endif //YCSB_EBF_LEVELDBVLOG_DB_H
