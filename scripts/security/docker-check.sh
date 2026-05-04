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

  # Rule: Must contain a USER instruction
  if ! grep -qi "^USER" "${df}"; then
    echo "  [FAIL] ${df} is running as root! Add a 'USER' instruction."
    FAILED=1
  fi
done

if [[ "${FAILED}" -eq 1 ]]; then
  echo "ERROR: Docker security check failed."
  exit 1
fi

echo "  all Dockerfiles passed."
exit 0
