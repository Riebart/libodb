#!/bin/bash
g++ -O2 -o $1 -DBANK -ltcmalloc $2
echo -n "BANK: "
. bench.sh $1 $3

g++ -O2 -o $1 -DDATASTORE -ltcmalloc $2
echo -n "DATASTORE: "
. bench.sh $1 $3


g++ -O2 -o $1 -DDEQUE -ltcmalloc $2
echo -n "DEQUE: "
. bench.sh $1 $3
