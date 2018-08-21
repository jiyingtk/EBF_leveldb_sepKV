import sys
import re


basex = 10 #10000
def parse(fin, fout, dtype = "diff"):
	fi = open(fin, "r") 
	fo = open(fout, "w")
	fo.write("#from file:" + fin + "\n")
	fo.write("#data type:" + dtype + "\n")
	# for line in fi:
	line = fi.readline()
	datas = line.split(",")

	y0 = 0
	for i in range(len(datas)):
		if datas[i] == '':
			continue
		y = float(datas[i])
		if dtype == "diff":
			y = y * (i + 1) - y0 * i
			y0 = float(datas[i])
		fo.write(str(basex * (i + 1)) + " " + str(y) + "\n")

	fi.close()
	fo.close()

if __name__ == '__main__':
	parse(sys.argv[1], sys.argv[2], sys.argv[3])