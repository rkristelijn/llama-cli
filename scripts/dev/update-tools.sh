#!/usr/bin/env bash
#
# update-tools.sh — Show outdated tools and optionally upgrade them.
#
# Compares installed versions against .config/versions.env and
# checks brew for newer versions available upstream.
#
# Usage:
#   make update           # show status + upgrade outdated
#   DRYRUN=1 make update  # show status only, don't upgrade
#
# @see .config/versions.env — pinned versions

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
cd "$(dirname "$0")/../.."

# shellcheck source=../../.config/versions.env
source .config/versions.env

DRYRUN="${DRYRUN:-0}"
OUTDATED=0
MISSING=0
OK=0

#######################################
# Compare installed version against pinned version.
# Arguments:
#   $1 — tool name
#   $2 — pinned version (from versions.env)
#   $3 — installed version (or empty)
#   $4 — brew package name (for upgrade)
#######################################
check_tool() {
  local name="$1" pinned="$2" installed="$3" brew_pkg="${4:-$1}"

  if [[ -z "${installed}" ]]; then
    printf "  %-20s %-12s → %-12s %s\n" "${name}" "MISSING" "${pinned}" "⚠️"
    MISSING=$((MISSING + 1))
    return
  fi

  if [[ "${installed}" == "${pinned}" ]] || echo "${installed}" | grep -q "^${pinned}"; then
    printf "  %-20s %-12s %s\n" "${name}" "${installed}" "✓"
    OK=$((OK + 1))
  else
    printf "  %-20s %-12s → %-12s %s\n" "${name}" "${installed}" "${pinned}" "⬆️"
    OUTDATED=$((OUTDATED + 1))
  fi
}

#######################################
# Get installed version of a tool.
#######################################
get_ver() {
  case "$1" in
  cmake) cmake --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1 ;;
  clang-format) clang-format --version 2>/dev/null | grep -oE '[0-9]+' | head -1 ;;
  cppcheck) cppcheck --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+' ;;
  doxygen) doxygen --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' ;;
  shellcheck) shellcheck --version 2>/dev/null | grep '^version:' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' ;;
  yamllint) yamllint --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' ;;
  rumdl) rumdl version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' ;;
  semgrep) semgrep --version 2>/dev/null ;;
  gitleaks) gitleaks version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' ;;
  trivy) trivy --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1 ;;
  trufflehog) trufflehog --version 2>&1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' ;;
  grype) grype version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1 ;;
  syft) syft version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1 ;;
  osv-scanner) osv-scanner --version 2>/dev/null | grep 'osv-scanner' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' ;;
  checkov) checkov --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' ;;
  git-cliff) git-cliff --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' ;;
  *) echo "" ;;
  esac 2>/dev/null || echo ""
}

main() {
  echo "==> Tool version status (.config/versions.env)"
  echo ""
  printf "  %-20s %-12s %s\n" "TOOL" "INSTALLED" "STATUS"
  printf "  %-20s %-12s %s\n" "----" "---------" "------"

  check_tool "cmake" "${CMAKE_VERSION}" "$(get_ver cmake)"
  check_tool "cppcheck" "${CPPCHECK_VERSION}" "$(get_ver cppcheck)"
  check_tool "doxygen" "${DOXYGEN_VERSION}" "$(get_ver doxygen)"
  check_tool "shellcheck" "${SHELLCHECK_VERSION}" "$(get_ver shellcheck)"
  check_tool "yamllint" "${YAMLLINT_VERSION}" "$(get_ver yamllint)"
  check_tool "rumdl" "${RUMDL_VERSION}" "$(get_ver rumdl)"
  check_tool "semgrep" "${SEMGREP_VERSION}" "$(get_ver semgrep)"
  check_tool "gitleaks" "${GITLEAKS_VERSION}" "$(get_ver gitleaks)"
  check_tool "trivy" "${TRIVY_VERSION}" "$(get_ver trivy)"
  check_tool "trufflehog" "${TRUFFLEHOG_VERSION}" "$(get_ver trufflehog)"
  check_tool "grype" "${GRYPE_VERSION}" "$(get_ver grype)"
  check_tool "syft" "${SYFT_VERSION}" "$(get_ver syft)"
  check_tool "osv-scanner" "${OSV_SCANNER_VERSION}" "$(get_ver osv-scanner)"
  check_tool "checkov" "${CHECKOV_VERSION}" "$(get_ver checkov)"
  check_tool "git-cliff" "${GIT_CLIFF_VERSION}" "$(get_ver git-cliff)"

  echo ""
  echo "  ${OK} up-to-date, ${OUTDATED} outdated, ${MISSING} missing"

  # Show brew outdated if on macOS
  if [[ "$(uname -s)" == "Darwin" ]] && command -v brew >/dev/null 2>&1; then
    echo ""
    echo "==> Checking brew for newer versions available upstream..."
    local brew_outdated
    brew_outdated=$(brew outdated --quiet 2>/dev/null | grep -E "semgrep|grype|syft|trufflehog|osv-scanner|checkov|trivy|gitleaks|rumdl|git-cliff|doxygen|shellcheck|yamllint|cppcheck" || true)
    if [[ -n "${brew_outdated}" ]]; then
      echo "  Newer versions available via brew:"
      echo "${brew_outdated}" | sed 's/^/    /'
      if [[ "${DRYRUN}" == "0" ]]; then
        echo ""
        echo "  Upgrading..."
        for pkg in ${brew_outdated}; do
          brew upgrade "${pkg}" 2>/dev/null || true
        done
        echo "  Done. Run 'make update' again to verify."
      else
        echo ""
        echo "  (dry-run mode — run without DRYRUN=1 to upgrade)"
      fi
    else
      echo "  All brew packages at latest version."
    fi
  fi
}

main "$@"
