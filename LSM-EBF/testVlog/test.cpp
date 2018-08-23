//
// Created by wujy on 8/21/18.
//

#include "leveldb/vlog.h"
#include <iostream>
#include <string>
#include <leveldb/filter_policy.h>

using namespace std;

int main(){
    leveldb::SepDB* db;
    leveldb::Options options;


    options.create_if_missing=true;
    leveldb::SepDB::Open(options,"./test","./vlog",&db);
    for(int i=0;i<100000000;i++){
        db->Put(leveldb::WriteOptions(),to_string(i),"a;sdlkfja;lskdfjlas;dkfjsal;kfjas;ldfkjsa;lfkjsaldfksaj;dlfksaj;lfksaj;lfkasjd;lfaksjdf;lsakjf;laskfjas;ldkjasl;fl;skfjsldkfjsldkfjslkfjsal;fjsal;fjsa;lkfj;slakfjsl;afjasl;fjasl;fjaslfjalskfjlaskfjlas;fj;laskfjlsajflskjflskjflskfjlksfjlskfjlsfjlskfjlskfjslkfjlsdkfjlsfjlskfjslkfjslfjslfjlskfjslfjslfjslkfjslkfjslkfjslkfjslfjslkfjslkfjslkfjslkfjslkfjs");
    }
    db->close();
    return 0;
}
