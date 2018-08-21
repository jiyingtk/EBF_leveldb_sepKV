#!/bin/bash
file_name=./workloads/workload1.spec
dbfilename=/home/ming/testVLogDB
logname=/home/ming/log
configpath=/home/ming/projects/key_value/bin/config.ini
rm -rf "$dbfilename"
rm -rf "$logname"
./ycsbc -db vlogwbdb -threads 1 -P "$file_name" -dbfilename "$dbfilename"  -configpath "$configpath"
