#!/bin/bash
# Convert llvm-cov show output to traditional gcov format
# Usage: llvm2gcov.sh <source_file>

if [ $# -ne 1 ]; then
    echo "Usage: $0 <source_file>"
    exit 1
fi

SOURCE_FILE="$1"
BASENAME=$(basename "$SOURCE_FILE" .c)
GCOV_FILE="${BASENAME}.c.gcov"

if [ ! -f ../coverage/merged.profdata ]; then
    echo "Error: ../coverage/merged.profdata not found. Run 'make coverage' first."
    exit 1
fi

# Generate gcov-style header
echo "        -:    0:Source:$SOURCE_FILE"
echo "        -:    0:Graph:dummy.gcno"  
echo "        -:    0:Data:dummy.gcda"
echo "        -:    0:Runs:1"
echo "        -:    0:Programs:1"

# Convert llvm-cov output to gcov format
# Format conversion: "line|count|source" -> "count:line:source"
xcrun llvm-cov show --instr-profile=../coverage/merged.profdata c-xref \
    --format=text --sources "$SOURCE_FILE" --show-line-counts 2>/dev/null | \
    awk -F'|' '{
        line = $1
        count = $2  
        source = $3
        
        # Remove leading/trailing whitespace from line number
        gsub(/^[ \t]+|[ \t]+$/, "", line)
        
        # Handle different count formats
        if (count ~ /^[ \t]*$/) {
            # Empty count - non-executable line
            printf "%9s:%5s:%s\n", "-", line, source
        } else {
            # Remove leading/trailing whitespace from count
            gsub(/^[ \t]+|[ \t]+$/, "", count)
            # Format numeric count or handle special cases
            if (count == "0" || count ~ /^[0-9]+$/) {
                printf "%9s:%5s:%s\n", count, line, source
            } else if (count ~ /[0-9.]+k$/) {
                # Convert 4.18k to 4180
                gsub(/k$/, "", count)
                printf "%9d:%5s:%s\n", int(count * 1000), line, source
            } else {
                # Fallback for other formats
                printf "%9s:%5s:%s\n", count, line, source
            }
        }
    }'