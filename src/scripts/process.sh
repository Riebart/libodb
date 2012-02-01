#!/bin/bash

#Arg1: File to process
#Arg2: Prefix for output files

#Entropy
cat <(echo -n "{") <(cat $1 | tail -n +6 | head -n -2 | grep -E "(ENTROPY|TIMESTAMP)" | sed 's/ENTROPY \(.*\)\/\(.*\)$/\1}/' | sed 's/TIMESTAMP \(.*\)$/{\1/' | tr '\n' ',' | head -c -1) <(echo -n "}") > $2.ei
cat <(echo -n "{") <(cat $1 | tail -n +6 | head -n -2 | grep -E "(ENTROPY|TIMESTAMP)" | sed 's/ENTROPY \(.*\)\/\(.*\)$/\2}/' | sed 's/TIMESTAMP \(.*\)$/{\1/' | tr '\n' ',' | head -c -1) <(echo -n "}") > $2.ev

#Ratio
cat <(echo -n "{") <(cat $1 | tail -n +6 | head -n -2 | grep -E "(RATIO|TIMESTAMP)" | sed 's/RATIO \(.*\)\/\(.*\)$/\1}/' | sed 's/TIMESTAMP \(.*\)$/{\1/' | tr '\n' ',' | head -c -1) <(echo -n "}") > $2.ri
cat <(echo -n "{") <(cat $1 | tail -n +6 | head -n -2 | grep -E "(RATIO|TIMESTAMP)" | sed 's/RATIO \(.*\)\/\(.*\)$/\2}/' | sed 's/TIMESTAMP \(.*\)$/{\1/' | tr '\n' ',' | head -c -1) <(echo -n "}") > $2.rv

#Total
cat <(echo -n "{") <(cat $1 | tail -n +6 | head -n -2 | grep -E "(TOTAL|TIMESTAMP)" | sed 's/TOTAL \(.*\)\/\(.*\)$/\1}/' | sed 's/TIMESTAMP \(.*\)$/{\1/' | tr '\n' ',' | head -c -1) <(echo -n "}") > $2.ti
cat <(echo -n "{") <(cat $1 | tail -n +6 | head -n -2 | grep -E "(TOTAL|TIMESTAMP)" | sed 's/TOTAL \(.*\)\/\(.*\)$/\2}/' | sed 's/TIMESTAMP \(.*\)$/{\1/' | tr '\n' ',' | head -c -1) <(echo -n "}") > $2.tv

#Unique
cat <(echo -n "{") <(cat $1 | tail -n +6 | head -n -2 | grep -E "(UNIQUE|TIMESTAMP)" | sed 's/UNIQUE \(.*\)\/\(.*\)$/\1}/' | sed 's/TIMESTAMP \(.*\)$/{\1/' | tr '\n' ',' | head -c -1) <(echo -n "}") > $2.ui
cat <(echo -n "{") <(cat $1 | tail -n +6 | head -n -2 | grep -E "(UNIQUE|TIMESTAMP)" | sed 's/UNIQUE \(.*\)\/\(.*\)$/\2}/' | sed 's/TIMESTAMP \(.*\)$/{\1/' | tr '\n' ',' | head -c -1) <(echo -n "}") > $2.uv
