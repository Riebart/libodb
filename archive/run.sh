#!/bin/bash
# Arg1: filename
# Arg2: fully qualified path
# Arg3: Number of files to process

if [ $# -ne 3 ]; then
    echo "Usage: run.sh <program name> <full path to files> <number of files>"
    exit 1
fi

sedLoc=$(echo -n $2 |  sed 's/\//\\\//g')
files=$(ls -1 $2 | tr "\n" "\ " | sed 's/\(.*\).$/\1/g' | sed "s/ / $sedLoc/g" | sed "s/\(.*\)$/$sedLoc\1/g")
echo $1 $3 $files | sh