import sys
import re
import os


file_str = "level%d_access_frequencies.txt"
out_region_ids = [0, 1, 2]

def is_empty(file_name):
	if not os.path.exists(file_name):
		return True
	else:
		f = open(file_name, "r")
		line = f.readline()
		f.close()
		if len(line) <= 1:
			return True
		else:
			return False

def parse(fin, fout, dtype = "fill"):
	i = 1
	file_name = os.path.join(fin, file_str % (i))

	fo = open(fout, "w")
	max_region_num = 0
	while not is_empty(file_name):
		print(file_name)

		f = open(file_name, "r")
		line = f.readline()
		if line[-1] == "\n":
			line = line[:-1]

		patt = '\(([^\)]*)\)'
		ms = re.finditer(patt, line)
		used = False
		for m in ms:
			used = True
			if m.group(1) == '':
				continue
			freqs = m.group(1).split(",")
			if max_region_num < len(freqs):
				max_region_num = len(freqs)
			if dtype == "join":
				fo.write("\n".join(freqs) + "\n")
			else:
				fo.write(" ".join(freqs) + "\n")
		if not used:
			freqs = line.split(",")
			fo.write("\n".join(freqs) + "\n")

		f.close()

		i += 1
		file_name = os.path.join(fin, file_str % (i))
	fo.close()

	if dtype == "fill" and max_region_num > 1:
		fi = open(fout, "r")
		fo = open(fout + "2", "w")
		for line in fi.readlines():
			if len(line) <= 1:
				continue
			if line[-1] == "\n":
				line = line[:-1]
			freqs = line.split(" ")
			while len(freqs) < max_region_num:
				freqs.append(freqs[-1])
			fo.write(" ".join(freqs) + "\n")
		fi.close()
		fo.close()

if __name__ == '__main__':
	parse(sys.argv[1], sys.argv[2], sys.argv[3])