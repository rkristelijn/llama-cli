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

# Block direct commits to main and validate branch naming
branch="$(git symbolic-ref --short HEAD 2>/dev/null || true)"
if [[ "${branch}" == "main" ]]; then
  echo "ERROR: direct commits to main are not allowed. Use a feature branch."
  exit 1
fi

# Validate branch naming convention: type/description (kebab-case)
# @see docs/adr/adr-048-quality-framework.md §3.2 check 0.2
BRANCH_PATTERN="^(feat|fix|chore|docs|refactor|test|ci|release)/.+$"
if [[ -n "${branch}" ]] && ! [[ "${branch}" =~ ${BRANCH_PATTERN} ]]; then
  echo ""
  echo "ERROR: Branch name '${branch}' does not follow naming convention."
  echo ""
  echo "  Expected: type/description  (e.g. feat/streaming, fix/config-crash)"
  echo "  Types:    feat, fix, chore, docs, refactor, test, ci, release"
  echo ""
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

# Auto-summarize headers + rebuild index if Ollama is reachable
if curl -s --max-time 2 "http://${OLLAMA_HOST:-localhost:11434}/api/tags" > /dev/null 2>&1; then
  bash scripts/dev/summarize-headers.sh
  bash scripts/dev/build-index.sh
  git add INDEX.md
else
  echo "[skip] Ollama not running — skipping header summarization"
fi
