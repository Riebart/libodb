#!/bin/bash
g++ -O2 -o $1 -DBANK -ltcmalloc $2

for (( j=1 ; j<=$4 ; j++ ))
do
    echo -n "BANK: "
    . bench.sh $1 $3
done

g++ -O2 -o $1 -DDATASTORE -ltcmalloc $2

for (( j=1 ; j<=$4 ; j++ ))
do
    echo -n "DATASTORE: "
    . bench.sh $1 $3
done

g++ -O2 -o $1 -DDEQUE -ltcmalloc $2

for (( j=1 ; j<=$4 ; j++))
do
    echo -n "DEQUE: "
    . bench.sh $1 $3
done