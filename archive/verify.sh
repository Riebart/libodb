#!/bin/bash
# Arg1: Number of items for each test.

if [ $# -eq 0 ]; then
    N="100000"
else
    N=$1
fi

echo "Batch 1..."
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        i2=$(echo "2*$i" | bc)
        echo -n "./test -e 8 -t $N -n 10 -T $j -i $i2..."
        res=$(./test -e 8 -t $N -n 10 -T $j -i $i2 2>"$j.$i2")
        echo "$res" | grep "^:.*$"
    done
done

echo ""
echo "Batch 2..."
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        i2=$(echo "2*$i+1" | bc)
        echo -n "./test -e 8 -t $N -n 10 -T $j -i $i2..."
        res=$(./test -e 8 -t $N -n 10 -T $j -i $i2 2>"$j.$i2")
        echo "$res" | grep "^:.*$"
    done
done

echo "Verifying batch 1 (Assuming 0.0 is authoritative)..."
auth=$(cat "0.0")
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        i2=$(echo "2*$i" | bc)
        curr=$(cat "$j.$i2")
        if [ "$auth" != "$curr" ]; then
            echo "FAIL: -T=$j, -i=$i2"
            i=2
            j=4
        fi
    done
done

echo "Verifying batch 2 (Assuming 0.1 is authoritative)..."
auth=$(cat "0.1")
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        i2=$(echo "2*$i+1" | bc)
        curr=$(cat "$j.$i2")
        if [ "$auth" != "$curr" ]; then
            echo "FAIL: -T=$j, -i=$i2"
            i=2
            j=4
        fi
    done
done

echo "Removing temp files..."
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=3 ; i++ ))
    do
        rm "$j.$i"
    done
done