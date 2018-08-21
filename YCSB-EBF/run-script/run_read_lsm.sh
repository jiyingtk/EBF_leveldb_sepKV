#!/bin/bash
DISK=SSD
dbfilename=/home/ming/vlog"$DISK"Dir100/lsm
configpath=./configDir/leveldb_config.ini
section=basic

function __modifyConfig(){
    key=$1
    value=$2
    ./modifyConfig.py "$configpath" "$section" "$key" "$value"   
}

function __loadLSM(){
    rm -rf "$dbfilename"
    loadname=$1
    loadname="$loadname"_load.txt
    dirname=$2
    levelIn=$3
    ltype=$4
    bb=$5
    workloadw_name=./workloads/glsmworkloadw_"$levelIn".spec
    ./ycsbc -db leveldb -threads 1 -P $workloadw_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad false > "$loadname"
    sync;echo 1 > /proc/sys/vm/drop_caches
    sleep 100s
    if [ ! -d "$dirname" ]; then
	mkdir "$dirname"
    fi
    mv "$loadname" "$dirname"
}

function __runLSM(){
    runname=$1
    runname="$runname"_run
    dirname=$2
    levelIn=$3
    ltype=$4
    bb=$5
    workloadr_name=./workloads/glsmworkloadr_"$levelIn".spec
    for j in `seq 1 4`
    do
	./ycsbc -db leveldb -threads 1 -P $workloadr_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad true > "$runname"_"$j".txt
	sync;echo 1 > /proc/sys/vm/drop_caches
	mv "$runname"_"$j".txt "$dirname"
	mv testlf.txt "$dirname"/latency_l"$levelIn"_lsmtype_"$ltype"_bloom_"$bb"_"$j".txt
        sleep 100s
    done
    #open seek compaction
    __modifyConfig seekCompactionFlag true
     runname="$runname"_seekcp
    for j in `seq 1 4`
    do
	./ycsbc -db leveldb -threads 1 -P $workloadr_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad true > "$runname"_"$j".txt
	sync;echo 1 > /proc/sys/vm/drop_caches
	mv "$runname"_"$j".txt "$dirname"
	mv testlf.txt "$dirname"/latency_l"$levelIn"_lsmtype_"$ltype"_bloom_"$bb"_"$j".txt
        sleep 100s
    done
}

leveldb_dir=/home/ming/workspace/leveldb_hierarchical
types=(lsm-hierarchical)
bloom_bit_array=(8)
levels=(4 5 6)
for lsmtype in ${types[@]}
do
    __modifyConfig hierarchicalBoomflag false
    for bloombits in ${bloom_bit_array[@]}
    do
	echo bloombits:"$bloombits"
	__modifyConfig bloomBits  "$bloombits"
	__loadLSM bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" /home/ming/workspace/YCSB-C/lsm_"$DISK"_read_compaction "$level"  "$lsmtype" "$bloombits"
	__runLSM bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" /home/ming/workspace/YCSB-C/lsm_"$DISK"_read_compaction "$level"  "$lsmtype" "$bloombits"
    done
done

#__runGLSM


