#!/bin/bash
counts_nocomments=$(cat <(find ./archive/*pp) <(find ./include/*pp) <(find ./*pp) <(find ./archive/*.c) <(find ./archive/*.h) | sed 's/\(.*\)$/grep -ve "^\\s*\$" \1 | grep -cve "^ *\/\/.*$"/' | sh | tr '\n' '+' | sed 's/\(.*\).$/\1/')
counts_noblank=$(cat <(find ./archive/*pp) <(find ./include/*pp) <(find ./*pp) <(find ./archive/*.c) <(find ./archive/*.h) | sed 's/\(.*\)$/grep -cve "^\\s*\$" \1/' | sh | tr '\n' '+' | sed 's/\(.*\).$/\1/')
counts_total=$(cat <(find ./archive/*pp) <(find ./include/*pp) <(find ./*pp) <(find ./archive/*.c) <(find ./archive/*.h) | sed 's/\(.*\)$/grep -c ".*" \1/' | sh | tr '\n' '+' | sed 's/\(.*\).$/\1/')

total_nocomments=$(echo $counts_nocomments | bc)
total_noblank=$(echo $counts_noblank | bc)
total_comments=$(echo "$total_noblank - $total_nocomments" | bc)
total_total=$(echo $counts_total | bc)
total_blank=$(echo "$total_total - $total_noblank" | bc)
files=$(cat <(find ./archive/*pp) <(find ./include/*pp) <(find ./*pp) <(find ./archive/*.c) <(find ./archive/*.h))

if [ $# -eq 1 ]; then
    echo "File processed:\n$files"
fi

echo "Total lines:                    $total_total"
echo "Total non-blank lines:          $total_noblank"
echo "Total non-blank/comment lines:  $total_nocomments"
echo "Total comment lines:            $total_comments"
echo "Total blank lines:              $total_blank"