File Edit Options Buffers Tools Sh-Script Help                                                                      
# Yahoo! Cloud System Benchmark                                                                                     
# Workload A: Update heavy workload                                                                                 
#   Application example: Session store recording recent actions                                                     
#                                                                                                                   
#   Read/update ratio: 50/50                                                                                        
#   Default data size: 1 KB records (10 fields, 100 bytes each, plus key)                                           
#   Request distribution: zipfian                                                                                   

recordcount=2600960
#operationcount=1211034                                                                                             
operationcount=3836
workload=com.yahoo.ycsb.workloads.CoreWorkload

readallfields=true

readproportion=0.0
updateproportion=0.0
scanproportion=0
insertproportion=1.0

fieldlength=1000
requestdistribution=uniform
insertstart=2600961
withtimestamp=false
timestamptracefilename=trace2.txt
withoperation=false
withlatency=true
latencyfilename=testlf
skipratioinload=2
adjustfilter=false

