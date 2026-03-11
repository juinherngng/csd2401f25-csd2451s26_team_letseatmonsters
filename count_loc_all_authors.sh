#!/bin/bash

# Script to count DIFF-BASED "real code" lines added/deleted per author since a given date.
# "Real code" = excludes blank lines and common comment-only lines.
# Usage: ./count_loc_all_authors.sh
#
# Notes:
# - This measures contribution (what was added/deleted in commits), not current file size.
# - It uses git patches (-p) and filters lines, similar to count_loc.sh.

SINCE_DATE="2026-02-16"
OUTPUT_DIR="loc_reports"

echo "=========================================="
echo "Lines of Code Counter - All Authors"
echo "=========================================="
echo "Since: $SINCE_DATE"
echo "Output Directory: $OUTPUT_DIR"
echo "=========================================="
echo ""

mkdir -p "$OUTPUT_DIR"

echo "Fetching all authors since $SINCE_DATE..."
AUTHORS=$(git log --since="$SINCE_DATE" --format="%an" | sort | uniq)

if [ -z "$AUTHORS" ]; then
    echo "No authors found since $SINCE_DATE"
    exit 0
fi

AUTHOR_COUNT=$(echo "$AUTHORS" | wc -l)
echo "Found $AUTHOR_COUNT authors"
echo ""

# Helper: produce per-file code-only added/deleted/net for one author
# Filters to C/C++ by default (.cpp/.hpp/.h/.c). Adjust extensions if needed.
code_only_stats_for_author() {
    local author="$1"

    git log --author="$author" --since="$SINCE_DATE" -p --no-color \
    | LC_ALL=C awk '
    function is_code_line(s) {
        if (s ~ /^[ \t]*$/) return 0;          # blank
        if (s ~ /^[ \t]*\/\//) return 0;       # //
        if (s ~ /^[ \t]*\/\*/) return 0;       # /*
        if (s ~ /^[ \t]*\*/)  return 0;        #  *
        if (s ~ /^[ \t]*\*\/[ \t]*$/) return 0;
        return 1;                               # everything else counts
    }

    # start of a new file diff
    /^diff --git/ {
        file = $4;                 # b/path
        sub("^b/", "", file);

        # Track only source/header by default
        if (file ~ /\.cpp$/ || file ~ /\.hpp$/ || file ~ /\.h$/ || file ~ /\.c$/) {
            currentFile = file;
        } else {
            currentFile = "";
        }
        next;
    }

    # ignore binary diffs and metadata
    /^Binary files / { next }
    /^\+\+\+/ { next }
    /^\-\-\-/ { next }
    /^@@/ { next }

    # if this diff is for a non-tracked file, skip
    currentFile == "" { next }

    # added lines
    /^\+/ {
        line = substr($0, 2);
        if (is_code_line(line)) added[currentFile]++;
        next;
    }

    # deleted lines
    /^\-/ {
        line = substr($0, 2);
        if (is_code_line(line)) deleted[currentFile]++;
        next;
    }

    END {
        # Print per file: File Added Deleted Net
        for (f in added) {
            a = added[f] + 0;
            d = deleted[f] + 0;
            printf "%s\t%d\t%d\t%d\n", f, a, d, a - d;
        }
        # Also include files with only deletions (no additions)
        for (f in deleted) {
            if (!(f in added)) {
                a = 0;
                d = deleted[f] + 0;
                printf "%s\t%d\t%d\t%d\n", f, a, d, a - d;
            }
        }
    }'
}

# Process each author
while IFS= read -r AUTHOR; do
    echo "Processing: $AUTHOR"

    SAFE_NAME=$(echo "$AUTHOR" | sed 's/[^a-zA-Z0-9]/_/g')
    OUTPUT_FILE="$OUTPUT_DIR/${SAFE_NAME}_loc_report.txt"

    {
        echo "=========================================="
        echo "Lines of Code Report (Diff-based)"
        echo "=========================================="
        echo "Author: $AUTHOR"
        echo "Since: $SINCE_DATE"
        echo "Generated: $(date)"
        echo "=========================================="
        echo ""

        echo "=========================================="
        echo "ACTUAL CODE CHANGES PER FILE (NO BLANKS/COMMENTS)"
        echo "=========================================="
        echo "Tracked extensions: .cpp .hpp .h .c"
        echo ""
        printf "%-70s %10s %10s %10s\n" "File" "Added" "Deleted" "Net"
        echo "--------------------------------------------------------------------------------------------"

        # Collect + sort by Net desc
        STATS=$(code_only_stats_for_author "$AUTHOR")
        if [ -z "$STATS" ]; then
            echo "No matching code diffs for $AUTHOR (in tracked file extensions)."
        else
            echo "$STATS" \
            | awk -F'\t' '{ printf "%-70s %10d %10d %10d\n", $1, $2, $3, $4 }' \
            | sort -k1,1V
        fi

        echo "--------------------------------------------------------------------------------------------"
        echo ""

        echo "=========================================="
        echo "ACTUAL CODE CHANGES - TOTAL (NO BLANKS/COMMENTS)"
        echo "=========================================="
        if [ -z "$STATS" ]; then
            echo "Lines Added:    0"
            echo "Lines Deleted:  0"
            echo "Net Change:     0"
        else
            echo "$STATS" | awk -F'\t' '
                { add += $2; del += $3; net += $4 }
                END {
                    printf "Lines Added:    %d\n", add;
                    printf "Lines Deleted:  %d\n", del;
                    printf "Net Change:     %d\n", net;
                }'
        fi

        COMMIT_COUNT=$(git log --author="$AUTHOR" --since="$SINCE_DATE" --oneline | wc -l)
        echo "Total Commits:  $COMMIT_COUNT"
        echo ""

        echo "=========================================="
        echo "FILES MODIFIED (RAW - ANY TYPE)"
        echo "=========================================="
        echo ""
        git log --author="$AUTHOR" --since="$SINCE_DATE" --name-only --pretty=format: \
            | sort | uniq | grep -v '^$'

    } > "$OUTPUT_FILE"

    echo "  → Saved to: $OUTPUT_FILE"
done <<< "$AUTHORS"

echo ""
echo "=========================================="
echo "Summary Report (Diff-based Code Only)"
echo "=========================================="
echo ""

SUMMARY_FILE="$OUTPUT_DIR/00_SUMMARY.txt"
{
    echo "=========================================="
    echo "SUMMARY - All Authors (Diff-based Code Only)"
    echo "=========================================="
    echo "Since: $SINCE_DATE"
    echo "Generated: $(date)"
    echo "=========================================="
    echo ""

    printf "%-25s %10s %12s %12s %12s\n" "Author" "Commits" "Lines Added" "Lines Del" "Net Change"
    echo "--------------------------------------------------------------------------------"

    while IFS= read -r AUTHOR; do
        COMMIT_COUNT=$(git log --author="$AUTHOR" --since="$SINCE_DATE" --oneline | wc -l)

        STATS=$(code_only_stats_for_author "$AUTHOR")
        if [ -z "$STATS" ]; then
            ADDED=0; DELETED=0; NET=0
        else
            read -r ADDED DELETED NET <<< "$(echo "$STATS" | awk -F'\t' '
                { add += $2; del += $3; net += $4 }
                END { printf "%d %d %d", add, del, net }')"
        fi

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
