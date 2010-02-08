#!/bin/bash
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=3 ; i++ ))
    do
        echo -n "./test 8 1000000 10 $j $i..."
        ./test 8 1000000 10 $j $i > /dev/null
        echo " Done"
    done
done