#!/usr/bin/env bash
#
# check-pii.sh — Detect PII (Personally Identifiable Information) in code

set -o errexit
set -o nounset
set -o pipefail

PII_FILE="${PII_FILE:-}"
INTERACTIVE="${INTERACTIVE:-true}"

# Check for .pii in .config/ first, then root
if [[ -z "$PII_FILE" ]]; then
  if [[ -f ".config/.pii" ]]; then
    PII_FILE=".config/.pii"
  elif [[ -f ".pii" ]]; then
    PII_FILE=".pii"
  fi
fi

if [[ -z "$PII_FILE" || ! -f "$PII_FILE" ]]; then
  cat >".config/.pii" <<'EOF'
# PII patterns to detect in code (one per line)
# Examples:
# apsnl
# john.doe@company.com
# 192.168.1.100
# my-secret-hostname

EOF
  echo "==> Created .config/.pii template"
  echo "    Add your PII patterns (one per line) and re-run"
  echo "    Note: .config/.pii is in .gitignore - each developer maintains their own"
  exit 0
fi

echo "==> Checking for PII in code..."

FOUND=0
PATTERNS=()

# Read patterns from .pii file
while IFS= read -r line; do
  # Skip comments and empty lines
  [[ "$line" =~ ^#.*$ ]] && continue
  [[ -z "$line" ]] && continue
  PATTERNS+=("$line")
done <"$PII_FILE"

if [[ ${#PATTERNS[@]} -eq 0 ]]; then
  echo "  [skip] No PII patterns defined in $PII_FILE"
  exit 0
fi

echo "  Scanning for ${#PATTERNS[@]} PII pattern(s)..."

# Scan source files (exclude the check script itself)
for pattern in "${PATTERNS[@]}"; do
  # Exact match
  if grep -r --include="*.cpp" --include="*.h" --include="*.sh" \
    --exclude="check-pii.sh" \
    -n "$pattern" src/ scripts/ docs/ 2>/dev/null | grep -v "^Binary"; then
    echo "  [FAIL] Found PII pattern: $pattern"
    FOUND=1
  fi
done

if [[ $FOUND -eq 1 ]]; then
  echo ""
  echo "  [FAIL] PII detected in code"
  echo "  Remove sensitive data and use placeholders like <hostname>, <email>"
  exit 1
fi

echo "  [pass] No PII detected"
