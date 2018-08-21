#!/bin/bash
DISK=SSD
dbfilename=/home/ming/vlog"$DISK"Dir10/lsm
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
    __modifyConfig directIOFlag false
    ./ycsbc -db leveldb -threads 1 -P $workloadw_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad false > "$loadname"
    sync;echo 1 > /proc/sys/vm/drop_caches
    sleep 100s
    if [ ! -d "$dirname" ]; then
	mkdir -p "$dirname"
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
    if [ ! -d "$dirname" ]; then
	mkdir -p "$dirname"
    fi
    __modifyConfig directIOFlag true
    for j in `seq 1 2`
    do
        ./ycsbc -db leveldb -threads 1 -P $workloadr_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad true > "$runname"_"$j".txt
	sync;echo 1 > /proc/sys/vm/drop_caches
	mv "$runname"_"$j".txt "$dirname"
	mv testlf1.txt "$dirname"/latency_l"$levelIn"_lsmtype_"$ltype"_bloom_"$bb"_"$j"_noseek_fix"$j".txt
	mv level?_filter_count_?.txt "$dirname"/
        sleep 100s
    done

}


types=(lsm)
bloom_bit_array=(8)
level=6
FilterCapacityRatio=4.0
for lsmtype in ${types[@]}
do
    __modifyConfig bloomType 2
    __modifyConfig seekCompactionFlag false
    __modifyConfig FilterCapacityRatio "$FilterCapacityRatio"
    dbfilename="$dbfilename""$level"
    echo dbfilename: "$dbfilename"
    for bloombits in ${bloom_bit_array[@]}
    do
	echo Counterpart bloombits:"$bloombits"
	__modifyConfig bloomBits  "$bloombits"
	dirname=/home/ming/workspace/YCSB-C/lsm_"$DISK"_read_zipfian_multi_filter/filter_capacity_ratio_"$FilterCapacityRatio"/200k_requests
	#__loadLSM bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits"
	__runLSM bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits"
    done
done

#__runGLSM


