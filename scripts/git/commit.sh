#!/usr/bin/env bash
# commit.sh — Interactive conventional commit builder (like commitizen).
#
# Usage: make commit  OR  bash scripts/git/commit.sh
#
# Prompts for: type, scope, description, body, breaking change, references.
# Builds a conventional commit message and runs git commit.
#
# @see docs/adr/adr-121-cpm-quality-layer.md

set -o errexit
set -o nounset
set -o pipefail

source lib/cpm/shell/init.sh 2>/dev/null || true

# Types with descriptions
TYPES=(
  "feat:     A new feature"
  "fix:      A bug fix"
  "refactor: Code change (no new feature, no bug fix)"
  "docs:     Documentation only"
  "chore:    Maintenance (deps, config, tooling)"
  "test:     Adding or fixing tests"
  "ci:       CI/CD changes"
  "style:    Formatting (no code change)"
  "perf:     Performance improvement"
  "build:    Build system changes"
)

# Detect scopes from git history + directory structure
detect_scopes() {
  # From recent commits
  git log --oneline -50 2>/dev/null | grep -oP '\(\K[^)]+' | sort -u
  # From src/ directories
  find src -maxdepth 1 -type d 2>/dev/null | sed 's|src/||' | grep -v '^src$'
}

echo ""
print_header "Conventional Commit"

# 1. Type
echo "  Select type:"
echo ""
for i in "${!TYPES[@]}"; do
  printf "    %2d) %s\n" $((i + 1)) "${TYPES[$i]}"
done
echo ""
printf "  Type [1-${#TYPES[@]}]: "
read -r type_num
type_num=${type_num:-1}
TYPE=$(echo "${TYPES[$((type_num - 1))]}" | cut -d: -f1 | tr -d ' ')

# 2. Scope (optional)
echo ""
echo "  Known scopes: $(detect_scopes 2>/dev/null | tr '\n' ', ' | sed 's/,$//')"
printf "  Scope (optional, enter to skip): "
read -r SCOPE

# 3. Short description
echo ""
printf "  Short description (max 72 chars): "
read -r DESC

if [[ -z "$DESC" ]]; then
  print_error "Description is required"
  exit 1
fi

# 4. Body (optional)
echo ""
printf "  Longer description (optional, enter to skip): "
read -r BODY

# 5. Breaking change?
echo ""
printf "  Breaking change? [y/N]: "
read -r BREAKING
BREAKING_PREFIX=""
[[ "$BREAKING" =~ ^[yY] ]] && BREAKING_PREFIX="!"

# 6. References (optional)
echo ""
printf "  References (e.g. #123, ADR-121, enter to skip): "
read -r REFS

# Build message
if [[ -n "$SCOPE" ]]; then
  FIRST_LINE="${TYPE}${BREAKING_PREFIX}(${SCOPE}): ${DESC}"
else
  FIRST_LINE="${TYPE}${BREAKING_PREFIX}: ${DESC}"
fi

# Build full message
MSG="$FIRST_LINE"
[[ -n "$BODY" ]] && MSG="$MSG

$BODY"
[[ -n "$REFS" ]] && MSG="$MSG

Refs: $REFS"
[[ "$BREAKING" =~ ^[yY] ]] && MSG="$MSG

BREAKING CHANGE: $DESC"

# Preview
echo ""
echo "  ─────────────────────────────────────"
echo "  $FIRST_LINE"
[[ -n "$BODY" ]] && echo "" && echo "  $BODY"
[[ -n "$REFS" ]] && echo "" && echo "  Refs: $REFS"
echo "  ─────────────────────────────────────"
echo ""
printf "  Commit? [Y/n]: "
read -r CONFIRM

if [[ "$CONFIRM" =~ ^[nN] ]]; then
  echo "  Aborted."
  exit 0
fi

git commit -m "$MSG"
