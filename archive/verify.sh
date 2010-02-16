#!/bin/bash
# Arg1: Number of items for each test.
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        i2=$(echo "2*$i" | bc)
        echo -n "./test 8 $1 10 $j $i2..."
        ./test 8 $1 10 $j $i2 | grep "^:.*$"
    done
done

echo ""

for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        i2=$(echo "2*$i+1" | bc)
        echo -n "./test 8 $1 10 $j $i2..."
        ./test 8 $1 10 $j $i2 | grep "^:.*$"
    done
done