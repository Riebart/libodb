#!/bin/bash
# Arg1: Number of items for each test.

if [ $# -eq 0 ]; then
    N="100000"
else
    N=$1
fi

success=1

subdir=$(date | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/')
mkdir -p $subdir

echo -n "Batch 1 "
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        echo -n "."
        i2=$(echo "2*$i" | bc)
        res=$(./test -e 8 -n $N -t 10 -T $j -i $i2 2>"./$subdir/$j.$i2")
    done
done

echo -en "\t\t\t"
cat "./$subdir/0.0" | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/'

echo -en "Batch 2 (DROP_DUPLICATES) "
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        echo -n "."
        i2=$(echo "2*$i+1" | bc)
        res=$(./test -e 8 -n $N -t 10 -T $j -i $i2 2>"./$subdir/$j.$i2")
    done
done

echo -en "\t"
cat "./$subdir/0.1" | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/'

echo -en "Verifying batch 1 (Assuming 0.0 is authoritative)..."
auth=$(cat "./$subdir/0.0")
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        i2=$(echo "2*$i" | bc)
        curr=$(cat "./$subdir/$j.$i2")
        if [ "$auth" != "$curr" ]; then
            echo -n "FAIL: -T=$j, -i=$i2"
            i=2
            j=4
            success=0
        else
            echo -n "OK "
        fi
    done
done

echo -en "\nVerifying batch 2 (Assuming 0.1 is authoritative)..."
auth=$(cat "./$subdir/0.1")
for (( j=0 ; j<=3 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        i2=$(echo "2*$i+1" | bc)
        curr=$(cat "./$subdir/$j.$i2")
        if [ "$auth" != "$curr" ]; then
            echo -n "FAIL: -T=$j, -i=$i2"
            i=2
            j=4
            success=0
        else
            echo -n "OK "
        fi
    done
done

if [ $success -eq 1 ]; then
    echo -e "\nRemoving temp files..."
    rm -r $subdir
else
    echo "Tests failed. Leaving temp files for analysis."
fi