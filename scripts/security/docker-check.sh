#!/usr/bin/env bash
#
# docker-check.sh — Lint Dockerfiles for security best practices.
# Detects running as root, etc.
#
# Usage:
#   bash scripts/security/docker-check.sh

set -o errexit
set -o nounset
set -o pipefail

echo "==> Linting Dockerfiles for security (Root check)..."

# Find all Dockerfiles
DOCKERFILES=$(find . -name "Dockerfile*")

FAILED=0
for df in ${DOCKERFILES}; do
  echo "  Checking ${df}..."

  # Rule: Must contain a USER instruction (not root/0)
  if ! grep -qi "^USER" "${df}"; then
    echo "  [FAIL] ${df} has no USER instruction (defaults to root)."
    FAILED=1
  elif grep -qiE "^USER[[:space:]]+(root|0)[[:space:]]*$" "${df}"; then
    echo "  [FAIL] ${df} explicitly sets USER root/0."
    FAILED=1
  fi
done

if [[ "${FAILED}" -eq 1 ]]; then
  echo "ERROR: Docker security check failed." >&2
  exit 1
fi

echo "  all Dockerfiles passed."
exit 0
