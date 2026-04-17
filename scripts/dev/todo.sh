#!/usr/bin/env bash
#
# todo.sh — Show TODO items from markdown files and source code.
#
# Usage:
#   bash scripts/dev/todo.sh
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

main() {
  echo "==> Markdown TODOs"
  find . -name "*.md" -not -path "./build/*" -not -path "./.git/*" -exec awk '
    FNR == 1 {
      if (prev_line != "") printf "%s:%d:%s\n", prev_file, prev_lnum, prev_line;
      prev_line = "";
    }
    /- \[ \]/ {
      match($0, /[^ ]/);
      curr_indent = RSTART;
      if (prev_line != "") {
        if (curr_indent > prev_indent) {
          printf "%s:%d:\033[1m%s\033[0m\n", FILENAME, prev_lnum, prev_line;
        } else {
          printf "%s:%d:%s\n", FILENAME, prev_lnum, prev_line;
        }
      }
      prev_line = $0;
      prev_indent = curr_indent;
      prev_lnum = FNR;
      prev_file = FILENAME;
    }
    END {
      if (prev_line != "") printf "%s:%d:%s\n", FILENAME, prev_lnum, prev_line;
    }' {} +

  echo ""
  echo "==> Code TODOs"
  grep -rn "TODO\|FIXME\|HACK\|XXX" src/ include/ --include="*.cpp" --include="*.h" 2>/dev/null || true
}

main "$@"
