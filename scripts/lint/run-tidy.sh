#!/usr/bin/env bash
#
# tidy.sh — Run clang-tidy on source files.
#
# In smart mode (default), only checks files changed vs main.
# In full mode (--full), checks all source files.
#
# Usage:
#   bash scripts/check/tidy.sh [--full]
#
# @see .config/.clang-tidy
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

CLANG_TIDY="${CLANG_TIDY:-$(command -v clang-tidy 2>/dev/null || echo /opt/homebrew/opt/llvm/bin/clang-tidy)}"
FULL=false
[[ "${1-}" == "--full" ]] && FULL=true

# Filter pattern for warnings we suppress
FILTER="linenoise\|SCENARIO\|cognitive complexity\|identifier-naming\|logging/logger.*function-size"

main() {
  if [[ "${FULL}" == true ]]; then
    echo "==> make tidy (full mode)"
    find src -name '*.cpp' -print0 \
      | xargs -0 "${CLANG_TIDY}" --config-file=.config/.clang-tidy -- -std=c++17 -I src/ 2>&1 \
      | grep "warning:" | grep -v "${FILTER}" && exit 1 || true
  else
    echo "==> make tidy (smart incremental mode)"
    local branch diff_base files
    branch="$(git rev-parse --abbrev-ref HEAD)"
    diff_base="$( [[ "${branch}" == "main" ]] && echo "HEAD^" || echo "origin/main" )"
    files="$(git diff --name-only "${diff_base}" | grep '\.cpp$' | grep '^src/' || true)"
    local headers
    headers="$(git diff --name-only "${diff_base}" | grep '\.h$' | grep '^src/' || true)"

    # If headers changed, include all cpp files from those directories
    if [[ -n "${headers}" ]]; then
      local header_dirs
      header_dirs="$(echo "${headers}" | xargs -n1 dirname | sort -u)"
      for hdir in ${header_dirs}; do
        files="$(printf '%s\n%s' "${files}" "$(find "${hdir}" -maxdepth 1 -name '*.cpp' 2>/dev/null)" | sort -u)"
      done
      files="$(echo "${files}" | sed '/^$/d')"
    fi

    if [[ -z "${files}" ]]; then
      echo "  [skip] no changed files vs ${diff_base}"
    else
      for dir in src $(find src -maxdepth 1 -mindepth 1 -type d); do
        dir_files="$(echo "${files}" | grep "^${dir}/[^/]*\.cpp$" || true)"
        if [[ -n "${dir_files}" ]]; then
          echo "  [checking] ${dir}/ ($(echo "${dir_files}" | wc -w) files)"
          ${CLANG_TIDY} --config-file=.config/.clang-tidy ${dir_files} -- -std=c++17 -I src/ 2>&1 \
            | grep "warning:" | grep -v "${FILTER}" && exit 1 || true
        fi
      done
    fi
  fi
  echo "  [done] tidy"
}

main "$@"
