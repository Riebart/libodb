#!/bin/bash
# Arg1: Number of items for each test.

if [ $# -eq 0 ]; then
    N="100000"
    proto_hash1="43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c"
    proto_hash2="bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0"
else
    N=$1
    proto_hash1=""
    proto_hash2=""
fi

success=1

subdir=$(date | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/')
mkdir -p $subdir

echo "Command in use: ./test -e 8 -n $N -t 10 -T ?? -i ??"
echo -n "Batch 1 "
for (( j=0 ; j<=1 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        echo -n "."
        i2=$(echo "2*$i" | bc)
        res=$(./test -e 8 -n $N -t 10 -T $j -i $i2 2>"./$subdir/$j.$i2")
    done
done

hash1=$(cat "./$subdir/0.0" | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/')

if [ "$proto_hash1" != "" ]; then
    if [ "$hash1" != "$proto_hash1" ]; then
        echo " FAIL"
        success=0
    else
        echo " pass"
    fi
else
    echo -en "\t\t\t"
    echo -n $hash1
fi

echo -en "Verifying batch 1 (Assuming 0.0 is authoritative)..."
auth=$(cat "./$subdir/0.0")
for (( j=0 ; j<=1 ; j++ ))
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
            echo -n "ok "
        fi
    done
done

echo -en "\nBatch 2 (DROP_DUPLICATES) "
for (( j=0 ; j<=1 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        echo -n "."
        i2=$(echo "2*$i+1" | bc)
        res=$(./test -e 8 -n $N -t 10 -T $j -i $i2 2>"./$subdir/$j.$i2")
    done
done

echo -en "\t"
hash2=$(cat "./$subdir/0.1" | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/')

if [ "$proto_hash2" != "" ]; then
    if [ "$hash2" != "$proto_hash2" ]; then
        echo " FAIL"
        success=0
    else
        echo " pass"
    fi
else
    echo -n $hash2
fi

echo -en "Verifying batch 2 (Assuming 0.1 is authoritative)..."
auth=$(cat "./$subdir/0.1")
for (( j=0 ; j<=1 ; j++ ))
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
            echo -n "ok "
        fi
    done
done

if [ $success -eq 1 ]; then
    echo -e "\nRemoving temp files..."
    rm -r $subdir
else
    echo -e "\nTests failed. Leaving temp files for analysis."
fi