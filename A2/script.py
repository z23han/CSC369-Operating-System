#!/bin/bash

#remove print_pagedirectory() in sim.c

import subprocess

algos = ["opt"]

files = ["/u/csc369h/winter/pub/a2-traces/tr-blocked.ref", "/u/csc369h/winter/pub/a2-traces/tr-matmul.ref"]

mems = [150, 200]

print "start"

for file in files:
	print("File: %s" % (file))
	for algo in algos:
		print("Algo: %s" % (algo))
		for mem in mems:
			script = "time ./sim -f %s -m %d -s 3000 -a %s" % (file, mem, algo)
			print(script)
			#subprocess.call("time ./sim -f sample/tr-simpleloop.ref -m 50 -s 3000 -a clock", shell=True)
			subprocess.call(script, shell=True)

print "end"