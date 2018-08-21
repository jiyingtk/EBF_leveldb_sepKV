#!/bin/bash
experiment_time=5
DISK=SSD"$experiment_time"
value_size=1KB
dbfilename_o=/home/ming/"$DISK"_"$value_size"/mlsm
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
    workloadw_name=./workloads/glsmworkloadw_"$levelIn"_"$sizeRatio"_"$value_size".spec
    echo "$workloadw_name"
    if [ ! -d "$dirname" ]; then
	mkdir  -p "$dirname"
    fi
    __modifyConfig directIOFlag false
    ./ycsbc -db leveldb -threads 1 -P $workloadw_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad false > "$loadname"
    sync;echo 1 > /proc/sys/vm/drop_caches
    sleep 100s
    mv "$loadname" "$dirname"
}

function __runLSM(){
    runname=$1
    runname="$runname"
    dirname=$2
    levelIn=$3
    ltype=$4
    bb=$5
    cR=$6
    workloadr_name=./workloads/glsmworkloadr_"$levelIn"_"$sizeRatio"_"$value_size".spec
    life_times=(10000)
    echo "$workloadr_name"
    __modifyConfig directIOFlag "$directIOFlag"
    section=LRU
    if [ ! -d "$dirname" ]; then
	mkdir  -p "$dirname"
    fi
    section=LRU
    for life_time in ${life_times[@]}
    do
	echo life_time "$life_time"
	__modifyConfig LifeTime "$life_time"
        ./ycsbc -db leveldb -threads 1 -P $workloadr_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad true -requestdistribution "$requestdistribution" -zipfianconst "$zipfianconst" > "$runname"_changeRatio"$cR"_lifetime"$life_time".txt
	sync;echo 1 > /proc/sys/vm/drop_caches
	mv "$runname"_changeRatio"$cR"_lifetime"$life_time".txt "$dirname"/
	mv testlf1.txt "$dirname"/latency_l"$levelIn"_lsmtype_"$ltype"_bloom_"$bb"_changeRatio"$cR"_lifetime"$life_time".txt
	mv nlf1.txt "$dirname"/nlatency_l"$levelIn"_lsmtype_"$ltype"_bloom_"$bb"_changeRatio"$cR"_lifetime"$life_time".txt
	mv level?_access_frequencies.txt "$dirname"/
        sleep 100s
    done
    cp configDir/leveldb_config.ini "$dirname"/
    section=basic
}


lsmtype=(lsm)
bloombits=5
level=6
sizeRatio=2
dbfilename="$dbfilename_o"l"$level"s"$sizeRatio"b"$bloombits"
FilterCapacityRatios=(5.0)
blockCacheSizes=(0) #MB
changeRatios=(0.00001)
initFilterNum=2
directIOFlag=false
requestdistribution=zipfian
zipfianconst=1.20
maxOpenfiles=550
bitsArrayFilename=/home/ming/workspace/bitsArray233344.txt
__modifyConfig maxOpenfiles "$maxOpenfiles"
__modifyConfig bitsArrayFilename "$bitsArrayFilename"
echo "$dbfilename"
for blockCacheSize in ${blockCacheSizes[@]}
do
    let bcs=blockCacheSize*1024*1024
    __modifyConfig blockCacheSize "$bcs" 
    __modifyConfig sizeRatio "$sizeRatio"
    for FilterCapacityRatio in ${FilterCapacityRatios[@]}
    do
	__modifyConfig bloomType 2
	__modifyConfig seekCompactionFlag false
	echo Counterpart bloombits:"$bloombits"

	__modifyConfig bloomBits  "$bloombits"
	section=LRU
	__modifyConfig FilterCapacityRatio "$FilterCapacityRatio"
	__modifyConfig initFilterNum "$initFilterNum"
	section=basic
	for changeRatio in ${changeRatios[@]}
	do
	    section=LRU
	    __modifyConfig changeRatio "$changeRatio"
	    section=basic
	    if [ "$requestdistribution" = "zipfian" ]; then
		echo "zipfian"
		dirname=/home/ming/experiment/expectation/mlsm_"$DISK"_read_"$requestdistribution""$zipfianconst"_multi_filter_sizeRatio"$sizeRatio"/experiment"$experiment_time"/NoModFilterCapacityRatio_"$FilterCapacityRatio"_lru0_30WRead_initFilterNum"$initFilterNum"_directIO_"$directIOFlag"_blockCacheSize"$blockCacheSize"MB
	    else
		echo "$requestdistribution"
		dirname=/home/ming/experiment/expectation/mlsm_"$DISK"_read_"$requestdistribution"_multi_filter_sizeRatio"$sizeRatio"/experiment"$experiment_time"/NoModFilterCapacityRatio_"$FilterCapacityRatio"_lru0_30WRead_initFilterNum"$initFilterNum"_directIO_"$directIOFlag"_blockCacheSize"$blockCacheSize"MB
	    fi
	    __loadLSM bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits"
	    __runLSM bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits" "$changeRatio"
	done
    done
done
#__runGLSM


