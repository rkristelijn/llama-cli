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

# Type selection via decision flow
echo ""
print_header "Conventional Commit"
echo "  What does this commit do?"
echo ""
echo "    f) fix        Fix a bug"
echo "    a) feat       Add new functionality"
echo "    r) refactor   Restructure code (no behavior change)"
echo "    d) docs       Documentation only"
echo "    t) test       Add or fix tests"
echo "    b) build      Deps, tooling, config"
echo "    c) ci         Pipeline/workflow"
echo "    p) perf       Performance improvement"
echo "    s) style      Formatting only"
echo "    x) chore      Truly nothing else fits"
echo ""
printf "  [f/a/r/d/t/b/c/p/s/x]: "
read -r type_key
case "${type_key:-f}" in
  f) TYPE="fix" ;;
  a) TYPE="feat" ;;
  r) TYPE="refactor" ;;
  d) TYPE="docs" ;;
  t) TYPE="test" ;;
  b) TYPE="build" ;;
  c) TYPE="ci" ;;
  p) TYPE="perf" ;;
  s) TYPE="style" ;;
  x) TYPE="chore" ;;
  *) TYPE="fix" ;;
esac

# Nudge: chore is often a lazy choice
if [[ "$TYPE" == "chore" ]]; then
  echo ""
  echo "  Hmm — could it be more specific?"
  echo "    r) refactor  → code change, no new behavior"
  echo "    b) build     → deps, tooling, config"
  echo "    c) ci        → pipeline/workflow"
  echo "    d) docs      → only documentation"
  printf "  Still chore? [y/r/b/c/d]: "
  read -r still_chore
  case "$still_chore" in
    r) TYPE="refactor" ;;
    b) TYPE="build" ;;
    c) TYPE="ci" ;;
    d) TYPE="docs" ;;
  esac
fi

# 2. Scope (optional)
echo ""
SCOPES=$(git log --oneline -50 2>/dev/null | sed 's/^[a-f0-9]* //' | grep -oE '\([^)]+\)' | tr -d '()' | sort -u | tr '\n' ', ' | sed 's/,$//')
[[ -n "$SCOPES" ]] && echo "  Recent scopes: $SCOPES"
printf "  Scope (optional, enter to skip): "
read -r SCOPE

# 3. Short description
echo ""
echo "  Write as imperative: \"when applied, this commit will...\""
echo "  Examples: add streaming support, fix empty input crash, remove unused dep"
printf "  Description (max 72 chars): "
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
