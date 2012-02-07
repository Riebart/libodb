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
