#!/usr/bin/env bash
#
# check-deps.sh — Verify required and optional tools are installed.
#
# Required tools cause a hard fail. Optional tools show a warning.
#
# Usage:
#   bash scripts/check/check-deps.sh
#
# @see docs/adr/adr-44-tidy-boilerplate.md

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
# TODO: !! move clang-tidy and pmccabe back to REQUIRED after brew install llvm
# See docs/backlog/022-fix-make-setup.md for install instructions
REQUIRED=(cmake clang-format cppcheck cloc doxygen)
OPTIONAL=(clang-tidy pmccabe semgrep zsteg gitleaks shellcheck yamllint rumdl)

missing=0
warnings=0

find_tool() {
  local tool="$1"
  if command -v "${tool}" >/dev/null 2>&1; then
    echo "$(command -v "${tool}")"
    return 0
  fi
  # macOS Homebrew LLVM path fallback
  if [[ "${tool}" == "clang-tidy" && -f "/opt/homebrew/opt/llvm/bin/clang-tidy" ]]; then
    echo "/opt/homebrew/opt/llvm/bin/clang-tidy"
    return 0
  fi
  # Ruby user gem path fallback
  if [[ "${tool}" == "zsteg" ]]; then
    USER_GEM_BIN=$(ruby -e 'puts File.join(Gem.user_dir, "bin")' 2>/dev/null || echo "")
    if [[ -n "${USER_GEM_BIN}" && -f "${USER_GEM_BIN}/zsteg" ]]; then
      echo "${USER_GEM_BIN}/zsteg"
      return 0
    fi
  fi
  return 1
}

main() {
  print_header "Checking required tools..."
  for tool in "${REQUIRED[@]}"; do
    if find_tool "${tool}" >/dev/null; then
      printf "  %-20s ✓\n" "${tool}"
    else
      printf "  %-20s MISSING\n" "${tool}"
      ((missing++)) || true
    fi
  done

  echo ""
  print_header "Checking optional tools..."
  for tool in "${OPTIONAL[@]}"; do
    if find_tool "${tool}" >/dev/null; then
      printf "  %-20s ✓\n" "${tool}"
    else
      printf "  %-20s not installed (optional)\n" "${tool}"
      ((warnings++)) || true
    fi
  done

  echo ""
  if ((missing > 0)); then
    echo "${missing} required tool(s) missing. Run 'make setup' to install."
    exit 1
  fi
  if ((warnings > 0)); then
    echo "All required tools present. ${warnings} optional tool(s) missing."
  else
    echo "All tools present."
  fi
}

main "$@"
