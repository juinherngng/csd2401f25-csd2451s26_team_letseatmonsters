#!/bin/bash

# Script to count lines of code for ALL authors and save to individual files
# Usage: ./count_loc_all_authors.sh

SINCE_DATE="2025-12-07"
OUTPUT_DIR="loc_reports"

echo "=========================================="
echo "Lines of Code Counter - All Authors"
echo "=========================================="
echo "Since: $SINCE_DATE"
echo "Output Directory: $OUTPUT_DIR"
echo "=========================================="
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Get list of all authors since the date
echo "Fetching all authors since $SINCE_DATE..."
AUTHORS=$(git log --since="$SINCE_DATE" --format="%an" | sort | uniq)

if [ -z "$AUTHORS" ]; then
    echo "No authors found since $SINCE_DATE"
    exit 0
fi

AUTHOR_COUNT=$(echo "$AUTHORS" | wc -l)
echo "Found $AUTHOR_COUNT authors"
echo ""

# Process each author
while IFS= read -r AUTHOR; do
    echo "Processing: $AUTHOR"
    
    # Create safe filename
    SAFE_NAME=$(echo "$AUTHOR" | sed 's/[^a-zA-Z0-9]/_/g')
    OUTPUT_FILE="$OUTPUT_DIR/${SAFE_NAME}_loc_report.txt"
    
    # Generate report for this author
    {
        echo "=========================================="
        echo "Lines of Code Report"
        echo "=========================================="
        echo "Author: $AUTHOR"
        echo "Since: $SINCE_DATE"
        echo "Generated: $(date)"
        echo "=========================================="
        echo ""
        
        # Get files modified by this author
        FILES=$(git log --author="$AUTHOR" --since="$SINCE_DATE" --name-only --pretty=format: | sort | uniq | grep -v '^$')
        
        if [ -z "$FILES" ]; then
            echo "No files found for $AUTHOR"
        else
            FILE_COUNT=$(echo "$FILES" | wc -l)
            echo "Files Modified: $FILE_COUNT unique files"
            echo ""
            
            # Filter for existing files
            EXISTING_FILES=""
            while IFS= read -r file; do
                if [ -f "$file" ]; then
                    EXISTING_FILES="$EXISTING_FILES $file"
                fi
            done <<< "$FILES"
            
            if [ -n "$EXISTING_FILES" ]; then
                EXISTING_COUNT=$(echo "$EXISTING_FILES" | wc -w)
                echo "Files Still Exist: $EXISTING_COUNT files"
                echo ""
                
                # Run cloc if available
                if command -v cloc &> /dev/null; then
                    echo "=========================================="
                    echo "CODE ANALYSIS - SUMMARY"
                    echo "Excludes: Comments and Blank Lines"
                    echo "=========================================="
                    echo ""
                    cloc $EXISTING_FILES
                    echo ""
                    echo "=========================================="
                    echo "CODE ANALYSIS - PER FILE (Current State)"
                    echo "=========================================="
                    echo ""
                    cloc --by-file $EXISTING_FILES
                else
                    echo "Note: cloc not installed, skipping detailed analysis"
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
            else
                echo "None of the modified files exist in current directory"
            fi
            
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
            echo "GIT COMMIT STATISTICS - TOTAL"
            echo "=========================================="
            echo ""
            
            # Git statistics
            git log --author="$AUTHOR" --since="$SINCE_DATE" --pretty=tformat: --numstat | \
                awk '{ add += $1; subs += $2; loc += $1 - $2 } END { 
                    printf "Lines Added:    %s\n", add; 
                    printf "Lines Deleted:  %s\n", subs; 
                    printf "Net Change:     %s\n", loc 
                }'
            
            COMMIT_COUNT=$(git log --author="$AUTHOR" --since="$SINCE_DATE" --oneline | wc -l)
            echo "Total Commits:  $COMMIT_COUNT"
            
            echo ""
            echo "=========================================="
            echo "FILES MODIFIED"
            echo "=========================================="
            echo ""
            echo "$FILES"
        fi
        
    } > "$OUTPUT_FILE"
    
    echo "  → Saved to: $OUTPUT_FILE"
    
done <<< "$AUTHORS"

echo ""
echo "=========================================="
echo "Summary Report"
echo "=========================================="
echo ""

# Create summary file
SUMMARY_FILE="$OUTPUT_DIR/00_SUMMARY.txt"
{
    echo "=========================================="
    echo "SUMMARY - All Authors"
    echo "=========================================="
    echo "Since: $SINCE_DATE"
    echo "Generated: $(date)"
    echo "=========================================="
    echo ""
    
    printf "%-25s %10s %12s %12s %12s\n" "Author" "Commits" "Lines Added" "Lines Del" "Net Change"
    echo "--------------------------------------------------------------------------------"
    
    while IFS= read -r AUTHOR; do
        COMMIT_COUNT=$(git log --author="$AUTHOR" --since="$SINCE_DATE" --oneline | wc -l)
        
        STATS=$(git log --author="$AUTHOR" --since="$SINCE_DATE" --pretty=tformat: --numstat | \
            awk '{ add += $1; subs += $2; loc += $1 - $2 } END { 
                printf "%s %s %s", add, subs, loc 
            }')
        
        read -r ADDED DELETED NET <<< "$STATS"
        printf "%-25s %10s %12s %12s %12s\n" "$AUTHOR" "$COMMIT_COUNT" "$ADDED" "$DELETED" "$NET"
        
    done <<< "$AUTHORS"
    
} | tee "$SUMMARY_FILE"

echo ""
echo "=========================================="
echo "Complete!"
echo "=========================================="
echo "Reports saved to: $OUTPUT_DIR/"
echo "Summary: $SUMMARY_FILE"
echo ""
echo "Individual reports:"
ls -1 "$OUTPUT_DIR"/*.txt | grep -v "00_SUMMARY"
