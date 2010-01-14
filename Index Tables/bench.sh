#!/bin/bash

s=0

for (( i=1 ; i<=$2 ; i++ ))
    do
    echo -n .
    s=$(echo "$s + $($1)" | bc)
done

echo "Total time: $s seconds"