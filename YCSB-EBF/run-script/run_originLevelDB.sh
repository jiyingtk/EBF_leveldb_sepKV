#!/bin/bash
experiment_time=6
DISK=SSD"$experiment_time"
dbfilename=/home/ming/RAID0_"$DISK"/lsm
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
	mkdir  -p "$dirname"
    fi
    mv "$loadname" "$dirname"
    cp configDir/leveldb_config.ini "$dirname"
}

function __runLSM(){
    runname=$1
    runname="$runname"_run
    dirname=$2
    levelIn=$3
    ltype=$4
    bb=$5
    workloadr_name=./workloads/glsmworkloadr_"$levelIn".spec
    __modifyConfig directIOFlag true
    for j in `seq 1 2`
    do
	let count=300/"$j"
	vmstat -n "$j" "$count"  > vmstat_"$count".txt &
        ./ycsbc -db leveldb -threads 1 -P $workloadr_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad true > "$runname"_"$j".txt
	sync;echo 1 > /proc/sys/vm/drop_caches
	if [ ! -d "$dirname" ]; then
	    mkdir  -p "$dirname"
	fi
	mv "$runname"_"$j".txt "$dirname"
	mv testlf1.txt "$dirname"/latency_l"$levelIn"_lsmtype_"$ltype"_bloom_"$bb"_"$j"_noseek_fix"$j".txt
	mv nlf1.txt "$dirname"/nlatency_l"$levelIn"_lsmtype_"$ltype"_bloom_"$bb"_"$j"_noseek_fix"$j".txt
	cp vmstat_"$count".txt "$dirname"/vmstat_count"$count"_"$j".txt
        sleep 100s
    done

}


types=(lsm)
bloom_bit_array=(6)
level=6
dbfilename="$dbfilename""$level"
maxOpenfiles=55986
for lsmtype in ${types[@]}
do
    __modifyConfig bloomType 0
    __modifyConfig seekCompactionFlag false
    __modifyConfig maxOpenfiles "$maxOpenfiles"
    for bloombits in ${bloom_bit_array[@]}
    do
	echo bloombits:"$bloombits"
	__modifyConfig bloomBits  "$bloombits"
	dirname=/home/ming/workspace/YCSB-C/lsm_"$DISK"_read_zipfian1.1/experiment"$experiment_time"/bloombits"$bloombits"level"$level"/open_files_"$maxOpenfiles"_allfound_300WRead
	#__loadLSM bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits"
	__runLSM bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits"
    done
done

#__runGLSM


