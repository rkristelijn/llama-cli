echo "WIP"
exit 0

#!/bin/bash
# Fetch CodeRabbit review feedback from PR #61

REPO="rkristelijn/llama-cli"
PR=61

echo "=== CodeRabbit Review Comments for PR #$PR ==="
echo ""

gh pr view "$PR" --repo "$REPO" --comments --json title,body,comments --jq '.comments[] | select(.author.login == "coderabbitai[bot]") | .body' 2>/dev/null | \
  sed -n '/## Review Summary/,/## Footer/p' | \
  sed '$d'

echo ""
echo "=== Files with issues ==="
gh pr view "$PR" --repo "$REPO" --comments --json comments --jq '.comments[] | select(.author.login == "coderabbitai[bot]") | .path' 2>/dev/null | sort -u
