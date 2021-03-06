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

if [ $# -eq 0 ];
then
    files=$(cat <(find ./include/*pp) <(find ./*pp))
else
    files=$(echo -n "")
    for f in "$@"
    do
        files=$(cat <(echo "$files") <(find "$f"))
    done
fi

files=$(echo -n "$files" | grep -v "^$")

echo "File processed:\n$files"

counts_nocomments=$(echo "$files" | sed 's/\(.*\)$/grep -ve "^\\s*\$" \1 | grep -cve "^ *\/\/.*$"/' | sh | tr '\n' '+' | sed 's/\(.*\).$/\1/')
counts_noblank=$(echo "$files" | sed 's/\(.*\)$/grep -cve "^\\s*\$" \1/' | sh | tr '\n' '+' | sed 's/\(.*\).$/\1/')
counts_total=$(echo "$files" | sed 's/\(.*\)$/grep -c ".*" \1/' | sh | tr '\n' '+' | sed 's/\(.*\).$/\1/')

total_nocomments=$(echo $counts_nocomments | bc)
total_noblank=$(echo $counts_noblank | bc)
total_comments=$(echo "$total_noblank - $total_nocomments" | bc)
total_total=$(echo $counts_total | bc)
total_blank=$(echo "$total_total - $total_noblank" | bc)

echo "Total lines:                    $total_total"
echo "Total non-blank lines:          $total_noblank"
echo "Total non-blank/comment lines:  $total_nocomments"
echo "Total comment lines:            $total_comments"
echo "Total blank lines:              $total_blank"
