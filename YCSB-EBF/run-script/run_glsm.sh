#!/bin/bash
function __runGLSM(){
./ycsbc -db leveldb -threads 1 -P $workload_name -dbfilename "testLevelDB" -configpath "$configpath" -skipLoad false
sync;echo 3 > /proc/sys/vm/drop_caches
workload_name=./workloads/glsmworkloadr_test.spec
./ycsbc -db leveldb -threads 1 -P $workload_name -dbfilename "testLevelDB" -configpath "$configpath" -skipLoad true
}

workload_name=./workloads/glsmworkloadw_test.spec
configpath=./configDir/leveldb_config.ini
section=basic
key=hierarchicalBoomflag
value=true
./modifyConfig.py "$configpath" "$section" "$key" "$value"
__runGLSM


