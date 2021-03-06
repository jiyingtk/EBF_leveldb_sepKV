# Yahoo! Cloud System Benchmark
# Workload A: Update heavy workload
#   Application example: Session store recording recent actions
#
#   Read/update ratio: 50/50
#   Default data size: 1 KB records (10 fields, 100 bytes each, plus key)
#   Request distribution: zipfian

recordcount=1000000
#recordcount=2174343
#operationcount=1211034
operationcount=0
workload=com.yahoo.ycsb.workloads.CoreWorkload

readallfields=true

readproportion=0.5
updateproportion=0.5
scanproportion=0
insertproportion=0
fieldlength=1000
requestdistribution=zipfian

withtimestamp=false
timestamptracefilename=trace2.txt
withoperation=false
#hotspotdatafraction=0.2
#hotspotopnfraction=0.8
withlatency=true
latencyfilename=testlf
adjustfilter=false
skipratioinload=2
