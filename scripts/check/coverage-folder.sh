#!/usr/bin/env bash
#
# coverage-folder.sh — Show test coverage summary per source directory.
#
# Requires a coverage build (cmake --coverage) to have been run first.
#
# Usage:
#   bash scripts/check/coverage-folder.sh [build_dir]
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BUILD_DIR="${1:-build}"

main() {
  echo "==> make coverage-folder"
  for dir in src $(find src -maxdepth 1 -mindepth 1 -type d); do
    files="$(ls "${BUILD_DIR}"/CMakeFiles/*.dir/"${dir}"/*.cpp.o 2>/dev/null || true)"
    if [[ -n "${files}" ]]; then
      printf "  %-20s " "${dir}/"
      gcov -n ${files} 2>/dev/null \
        | sed 's/,/./g' \
        | awk '/Lines executed/ {
            split($2, a, ":"); split(a[2], b, "%");
            exec_lines += b[1] * $4 / 100;
            total_lines += $4;
          } END {
            if (total_lines > 0) printf "%.2f%%\n", (exec_lines / total_lines) * 100;
            else print "N/A";
          }' || echo "N/A"
    fi
  done
  echo "  [done] coverage-folder"
}

main "$@"
