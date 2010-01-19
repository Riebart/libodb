#!/bin/bash

s=0
M=0
m=100

for (( i=1 ; i<=$2 ; i++ ))
do
    echo -n .
    t[$i]=$($1)
    s=$(echo "$s + ${t[$i]}" | bc)
    
    c=$(echo "${t[$i]} > $M" | bc)
    if [ $c -eq 1 ]
    then
        M=${t[$i]}
    fi
    
    c=$(echo "${t[$i]} < $m" | bc)
    if [ $c -eq 1 ]
    then
        m=${t[$i]}
    fi
done

s=$(echo "scale = 8; $s / $2" | bc)
d=$(echo "$M - $m" | bc)

echo "Average time per run is $s seconds with a spread of $d seconds"