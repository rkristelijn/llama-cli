#!/usr/bin/env bash
#
# pre-commit — Block commits to main, auto-format staged C++ files, verify build.
#
# Installed by: make hooks / make setup
# Location:     .git/hooks/pre-commit (symlink or copy from scripts/git/pre-commit.sh)
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail

# Block direct commits to main
branch="$(git symbolic-ref --short HEAD 2>/dev/null || true)"
if [[ "${branch}" == "main" ]]; then
  echo "ERROR: direct commits to main are not allowed. Use a feature branch."
  exit 1
fi

# Auto-format staged C++ files
staged="$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h)$' || true)"
if [[ -n "${staged}" ]]; then
  echo "${staged}" | xargs clang-format --style=file:.config/.clang-format -i
  echo "${staged}" | xargs git add
fi

# Quick build + secret scan (catch secrets early)
bash scripts/git/precommit-check.sh
