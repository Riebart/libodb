#!/bin/bash

# MPL2.0 HEADER START
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# MPL2.0 HEADER END
#
# Copyright 2010-2012 Michael Himbeault and Travis Friesen
#

# Arg1: Number of items for each test.

if [ $# -eq 0 ]; then
    N="100000"
    proto_hash11="43031308a1919ec69606810188462336ceab795a057a987a2ea3112898ff5c2c"
    proto_hash12="5f95f2dd442d9ed865ff201f7e07b5a224cab688cdd506dc617f06616f8b9369"
    proto_hash21="bbfd379491b29ba724f4a2163972bf827c83ff7bc14a5a49058fc39d23d658e0"
    proto_hash22="6d81f80fa65fe4873024a1df6a066c1d633421b06a38c2b29c299800a3967fb7"
else
    N=$1
    proto_hash11=""
    proto_hash12=""
    proto_hash21=""
    proto_hash12=""
fi

success=1

subdir=$(date | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/')

out="false"

if [ "$out" != "false" ]; then
    mkdir -p $subdir
    echo "Writing output to: $subdir"
fi

echo "Command in use: ./test-ind_ds -e 8 -n $N -t 10 -T ?? -i ??"
echo -n "Batch 1 "
for (( j=0 ; j<=1 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        echo -n "."
        i2=$[ 2 * $i ]
        
        if [ "$out" != "false" ]; then
            out="$subdir/$j.$i2"
        fi
        res=$(./test-ind_ds -e 8 -n $N -t 10 -T $j -i $i2 2>&1 1>/dev/null | tee $out | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/')
        
        if [ "$proto_hash11" != "" ]; then
            if [ "$res" != "$proto_hash11" ]; then
                echo -n "FAIL "
                success=0
            else
                echo -n "pass "
            fi
        else
            echo -en "\t\t\t"
            echo -n $res
        fi
    done
    
    echo -n "."
    j2=$[ j * 2 ]
    
    if [ "$out" != "false" ]; then
        out="$subdir/4.$j2"
    fi
    res=$(./test-ind_ds -e 8 -n $N -t 10 -T 4 -i $j2 2>&1 1>/dev/null | tee $out | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/')
    
    if [ "$proto_hash12" != "" ]; then
        if [ "$res" != "$proto_hash12" ]; then
            echo -n "FAIL "
            success=0
        else
            echo -n "pass "
        fi
    else
        echo -en "\t\t\t"
        echo -n $res
    fi
done

echo -en "\nBatch 2 (DROP_DUPLICATES) "
for (( j=0 ; j<=1 ; j++ ))
do
    for (( i=0 ; i<=1 ; i++ ))
    do
        echo -n "."
        i2=$[ 2 * $i + 1 ]
        
        if [ "$out" != "false" ]; then
            out="$subdir/$j.$i2"
        fi
        res=$(./test-ind_ds -e 8 -n $N -t 10 -T $j -i $i2 2>&1 1>/dev/null | tee $out | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/')
        
        if [ "$proto_hash21" != "" ]; then
            if [ "$res" != "$proto_hash21" ]; then
                echo -n "FAIL "
                success=0
            else
                echo -n "pass "
            fi
        else
            echo -en "\t\t\t"
            echo -n $res
        fi
    done
    
    echo -n "."
    j2=$[ 2 * $j + 1 ]
    
    if [ "$out" != "false" ]; then
        out="$subdir/4.$j2"
    fi
    res=$(./test-ind_ds -e 8 -n $N -t 10 -T 4 -i $j2 2>&1 1>/dev/null | tee $out | sha256sum | sed 's/^\([0-9,a-f]*\).*$/\1/')
    
    if [ "$proto_hash22" != "" ]; then
        if [ "$res" != "$proto_hash22" ]; then
            echo -n "FAIL "
            success=0
        else
            echo -n "pass "
        fi
    else
        echo -en "\t\t\t"
        echo -n $res
    fi
done

if [ "$out" != "false" ]; then
    if [ $success -eq 1 ]; then
        echo -e "\nRemoving temp files..."
    #    rm -r $subdir
    else
        echo -e "\nTests failed. Leaving temp files for analysis."
    fi
fi

echo
