#!/usr/bin/env bash
#
# check-error-handling.sh — Detect common error handling gaps in C++ source.
#
# Checks:
#   1. return "" without LOG_EVENT or tui::error within 5 lines above
#   2. popen() without null check on the next 2 lines
#   3. std::stoi/stol/stof without try-catch in surrounding context
#
# Suppress false positives: add "// error-handling:ok" on the line.
#
# Usage: bash scripts/lint/check-error-handling.sh [--verbose]

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

VERBOSE=0
if [[ "${1-}" == "--verbose" ]]; then VERBOSE=1; fi

SRC_DIR="src"
ERRFILE=$(mktemp)
trap 'rm -f "$ERRFILE"' EXIT
echo 0 > "$ERRFILE"

# Increment error count (file-based to survive subshells)
bump() { echo $(( $(cat "$ERRFILE") + 1 )) > "$ERRFILE"; }

# Collect source files (skip test, integration, and fuzz files)
FILES=$(find "$SRC_DIR" -name '*.cpp' -o -name '*.h' \
  | grep -vE '_test\.|_it\.|fuzz_|test_helpers' | sort)

echo "==> checking error handling..."

for file in $FILES; do
  line_num=0

  while IFS= read -r line; do
    line_num=$((line_num + 1))

    # Skip suppressed lines
    case "$line" in *error-handling:ok*) continue ;; esac

    # --- Check 1: return "" without nearby error logging ---
    if echo "$line" | grep -qE 'return\s+""|return\s+std::string\(\)'; then
      start=$((line_num - 5))
      if [ "$start" -lt 1 ]; then start=1; fi
      context=$(sed -n "${start},${line_num}p" "$file")
      if ! echo "$context" | grep -qE 'LOG_EVENT|tui::error|std::cerr|error-handling:ok'; then
        echo "  $file:$line_num: return \"\" without error logging"
        if [ "$VERBOSE" -eq 1 ]; then
          sed -n "${start},${line_num}p" "$file" | sed 's/^/    | /'
          echo ""
        fi
        bump
      fi
    fi

    # --- Check 2: popen without null check ---
    if echo "$line" | grep -qE 'popen\('; then
      end=$((line_num + 2))
      context=$(sed -n "${line_num},${end}p" "$file")
      if ! echo "$context" | grep -qE 'if\s*\(|nullptr|NULL|error-handling:ok'; then
        echo "  $file:$line_num: popen() without null check"
        if [ "$VERBOSE" -eq 1 ]; then
          sed -n "${line_num},${end}p" "$file" | sed 's/^/    | /'
          echo ""
        fi
        bump
      fi
    fi

    # --- Check 3: stoi/stol/stof without try-catch ---
    if echo "$line" | grep -qE 'std::sto[ifl]\('; then
      start=$((line_num - 10))
      if [ "$start" -lt 1 ]; then start=1; fi
      end=$((line_num + 10))
      context=$(sed -n "${start},${end}p" "$file")
      if ! echo "$context" | grep -qE 'try|catch|error-handling:ok'; then
        echo "  $file:$line_num: std::stoi/stol/stof without try-catch"
        if [ "$VERBOSE" -eq 1 ]; then
          echo "    | $(sed -n "${line_num}p" "$file")"
          echo ""
        fi
        bump
      fi
    fi

  done < "$file"
done

ERRORS=$(cat "$ERRFILE")

if [ "$ERRORS" -gt 0 ]; then
  echo ""
  echo "FAIL: $ERRORS error handling issue(s) found"
  echo ""
  echo "Fix by adding LOG_EVENT/tui::error before return \"\","
  echo "checking popen() return values, or wrapping stoi in try-catch."
  echo "Suppress false positives with: // error-handling:ok"
  exit 1
fi

echo "  [done] check-error-handling (0 issues)"
