#!/bin/bash
file_name=./workloads/workloadt.spec
./ycsbc -db leveldb -threads 1 -P $file_name -dbfilename "testLevelDB" 