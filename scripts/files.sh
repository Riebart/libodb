#!/bin/sh
pwd=$(pwd | sed 's/\//\\\//g')
find . -type f | grep -vE "(^\.\/\.hg|^\.\/build|spinlock|sh$)" | grep -E "(c|h|cpp|hpp)$" | sed "s/^.\(.*\)$/$pwd\1/" | tr '\n' ' '
