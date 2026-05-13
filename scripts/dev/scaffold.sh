#!/usr/bin/env bash
#
# scaffold.sh — Generate new files from templates (ADR-100).
# lint-exempt: max-exits (CLI tool with per-branch input validation)
#
# Usage:
#   bash scripts/dev/scaffold.sh TYPE=cpp NAME=parser/tokenizer BRIEF="Tokenizes input"
#   bash scripts/dev/scaffold.sh TYPE=script NAME=check-foo DIR=lint BRIEF="Check foo"
#   bash scripts/dev/scaffold.sh TYPE=test NAME=parser/tokenizer
#
# Types: cpp (h+cpp), test, script, module (h+cpp+test)

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
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
TMPL_DIR="$PROJECT_ROOT/templates"

# --- Parse args (KEY=VALUE format) ---
TYPE=""
NAME=""
BRIEF="TODO: add description"
DIR=""
ADR=""

for arg in "$@"; do
  case "$arg" in
  TYPE=*) TYPE="${arg#TYPE=}" ;;
  NAME=*) NAME="${arg#NAME=}" ;;
  BRIEF=*) BRIEF="${arg#BRIEF=}" ;;
  DIR=*) DIR="${arg#DIR=}" ;;
  ADR=*) ADR="${arg#ADR=}" ;;
  *)
    echo "Unknown arg: $arg"
    exit 1
    ;;
  esac
done

if [[ -z "$TYPE" || -z "$NAME" ]]; then
  echo "Usage: scaffold.sh TYPE=<cpp|test|script|module> NAME=<name> [BRIEF=...] [DIR=...] [ADR=...]"
  exit 1
fi

# --- Derive variables ---
# NAME can be "module/file" (e.g. "parser/tokenizer")
MODULE=$(dirname "$NAME" 2>/dev/null)
BASENAME=$(basename "$NAME")
if [[ "$MODULE" == "." ]]; then MODULE="$BASENAME"; fi

# Include guard: MODULE_BASENAME_H → uppercase
GUARD=$(echo "${MODULE}_${BASENAME}" | tr '[:lower:]/' '[:upper:]_')

# --- Template substitution ---
render() {
  local tmpl="$1"
  sed -e "s|{{NAME}}|${BASENAME}|g" \
    -e "s|{{MODULE}}|${MODULE}|g" \
    -e "s|{{BRIEF}}|${BRIEF}|g" \
    -e "s|{{GUARD}}|${GUARD}|g" \
    -e "s|{{DIR}}|${DIR:-dev}|g" \
    -e "s|{{ADR}}|${ADR:-TODO}|g" \
    "$tmpl"
}

# --- Generate files ---
case "$TYPE" in
cpp)
  mkdir -p "src/$MODULE"
  OUT_H="src/$MODULE/$BASENAME.h"
  OUT_CPP="src/$MODULE/$BASENAME.cpp"
  if [[ -f "$OUT_H" ]]; then
    echo "EXISTS: $OUT_H"
    exit 1
  fi
  render "$TMPL_DIR/cpp.h.tmpl" >"$OUT_H"
  render "$TMPL_DIR/cpp.cpp.tmpl" >"$OUT_CPP"
  echo "  created $OUT_H"
  echo "  created $OUT_CPP"
  ;;
test)
  mkdir -p "src/$MODULE"
  OUT="src/$MODULE/${BASENAME}_test.cpp"
  if [[ -f "$OUT" ]]; then
    echo "EXISTS: $OUT"
    exit 1
  fi
  render "$TMPL_DIR/cpp_test.cpp.tmpl" >"$OUT"
  echo "  created $OUT"
  ;;
script)
  SCRIPT_DIR_OUT="scripts/${DIR:-dev}"
  mkdir -p "$SCRIPT_DIR_OUT"
  OUT="$SCRIPT_DIR_OUT/$BASENAME.sh"
  if [[ -f "$OUT" ]]; then
    echo "EXISTS: $OUT"
    exit 1
  fi
  render "$TMPL_DIR/script.sh.tmpl" >"$OUT"
  chmod +x "$OUT"
  echo "  created $OUT"
  ;;
module)
  mkdir -p "src/$MODULE"
  OUT_H="src/$MODULE/$BASENAME.h"
  OUT_CPP="src/$MODULE/$BASENAME.cpp"
  OUT_TEST="src/$MODULE/${BASENAME}_test.cpp"
  if [[ -f "$OUT_H" ]]; then
    echo "EXISTS: $OUT_H"
    exit 1
  fi
  render "$TMPL_DIR/cpp.h.tmpl" >"$OUT_H"
  render "$TMPL_DIR/cpp.cpp.tmpl" >"$OUT_CPP"
  render "$TMPL_DIR/cpp_test.cpp.tmpl" >"$OUT_TEST"
  echo "  created $OUT_H"
  echo "  created $OUT_CPP"
  echo "  created $OUT_TEST"
  echo "  NOTE: add src/$MODULE/$BASENAME.cpp to CMakeLists.txt LIB_SOURCES"
  ;;
*)
  echo "Unknown TYPE: $TYPE (use: cpp, test, script, module)"
  exit 1
  ;;
esac
