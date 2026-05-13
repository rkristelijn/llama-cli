#!/usr/bin/env bash
#
# check-version-pins.sh — Enforce that all version references come from versions.env.
#
# Checks:
# 1. GitHub Actions in workflows use SHA pins (not mutable tags like @v4)
# 2. GitHub Action SHAs in workflows match those in versions.env
# 3. No hardcoded version numbers in scripts (should source versions.env)
#
# Usage:
#   make lint-versions
#   bash scripts/lint/check-version-pins.sh
#
# @see .config/versions.env — single source of truth
# @see docs/adr/adr-026-version-pinning.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Source cpm ui.sh if available, otherwise define minimal fallback
if [[ -f "${CPM_UI:-lib/cpm/shell/ui.sh}" ]]; then
  source "${CPM_UI:-lib/cpm/shell/ui.sh}"
else
  print_step() { echo "  $2 $3${4:+ $4}"; }
  print_header() { echo "==> $1"; }
  print_error() { echo "  ERROR: $1"; }
  print_warning() { echo "  WARNING: $1"; }
fi
cd "$(dirname "$0")/../.."

# shellcheck source=../../.config/versions.env
source .config/versions.env

FAIL=0

print_header "checking version pinning..."

# --- Check 1: No mutable tags in GitHub Actions workflows ---
# Actions must use SHA@<hash> # <version>, not @v4 or @v2
while IFS= read -r line; do
  # Skip lines that are already SHA-pinned (40 hex chars)
  if echo "${line}" | grep -qE '@[0-9a-f]{40}'; then
    continue
  fi
  # Flag mutable tag references like @v4, @v2.1, @main
  if echo "${line}" | grep -qE 'uses:.*@'; then
    print_error "mutable action tag: ${line}"
    FAIL=1
  fi
done < <(grep -rn 'uses:' .github/workflows/ 2>/dev/null | grep -v '#')

# --- Check 2: Verify workflow SHAs match versions.env ---
verify_sha() {
  local action="$1" expected_sha="$2" expected_ver="$3"
  local found
  found=$(grep -rh "uses: ${action}@" .github/workflows/ 2>/dev/null | head -1 || true)
  if [[ -n "${found}" ]]; then
    if ! echo "${found}" | grep -q "${expected_sha}"; then
      print_error "${action} SHA mismatch (expected ${expected_sha} # ${expected_ver})"
      FAIL=1
    fi
  fi
}

verify_sha "actions/checkout" "${ACTION_CHECKOUT_SHA}" "${ACTION_CHECKOUT_VERSION}"
verify_sha "actions/download-artifact" "${ACTION_DOWNLOAD_ARTIFACT_SHA}" "${ACTION_DOWNLOAD_ARTIFACT_VERSION}"
verify_sha "actions/upload-artifact" "${ACTION_UPLOAD_ARTIFACT_SHA}" "${ACTION_UPLOAD_ARTIFACT_VERSION}"
verify_sha "softprops/action-gh-release" "${ACTION_GH_RELEASE_SHA}" "${ACTION_GH_RELEASE_VERSION}"
verify_sha "dorny/paths-filter" "${ACTION_PATHS_FILTER_SHA}" "${ACTION_PATHS_FILTER_VERSION}"
verify_sha "codecov/codecov-action" "${ACTION_CODECOV_SHA}" "${ACTION_CODECOV_VERSION}"

# --- Check 3: No hardcoded version numbers in scripts ---
# Scripts should `source .config/versions.env` and use variables.
# We look for patterns like: VERSION=1.2.3 or ver="1.2.3" that aren't in versions.env itself.
HARDCODED=$(grep -rn --include='*.sh' -E '(VERSION|_VER|_VERSION)="?[0-9]+\.[0-9]+' scripts/ 2>/dev/null |
  grep -v 'versions.env' |
  grep -v 'source.*versions' |
  grep -vF '${' |
  grep -v '# pinned' |
  grep -v 'check-version' ||
  true)

if [[ -n "${HARDCODED}" ]]; then
  print_error "hardcoded versions found in scripts (should use versions.env):"
  echo "${HARDCODED}" | sed 's/^/    /'
  FAIL=1
fi

if [[ "${FAIL}" -eq 0 ]]; then
  :
else
  echo ""
  echo "  FIX: Pin actions to SHA in workflows, use \$VAR from versions.env in scripts."
  exit 1
fi
