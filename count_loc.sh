#!/bin/bash

# Script to count lines of code modified by a specific author since a given date
# Usage: ./count_loc.sh [author-name] [--list-files|-l]

SINCE_DATE="2025-12-07"
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
        echo "=========================================="
        echo "CODE ANALYSIS - SUMMARY"
        echo "=========================================="
        echo "This analyzes the CURRENT state of files"
        echo "(excludes comments & blank lines)"
        echo ""
        cloc $EXISTING_FILES
        
        echo ""
        echo "=========================================="
        echo "CODE ANALYSIS - PER FILE (Current State)"
        echo "=========================================="
        echo ""
        cloc --by-file $EXISTING_FILES
    else
        echo "WARNING: 'cloc' is not installed."
        echo "Install with: sudo apt-get install cloc"
        echo ""
        echo "Basic line count per file (non-empty lines):"
        echo ""
        printf "%-60s %10s\n" "File" "Lines"
        echo "--------------------------------------------------------------------------------"
        for file in $EXISTING_FILES; do
            lines=$(grep -v '^[[:space:]]*$' "$file" 2>/dev/null | wc -l)
            printf "%-60s %10s\n" "$file" "$lines"
        done
        echo "--------------------------------------------------------------------------------"
        total_lines=0
        for file in $EXISTING_FILES; do
            lines=$(grep -v '^[[:space:]]*$' "$file" 2>/dev/null | wc -l)
            total_lines=$((total_lines + lines))
        done
        echo "Total non-empty lines: $total_lines"
    fi
fi

# Show git changes per file
echo ""
echo "=========================================="
echo "GIT CHANGES PER FILE"
echo "=========================================="
echo ""
printf "%-70s %10s %10s %10s\n" "File" "Added" "Deleted" "Net"
echo "--------------------------------------------------------------------------------------------"

# Get git stats per file
git log --author="$AUTHOR" --since="$SINCE_DATE" --pretty=format: --numstat | \
    awk '{file[$3]+=$1; file_del[$3]+=$2} END {for (f in file) printf "%-70s %10s %10s %10s\n", f, file[f], file_del[f], file[f]-file_del[f]}' | \
    sort -t' ' -k4 -nr

echo "--------------------------------------------------------------------------------------------"

echo ""
echo "=========================================="
echo "GIT CHANGES PER FILE (CODE ONLY - NO COMMENTS)"
echo "=========================================="
echo ""
printf "%-70s %10s %10s %10s\n" "File" "Added" "Deleted" "Net"
echo "--------------------------------------------------------------------------------------------"

git log --author="$AUTHOR" --since="$SINCE_DATE" -p --no-color \
| LC_ALL=C awk '
function is_code_line(s, t) {
    # blank or whitespace-only -> ignore
    if (s ~ /^[ \t]*$/) return 0;

    # full-line comments -> ignore
    if (s ~ /^[ \t]*\/\//) return 0;   # //
    if (s ~ /^[ \t]*\/\*/) return 0;   # /*
    if (s ~ /^[ \t]*\*/)  return 0;    #  *
    if (s ~ /^[ \t]*\*\/[ \t]*$/) return 0; # */

    # lines that are only braces + spaces -> ignore
    t = s;
    gsub(/[ \t{}]/, "", t);
    if (t == "") return 0;

    # require some "code-ish" characters
    if (s ~ /[A-Za-z0-9_]/) return 1;                # identifiers, numbers
    if (s ~ /[+\-*/%<>=!&|^~]/) return 1;            # operators
    if (s ~ /[;:,]/) return 1;                       # statement separators
    return 0;
}

# start of a new file diff
/^diff --git/ {
    file = $3;                 # b/path
    sub("^b/", "", file);

    # only track .cpp and .hpp files
    if (file ~ /\.cpp$/ || file ~ /\.hpp$/) {
        currentFile = file;
    } else {
        currentFile = "";
    }
    next;
}

# ignore binary diffs and hunk headers
/^Binary files / { next }
/^\+\+\+/ { next }
/^\-\-\-/ { next }

# if this diff is for a non-code file, skip everything
currentFile == "" { next }

# added lines
/^\+/ {
    line = substr($0, 2);
    if (is_code_line(line)) {
        added[currentFile]++;
    }
    next;
}

# deleted lines
/^\-/ {
    line = substr($0, 2);
    if (is_code_line(line)) {
        deleted[currentFile]++;
    }
    next;
}

END {
    for (f in added) {
        printf "%-70s %10d %10d %10d\n",
               f, added[f], deleted[f], added[f] - deleted[f];
    }
}
' | sort -k4 -nr

echo "--------------------------------------------------------------------------------------------"

# Show git stats
echo ""
echo "=========================================="
echo "Git Statistics for $AUTHOR since $SINCE_DATE:"
echo "=========================================="
echo "(This shows actual changes made in commits)"
echo ""
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
