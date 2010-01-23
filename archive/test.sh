#!/bin/bash
#g++ -O2 -o $1 -DBANK -ltokyocabinet -Wl,-R /usr/local/lib -ltcmalloc $2
g++ -O2 -o $1 -DBANK -ltcmalloc_minimal -iquote ../include/ $2
echo -n "BANK: "
. bench.sh $1 $3

# g++ -O2 -o $1 -DBANK -DTC -ltokyocabinet -Wl,-R /usr/local/lib -ltcmalloc $2
# echo -n "TC: "
# . bench.sh $1 $3

g++ -O2 -o $1 -DDATASTORE -ltcmalloc_minimal -iquote ../include/ $2
echo -n "DATASTORE: "
. bench.sh $1 $3

# g++ -O2 -o $1 -DDEQUE -ltcmalloc $2
# echo -n "DEQUE: "
# . bench.sh $1 $3
