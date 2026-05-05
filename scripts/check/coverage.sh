#!/usr/bin/env bash
# DEPRECATED: use 'make coverage' instead. See ADR-044.
set -o errexit
set -o nounset
set -o pipefail
echo "DEPRECATED: scripts/check/coverage.sh — use 'make coverage' instead" >&2
exit 1
