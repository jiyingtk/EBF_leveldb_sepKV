#!/bin/sh



function __modifyConfig() {
    key=$1
    value=$2
    ./modifyConfig.py "$configpath" "$section" "$key" "$value"   
}

function __loadLSM() {
    rm -rf "$dbfilename"
    rm -rf "$vlogfilename"
    loadname=$1
    loadname="$loadname"_load.txt
    dirname=$2
    levelIn=$3
    ltype=$4
    bb=$5
    # workloadw_name=./workloads/workloada.spec
    workloadw_name=./workloads/glsmworkloadw_"$levelIn"_b4_10_"$value_size".spec
    echo workload_name $workloadw_name

    if [ ! -d "$dirname" ]; then
        mkdir  -p "$dirname"
    fi
    __modifyConfig directIOFlag false
    echo ./ycsbc -db leveldbVlog -threads 4 -P $workloadw_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad false \> "$loadname"
    ./ycsbc -db leveldbVlog -threads 4 -P $workloadw_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad false > "$loadname"
    sync;echo 1 > /proc/sys/vm/drop_caches
    echo finish ycsb load
    sleep 100s
    mv "$loadname" "$dirname"
}

function __runLSM(){
    runname=$1
    runname="$runname"_a"$arrayname"
    dirname=$2
    levelIn=$3
    ltype=$4
    bb=$5
    cR=$6
    RegionDS=$7
    # workloadr_name=./workloads/workloada.spec
    workloadr_name=./workloads/glsmworkloadr_"$levelIn"_b4_10_"$value_size".spec
    echo workload_name $workloadr_name

    life_times=(20000)
    __modifyConfig directIOFlag "$directIOFlag"
    section=LRU
    if [ ! -d "$dirname" ]; then
    mkdir  -p "$dirname"
    fi
    if [ ! -d "$dirname"/"$RegionDS" ]; then
    mkdir  -p "$dirname"/"$RegionDS"
    fi
    section=LRU
    for life_time in ${life_times[@]}
    do
    echo life_time "$life_time"
    __modifyConfig LifeTime "$life_time"
    if [ x$workload_prefix != x ]
    then
        ./ycsbc -db leveldbVlog -threads 1 -P $workloadr_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad true -requestdistribution "$requestdistribution" -zipfianconst "$zipfianconst"
    else
            echo ./ycsbc -db leveldbVlog -threads 2 -P $workloadr_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad true -requestdistribution "$requestdistribution" -zipfianconst "$zipfianconst" -readTheadNums 0 \> "$runname"_changeRatio"$cR"_lifetime"$life_time".txt 2\>\&1
            ./ycsbc -db leveldbVlog -threads 2 -P $workloadr_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad true -requestdistribution "$requestdistribution" -zipfianconst "$zipfianconst" -readTheadNums 0 > "$runname"_changeRatio"$cR"_lifetime"$life_time".txt 2>&1
        sync;echo 1 > /proc/sys/vm/drop_caches
        echo finish ycsbc run
        chmod +r fp_access_file.txt
        chmod +r fp_calc_fpr.txt
        chmod +r fp_real_fpr.txt
        chmod +r fp_real_io.txt
        mv fp_access_file.txt "$dirname"/"$RegionDS"/
        mv fp_calc_fpr.txt "$dirname"/"$RegionDS"/
        mv fp_real_fpr.txt "$dirname"/"$RegionDS"/
        mv fp_real_io.txt "$dirname"/"$RegionDS"/
        mv "$runname"_changeRatio"$cR"_lifetime"$life_time".txt "$dirname"/"$RegionDS"/
        mv testlf1.txt "$dirname"/"$RegionDS"/latency_"$runname"_changeRatio"$cR"_lifetime"$life_time".txt
        mv nlf1.txt "$dirname"/"$RegionDS"/nlatency_"$runname"_changeRatio"$cR"_lifetime"$life_time".txt
        mv level?_access_frequencies.txt "$dirname"/"$RegionDS"/
            sleep 100s
    fi
    done
    cp configDir/leveldb_config.ini "$dirname"/
    section=basic
}


experiment_time=5
dbfilename_o=./ssd2/mlsmvlog
vlogfilename=./ssd2/vlog
configpath=./configDir/leveldb_config.ini
section=vlog
__modifyConfig vlogFileName "$vlogfilename"
section=basic
lsmtype=(mleveldb)

# arrayname=455555
# arrayname=355555
arrayname=444444
# arrayname=2222222222

level=6
bloombits=4
sizeRatio=10

dbsize=10GB
tablesize=2   #MB

value_size=1KB
# value_size=100B

dbfilename="$dbfilename_o"l"$level"s"$sizeRatio"b"$bloombits"a"$arrayname"db"$dbsize"table"$tablesize"MBkv"$value_size"

outnameprefix=halfrmcp

initFilterNum=2
changeRatios=(0.0001)
blockCacheSizes=(8) #MB
FilterCapacityRatios=(4.0)

directIOFlag=true
# directIOFlag=false

requestdistribution=zipfian
# requestdistribution=uniform

zipfianconsts=(0.99)
# zipfianconsts=(1.10)

DisableCompaction=false

maxOpenfiles=6100
# maxOpenfiles=1750


regionDivideSizes=(16384) #KB

run_type=2
# 0: only load, 1: only run, 2: load+run


bitsArrayFilename=./bitsArray/bitsArray"$arrayname".txt
echo dbfilename: "$dbfilename"
echo vlogfilename: "$vlogfilename"
let ts=tablesize*1024*1024
__modifyConfig valueSize 1024
__modifyConfig filterBaseLg 16
__modifyConfig bitsArrayFilename "$bitsArrayFilename"
__modifyConfig maxOpenfiles "$maxOpenfiles"
__modifyConfig maxFilesize "$ts"
__modifyConfig L0BaseRatio 1.0
__modifyConfig forceDeleteLevel0File false
__modifyConfig forceDisableCompactionFlag "$DisableCompaction"

workload_prefix=$1
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
            for regionDivideSize in ${regionDivideSizes[@]}
            do
                section=basic
                let rds=regionDivideSize*1024
                __modifyConfig RegionDivideSize "$rds"
                
                section=LRU
                __modifyConfig changeRatio "$changeRatio"
                section=basic
                if [ "$requestdistribution" = "zipfian" ]; then
                    echo request_distribution: "zipfian"
                    for zipfianconst in ${zipfianconsts[@]}
                    do
                        # dirname=/home/ming/experiment/expectation/lsm_"$DISK"_read_"$requestdistribution""$zipfianconst"_multi_filter_sizeRatio"$sizeRatio"/experiment"$experiment_time"_"$value_size"/FilterCapacityRatio_"$FilterCapacityRatio"_lru0_100WRead_initFilterNum"$initFilterNum"_directIO_"$directIOFlag"_blockCacheSize"$blockCacheSize"MB
                        dirname=./result

                        if [ $run_type = 0 ]; then
                            echo load mode
                            __loadLSM "$outnameprefix"_load_bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits" "$changeRatio"
                        elif [ $run_type = 1 ]; then
                            echo run mode
                            __runLSM "$outnameprefix"_run_bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits" "$changeRatio" RDS"$regionDivideSize"KB
                        else
                            echo load + run mode
                            __loadLSM "$outnameprefix"_load_bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits" "$changeRatio"
                            __runLSM "$outnameprefix"_run_bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits" "$changeRatio"
                        fi

                    done
                else
                    echo request_distribution: "$requestdistribution"
                    # dirname=/home/ming/experiment/expectation/lsm_"$DISK"_read_"$requestdistribution"_multi_filter_sizeRatio"$sizeRatio"/experiment"$experiment_time"_"$value_size"/FilterCapacityRatio_"$FilterCapacityRatio"_lru0_100WRead_initFilterNum"$initFilterNum"_directIO_"$directIOFlag"_blockCacheSize"$blockCacheSize"MB
                    dirname=./result
                    zipfianconst=0.99

                    if [ $run_type = 0 ]; then
                        echo load mode
                        __loadLSM "$outnameprefix"_load_bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits" "$changeRatio"
                    elif [ $run_type = 1 ]; then
                        echo run mode
                        __runLSM "$outnameprefix"_run_bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits" "$changeRatio"
                    else
                        echo load + run mode
                        __loadLSM "$outnameprefix"_load_bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits" "$changeRatio"
                        __runLSM "$outnameprefix"_run_bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" "$dirname" "$level"  "$lsmtype" "$bloombits" "$changeRatio"
                    fi

                fi
            done
        done
    done
done
