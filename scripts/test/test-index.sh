#!/usr/bin/env bash
#
# Test llama-cli sync mode with repo indexing
# Usage: ./scripts/test/test-index.sh [model] [limit]
# Logs written to: ~/git/hub/personal/tools/index-test-logs/

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

set -euo pipefail

LLAMA="$HOME/git/hub/llama-cli/build/llama-cli"
REPO="$HOME/git/hub/personal"
LOGDIR="$REPO/tools/index-test-logs"
MODEL="${1:-gemma4:e4b}"
LIMIT="${2:-5}"
TIMEOUT=120
LOGFILE="$LOGDIR/run-$(date +%Y%m%d-%H%M)-${MODEL//[:\/]/-}.md"

mkdir -p "$LOGDIR"

# Write log header
cat > "$LOGFILE" << EOF
# Index Test Run

- **Model**: $MODEL
- **Date**: $(date)
- **Limit**: $LIMIT files
- **Timeout**: ${TIMEOUT}s

| # | File | Duration | Summary | Status |
|---|------|----------|---------|--------|
EOF

count=0
pass=0
fail=0

# Find files without frontmatter summary
find "$REPO" -type f \( -name "*.md" -o -name "*.txt" \) \
  ! -path "*/.git/*" ! -path "*/node_modules/*" ! -name "INDEX.md" \
  | sort | while read -r file; do

  # Skip files that already have a summary
  if head -1 "$file" | grep -q '^---$' && sed -n '2,/^---$/p' "$file" | grep -q '^summary:'; then
    continue
  fi

  count=$((count + 1))
  [ "$count" -gt "$LIMIT" ] && break

  relpath="${file#$REPO/}"
  content=$(head -c 4000 "$file")

  echo "⏳ [$count/$LIMIT] $relpath ($MODEL)"

  start=$(date +%s)

  summary=$(timeout "$TIMEOUT" "$LLAMA" --model="$MODEL" --timeout="$TIMEOUT" \
    "Vat samen in 1 zin (Nederlands, max 120 tekens, geen aanhalingstekens, geen markdown): $content" 2>/dev/null \
    | tr '\n' ' ' | sed 's/^ *//;s/ *$//' | head -c 80) || true

  end=$(date +%s)
  dur=$((end - start))

  if [ -n "$summary" ]; then
    echo "  ✅ ${dur}s — $summary"
    echo "| $count | $(basename "$file") | ${dur}s | $summary | ✅ |" >> "$LOGFILE"
    pass=$((pass + 1))
  else
    echo "  ❌ ${dur}s — timeout/error"
    echo "| $count | $(basename "$file") | ${dur}s | — | ❌ |" >> "$LOGFILE"
    fail=$((fail + 1))
  fi
done

# Write summary
cat >> "$LOGFILE" << EOF

## Results

- **Pass**: $pass
- **Fail**: $fail
- **Total**: $((pass + fail))
EOF

echo ""
echo "Done. Log: $LOGFILE"
