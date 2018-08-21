import os
import time

key = "ycsb"
print("key word: " + key)
while True:
	os.system("date >> mem.txt")
	os.system("ps -e -o 'pid,comm,pcpu,rsz,vsz,stime' | grep %s >> mem.txt" % (key))
	#os.system("ps -e -o 'pid,comm,pcpu,rsz,vsz,stime' | grep staged >> mem.txt")
#	os.system("ps -e -o 'pid,comm,pcpu,rsz,vsz,stime' | grep db_bench >> mem.txt")
	time.sleep(10)

