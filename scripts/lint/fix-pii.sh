#!/usr/bin/env bash
#
# fix-pii.sh — Auto-obfuscate PII patterns from .pii file

set -o errexit
set -o nounset
set -o pipefail

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi

PII_FILE="${PII_FILE:-}"

# Check for .pii in .config/ first, then root
if [[ -z "$PII_FILE" ]]; then
  if [[ -f ".config/.pii" ]]; then
    PII_FILE=".config/.pii"
  elif [[ -f ".pii" ]]; then
    PII_FILE=".pii"
  fi
fi

if [[ -z "$PII_FILE" || ! -f "$PII_FILE" ]]; then
  echo "==> No .pii file found"
  echo "    Run check-pii.sh first to create template"
  exit 1
fi

echo "==> Auto-obfuscating PII patterns..."
echo "    WARNING: This will modify files. Review changes carefully!"
read -p "    Continue? [y/N] " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
  echo "  Aborted"
  exit 0
fi

FIXED=0
PATTERNS=()

# Read patterns from .pii file
while IFS= read -r line; do
  [[ "$line" =~ ^#.*$ ]] && continue
  [[ -z "$line" ]] && continue
  PATTERNS+=("$line")
done <"$PII_FILE"

if [[ ${#PATTERNS[@]} -eq 0 ]]; then
  echo "  [skip] No PII patterns defined in $PII_FILE"
  exit 0
fi

echo "  Obfuscating ${#PATTERNS[@]} pattern(s)..."

# Determine placeholder based on pattern
get_placeholder() {
  local pattern="$1"

  # Email
  if [[ "$pattern" =~ @.*\. ]]; then
    echo "<email>"
    return
  fi

  # IP address
  if [[ "$pattern" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "<ip-address>"
    return
  fi

  # UUID
  if [[ "$pattern" =~ ^[0-9a-f]{8}-[0-9a-f]{4} ]]; then
    echo "<uuid>"
    return
  fi

  # Long hex (token)
  if [[ "$pattern" =~ ^[0-9a-f]{32,}$ ]]; then
    echo "<token>"
    return
  fi

  # Credit card (16 digits)
  if [[ "$pattern" =~ ^[0-9]{16}$ ]]; then
    echo "<credit-card>"
    return
  fi

  # BSN (9 digits)
  if [[ "$pattern" =~ ^[0-9]{9}$ ]]; then
    echo "<bsn>"
    return
  fi

  # IBAN
  if [[ "$pattern" =~ ^[A-Z]{2}[0-9]{2} ]]; then
    echo "<iban>"
    return
  fi

  # Default: hostname
  echo "<hostname>"
}

# Fix files
for pattern in "${PATTERNS[@]}"; do
  placeholder=$(get_placeholder "$pattern")

  for file in src/**/*.cpp src/**/*.h src/**/*.sh docs/**/*.md scripts/**/*.sh; do
    [[ -f "$file" ]] || continue
    [[ "$file" == "scripts/lint/check-pii.sh" ]] && continue
    [[ "$file" == "scripts/lint/fix-pii.sh" ]] && continue

    if grep -q "$pattern" "$file" 2>/dev/null; then
      sed -i.bak "s/$pattern/$placeholder/g" "$file"
      if ! diff -q "$file" "$file.bak" >/dev/null 2>&1; then
        echo "  [fixed] $file: $pattern → $placeholder"
        FIXED=$((FIXED + 1))
      fi
      rm -f "$file.bak"
    fi
  done
done

echo ""
echo "  Fixed $FIXED file(s)"
echo "  Review with: git diff"
echo "  Revert with: git checkout ."
