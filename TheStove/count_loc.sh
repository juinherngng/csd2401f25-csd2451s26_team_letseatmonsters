#!/bin/bash

# Script to count lines of code modified by a specific author since a given date
# Usage: ./count_loc.sh [author-name]
# Example: ./count_loc.sh "John Doe"

# Configuration
SINCE_DATE="2025-12-07"
AUTHOR="${1:-$(git config user.name)}"  # Use provided author or current git user

echo "=========================================="
echo "Lines of Code Counter"
echo "=========================================="
echo "Author: $AUTHOR"
echo "Since: $SINCE_DATE"
echo "=========================================="
echo ""

# Create a temporary directory for modified files
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# Get list of files modified by the author since the date
echo "Fetching files modified by $AUTHOR since $SINCE_DATE..."
FILES=$(git log --author="$AUTHOR" --since="$SINCE_DATE" --name-only --pretty=format: | sort | uniq | grep -v '^$')

if [ -z "$FILES" ]; then
    echo "No files found modified by $AUTHOR since $SINCE_DATE"
    exit 0
fi

echo "Found $(echo "$FILES" | wc -l) unique files"
echo ""

# Copy current versions of modified files to temp directory
echo "Copying files for analysis..."
while IFS= read -r file; do
    if [ -f "$file" ]; then
        mkdir -p "$TEMP_DIR/$(dirname "$file")"
        cp "$file" "$TEMP_DIR/$file" 2>/dev/null
    fi
done <<< "$FILES"

# Check if cloc is installed
if ! command -v cloc &> /dev/null; then
    echo "WARNING: 'cloc' is not installed."
    echo "You can install it with: sudo apt-get install cloc"
    echo ""
    echo "For now, using a basic line count (excluding empty lines):"
    echo ""
    
    total_lines=0
    while IFS= read -r file; do
        if [ -f "$file" ]; then
            lines=$(grep -v '^[[:space:]]*$' "$file" 2>/dev/null | wc -l)
            total_lines=$((total_lines + lines))
        fi
    done <<< "$FILES"
    
    echo "Total non-empty lines: $total_lines"
else
    # Use cloc to count lines of code (excluding comments, blank lines)
    echo "Running cloc analysis..."
    echo ""
    cloc "$TEMP_DIR" --by-file-by-lang
    echo ""
    echo "=========================================="
    echo "Summary:"
    echo "=========================================="
    cloc "$TEMP_DIR"
fi

# Show git stats for the author
echo ""
echo "=========================================="
echo "Git Statistics for $AUTHOR since $SINCE_DATE:"
echo "=========================================="
git log --author="$AUTHOR" --since="$SINCE_DATE" --pretty=tformat: --numstat | \
    awk '{ add += $1; subs += $2; loc += $1 - $2 } END { 
        printf "Lines added: %s\n", add; 
        printf "Lines deleted: %s\n", subs; 
        printf "Net lines: %s\n", loc 
    }'

echo ""
echo "Commits by $AUTHOR since $SINCE_DATE: $(git log --author="$AUTHOR" --since="$SINCE_DATE" --oneline | wc -l)"
