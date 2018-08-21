#!/bin/bash
DISK=SSD
dbfilename=/home/ming/vlog"$DISK"Dir100/glsm
function __runGLSM(){
    rm -rf "$dbfilename"
    loadname=$1
    loadname="$loadname"_load.txt
    runname=$1
    runname="$runname"_run
    dirname=$2
    levelIn=$3
    ltype=$4
    bb=$5
    workloadw_name=./workloads/glsmworkloadw_"$levelIn".spec
    __modifyConfig directIOFlag false
    ./ycsbc -db leveldb -threads 1 -P $workloadw_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad false > "$loadname"
    sync;echo 1 > /proc/sys/vm/drop_caches
    mv "$loadname" "$dirname"
    sleep 120s
    workloadr_name=./workloads/glsmworkloadr_"$levelIn".spec
    __modifyConfig directIOFlag true
    for j in `seq 1 4`
    do
	./ycsbc -db leveldb -threads 1 -P $workloadr_name -dbfilename "$dbfilename" -configpath "$configpath" -skipLoad true > "$runname"_"$j".txt
	sync;echo 1 > /proc/sys/vm/drop_caches
	mv "$runname"_"$j".txt "$dirname"
	mv testlf.txt "$dirname"/latency_l"$levelIn"_glsmtype_"$ltype"_bloom_"$bb"_"$j".txt
	mv nlf.txt "$dirname"/nlatency_l"$levelIn"_lsmtype_"$ltype"_bloom_"$bb"_"$j".txt
	sleep 120s
    done
}


configpath=./configDir/leveldb_config.ini
section=basic
function __modifyConfig(){
    key=$1
    value=$2
    ./modifyConfig.py "$configpath" "$section" "$key" "$value"   
}

leveldb_dir=/home/ming/workspace/leveldb_hierarchical

#run glsm
function __checkOutBranch(){
    branch=$1
    cd "$leveldb_dir"
    git checkout "$branch"
    ./install_leveldb.sh
    cd -
    make
}
#branches=(glsm)
types=(glsm-hierarchical glsm)
bloom_bit_array=(10)
levels=(6)
for lsmtype in ${types[@]}
do
    __checkOutBranch glsm-hierarchical
    __modifyConfig hierarchicalBoomflag true
    for bloombits in ${bloom_bit_array[@]}
    do
	echo bloombits:"$bloombits"
	__modifyConfig bloomBits  "$bloombits"
	for level in ${levels[@]}
	do
	   echo level:"$level"
	   if [ "$lsmtype" = "glsm" ]; then
	       echo "glsm"
	       __modifyConfig bloomFilename /home/ming/workspace/blooml"$level"_"$bloombits".txt
	   else
	       echo "glsm-hierarchical"
	       __modifyConfig bloomFilename /home/ming/workspace/blooml"$level"_"$bloombits"_h.txt
	   fi
	   __runGLSM bloombits"$bloombits"_level"$level"_lsmtype_"$lsmtype" /home/ming/workspace/YCSB-C/glsm_"$DISK"_read "$level" "$lsmtype" "$bloombits"
	done
    done
done

#__runGLSM


