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

echo $1 $2 $files
