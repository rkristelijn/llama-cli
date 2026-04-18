#!/usr/bin/env bash
#
# commit-msg — Validate commit message follows Conventional Commits format.
#
# Format: type(scope): description  OR  type: description
# Types:  feat, fix, chore, refactor, docs, test, ci, style, perf, build
#
# Installed by: make hooks
# @see docs/adr/adr-048-quality-framework.md §3.2 check 0.1

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

MSG_FILE="$1"
MSG=$(head -1 "$MSG_FILE")

# Allow merge commits
if [[ "$MSG" =~ ^Merge ]]; then
  exit 0
fi

PATTERN="^(feat|fix|chore|refactor|docs|test|ci|style|perf|build)(\(.+\))?: .{1,72}$"

if ! [[ "$MSG" =~ $PATTERN ]]; then
  echo ""
  echo "ERROR: Commit message does not follow Conventional Commits format."
  echo ""
  echo "  Expected: type(scope): description"
  echo "  Examples: feat: add streaming support"
  echo "            fix(repl): handle empty input"
  echo "            chore: update dependencies"
  echo ""
  echo "  Valid types: feat fix chore refactor docs test ci style perf build"
  echo ""
  exit 1
fi
