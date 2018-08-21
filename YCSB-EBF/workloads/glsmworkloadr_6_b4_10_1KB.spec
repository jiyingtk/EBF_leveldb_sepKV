# Yahoo! Cloud System Benchmark
# Workload A: Update heavy workload
#   Application example: Session store recording recent actions
#
#   Read/update ratio: 50/50
#   Default data size: 1 KB records (10 fields, 100 bytes each, plus key)
#   Request distribution: zipfian

recordcount=21743432
#recordcount=217434321
#recordcount=109772800
#operationcount=1211034
operationcount=2000000
#operationcount=100000
workload=com.yahoo.ycsb.workloads.CoreWorkload

readallfields=true
insertstart=21743432

#readproportion=1.0
#updateproportion=0.0
readproportion=0.5
updateproportion=0.0
scanproportion=0
insertproportion=0.5
fieldlength=1000
requestdistribution=zipfian

withtimestamp=false
timestamptracefilename=trace2.txt
withoperation=false
#hotspotdatafraction=0.2
#hotspotopnfraction=0.8
withlatency=true
latencyfilename=testlf
#adjustfilter=true
adjustfilter=false
skipratioinload=2
