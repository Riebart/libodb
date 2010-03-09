#!/bin/bash
# Arg1: filename
# Arg2: Number of files to process
# Arg3+: fully qualified path

if [ $# -le 2 ]; then
    echo "Usage: run.sh <program name> <number of files> <full path to files>+"
    exit 1
fi

files=""

i=1
for arg in "$@"
do
    if [ $i -ge 3 ]; then
        sedLoc=$(echo -n ${arg} | sed 's/\//\\\//g')
        files=$(echo $files $(ls -1 ${arg} | tr "\n" "\ " | sed 's/\(.*\).$/\1/g' | sed "s/ / $sedLoc/g" | sed "s/\(.*\)$/$sedLoc\1/g"))
    fi
    let "i += 1"
done

echo $1 $2 $files | sh
