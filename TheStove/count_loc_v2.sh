#!/bin/bash

# Script to count lines of code modified by a specific author since a given date
# Usage: ./count_loc_v2.sh [author-name]

SINCE_DATE="2025-10-13"
AUTHOR="${1:-$(git config user.name)}"

echo "=========================================="
echo "Lines of Code Counter"
echo "=========================================="
echo "Author: $AUTHOR"
echo "Since: $SINCE_DATE"
echo "=========================================="
echo ""

# Get list of files modified by the author since the date
echo "Fetching files modified by $AUTHOR since $SINCE_DATE..."
FILES=$(git log --author="$AUTHOR" --since="$SINCE_DATE" --name-only --pretty=format: | sort | uniq | grep -v '^$')

if [ -z "$FILES" ]; then
    echo "No files found modified by $AUTHOR since $SINCE_DATE"
    exit 0
fi

FILE_COUNT=$(echo "$FILES" | wc -l)
echo "Found $FILE_COUNT unique files modified"
echo ""

# Filter for existing files
EXISTING_FILES=""
while IFS= read -r file; do
    if [ -f "$file" ]; then
        EXISTING_FILES="$EXISTING_FILES $file"
    fi
done <<< "$FILES"

if [ -z "$EXISTING_FILES" ]; then
    echo "None of the modified files exist in the current working directory."
    echo "They may have been deleted or renamed."
else
    EXISTING_COUNT=$(echo "$EXISTING_FILES" | wc -w)
    echo "$EXISTING_COUNT files still exist in the repository"
    echo ""
    
    # Check if cloc is installed
    if command -v cloc &> /dev/null; then
        echo "Running cloc analysis on existing files..."
        echo ""
        cloc $EXISTING_FILES
    else
        echo "WARNING: 'cloc' is not installed."
        echo "Install with: sudo apt-get install cloc"
        echo ""
        echo "Basic line count (non-empty lines):"
        total_lines=0
        for file in $EXISTING_FILES; do
            lines=$(grep -v '^[[:space:]]*$' "$file" 2>/dev/null | wc -l)
            total_lines=$((total_lines + lines))
        done
        echo "Total non-empty lines in existing files: $total_lines"
    fi
fi

# Show git stats
echo ""
echo "=========================================="
echo "Git Statistics for $AUTHOR since $SINCE_DATE:"
echo "=========================================="
git log --author="$AUTHOR" --since="$SINCE_DATE" --pretty=tformat: --numstat | \
    awk '{ add += $1; subs += $2; loc += $1 - $2 } END { 
        printf "Lines added:    %s\n", add; 
        printf "Lines deleted:  %s\n", subs; 
        printf "Net lines:      %s\n", loc 
    }'

COMMIT_COUNT=$(git log --author="$AUTHOR" --since="$SINCE_DATE" --oneline | wc -l)
echo "Total commits:  $COMMIT_COUNT"
echo ""

# Show list of files if requested
if [ "$2" == "--list-files" ] || [ "$2" == "-l" ]; then
    echo "=========================================="
    echo "Files Modified:"
    echo "=========================================="
    echo "$FILES"
fi
