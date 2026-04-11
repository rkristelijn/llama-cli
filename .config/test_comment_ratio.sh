#!/bin/sh
# Check that comment ratio in src/ meets the minimum threshold.
# Uses cloc CSV output: fields are files,language,blank,comment,code

THRESHOLD=20

totals=$(cloc src/ --exclude-dir=test --not-match-f='(_test|_it)\.cpp$' --csv --quiet | grep SUM)
comments=$(echo "$totals" | cut -d',' -f4)
code=$(echo "$totals"     | cut -d',' -f5)

total=$((comments + code))
if [ "$total" -eq 0 ]; then
    echo "ERROR: no source lines found in src/"
    exit 1
fi

ratio=$((comments * 100 / total))

echo "Comment ratio: ${comments} comments / ${total} lines = ${ratio}% (minimum: ${THRESHOLD}%)"

if [ "$ratio" -lt "$THRESHOLD" ]; then
    echo "FAIL: comment ratio ${ratio}% is below the ${THRESHOLD}% threshold"
    exit 1
fi

echo "PASS"
