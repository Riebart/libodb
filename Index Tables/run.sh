#!/bin/bash
# Arg1: filename
# Arg2: fully qualified path
# Arg3: Number of files to process

sedLoc=$(echo -n $2 |  sed 's/\//\\\//g')
files=$(ls -1 $2 | tr "\n" "\ " | sed 's/\(.*\).$/\1/g' | sed "s/ / $sedLoc/g" | sed "s/\(.*\)$/$sedLoc\1/g")
echo $1 $3 $files | sh > out.txt