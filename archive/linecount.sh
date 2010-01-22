#!/bin/bash
files=$(find . | grep pp$ | sed 's/\(.*\)$/grep -cve "^\\s*\$" \1/' | sh | tr '\n' '+' | sed 's/\(.*\).$/\1/')
echo $files | bc