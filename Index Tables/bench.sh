#!/bin/bash

s=0

#echo $3
for (( i=1 ; i<=$2 ; i++ ))
do
    echo "Running...$i of $2"
    s=$(echo "$s + $($1)" | bc)
done
echo "Total time: $s seconds"