import sys
import random

if len(sys.argv) != 4:
    print "Usage: genrandom.py numthreads numblocks maxblocksize\n"

numthreads = int(sys.argv[1])
numblocks = int(sys.argv[2])
maxblocksize = int(sys.argv[3])

trace = []
i = 0

# make a bunch of mallocs
while i < numblocks:
    size = random.randint(0, maxblocksize)
    size = (size / 4) * 4
    tid = random.randint(0, numthreads)
    trace.append(('m', tid, i,  size))
    i += 1

# add frees in appropriate places


print "('m', tid, i, size)"
for i in range(len(trace)):
    print trace[i]

i = 0
while i < numblocks:
    minval = i
    while minval < (numblocks + i):
        if (trace[minval][0] == 'm') and (trace[minval][2] == i):
            break
        minval += 1

    #pos = random.randint(0, (numblocks + i - minval -1) + minval + 1)
    pos = random.randint(0, (numblocks + i - minval -1))
    print str(i) + ", upper = " + str(numblocks + i - minval -1)
    print str(i) + ", pos is " + str(pos) + " minval is " + str(minval)
    trace.insert(pos + minval + 1, ('f', trace[minval][1], i))

    i += 1

for el in trace:
    if el[0] == 'm':

        print el[0], str(el[1]), str(el[2]), str(el[3])
    elif el[0] == 'f':
        print el[0], str(el[1]), str(el[2])
