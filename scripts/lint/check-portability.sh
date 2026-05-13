#!/usr/bin/env bash
#
# check-portability.sh — Detect cross-platform issues before they hit CI.
#
# Catches:
#   - Names that conflict with system headers (macOS MacTypes.h, Windows.h)
#   - Case-sensitive include mismatches (Linux vs macOS/Windows)
#   - Hardcoded path separators
#   - POSIX-only headers without platform guards
#   - Type size assumptions (long vs int32_t)
#
# Usage:
#   bash scripts/lint/check-portability.sh

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
FAIL=0
WARN=0

echo "==> checking portability..."

# --- Check 1: Names that conflict with macOS system headers ---
MACOS_RESERVED="Style|Byte|Handle|Ptr|Fixed|Fract"
conflicts=$(grep -rn "^struct \(${MACOS_RESERVED}\)\b\|^class \(${MACOS_RESERVED}\)\b" src/ --include="*.h" 2>/dev/null || true)
if [[ -n "$conflicts" ]]; then
  echo "  [fail] macOS MacTypes.h name conflicts:"
  echo "$conflicts" | sed 's/^/    /'
  FAIL=1
fi

# --- Check 2: Names that conflict with Windows headers ---
WIN_RESERVED="BOOL|BYTE|WORD|DWORD|HANDLE|ERROR|DELETE|interface|far|near"
win_conflicts=$(grep -rn "^struct \(${WIN_RESERVED}\)\b\|^class \(${WIN_RESERVED}\)\b\|^#define \(${WIN_RESERVED}\)\b" src/ --include="*.h" 2>/dev/null || true)
if [[ -n "$win_conflicts" ]]; then
  echo "  [fail] Windows.h name conflicts:"
  echo "$win_conflicts" | sed 's/^/    /'
  FAIL=1
fi

# --- Check 3: Case-sensitive include mismatches ---
# Exclude external deps fetched by CMake (linenoise, dtl, doctest)
mismatches=""
while IFS= read -r inc; do
  file=$(echo "$inc" | sed 's/.*#include "//;s/".*//')
  # Skip known external headers
  case "$file" in
  linenoise* | dtl/* | doctest/* | system_prompt_generated* | version_generated*) continue ;;
  esac
  if [[ -n "$file" ]] && ! find src/ -path "*/$file" -print -quit 2>/dev/null | grep -q .; then
    mismatches+="  $inc\n"
  fi
done < <(grep -rn '#include "' src/ --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "generated\|doctest" || true)
if [[ -n "$mismatches" ]]; then
  echo "  [fail] Include path not found (case mismatch?):"
  echo -e "$mismatches" | head -10
  FAIL=1
fi

# --- Check 4: MSVC-only or platform-specific headers ---
platform_headers=$(grep -rn "stdafx.h\|conio.h\|direct.h\|io.h" src/ --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "#ifdef\|#if\|//\|NOLINT" || true)
if [[ -n "$platform_headers" ]]; then
  echo "  [fail] Platform-specific headers without guard:"
  echo "$platform_headers" | sed 's/^/    /'
  FAIL=1
fi

# --- Check 5: POSIX headers count (informational for Windows port) ---
posix_count=$(grep -rn "dirent.h\|sys/stat.h\|sys/ioctl.h\|termios.h\|unistd.h\|sys/wait.h" src/ --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "#ifdef\|#if\|_WIN32" | wc -l)
if [[ "$posix_count" -gt 0 ]]; then
  echo "  [info] ${posix_count} POSIX header includes (need abstraction for Windows port)"
  WARN=1
fi

# --- Check 6: sizeof(long) assumptions (long is 4 bytes on Windows, 8 on Linux 64-bit) ---
long_issues=$(grep -rn "\bsizeof(long)\|\b(long)\b.*cast\|static_cast<long>" src/ --include="*.cpp" --include="*.h" 2>/dev/null | grep -v "long long\|//\|NOLINT" || true)
if [[ -n "$long_issues" ]]; then
  echo "  [warn] sizeof(long) varies across platforms — prefer int32_t/int64_t:"
  echo "$long_issues" | sed 's/^/    /' | head -5
  WARN=1
fi

# --- Check 7: Shell script portability (non-POSIX commands) ---
# grep -P (Perl regex) is GNU-only — fails on macOS/BSD/Alpine
grep_p=$(grep -rn 'grep -[a-zA-Z]*P\|grep --perl-regexp' scripts/ --include="*.sh" 2>/dev/null | grep -v "check-portability.sh" || true)
if [[ -n "$grep_p" ]]; then
  echo "  [fail] grep -P (Perl regex) — not available on macOS/BSD/Alpine:"
  echo "$grep_p" | sed 's/^/    /' | head -5
  echo "    → Use grep -E (extended regex) or literal Unicode chars instead"
  FAIL=1
fi

# sed -i without '' arg — GNU sed uses -i, BSD/macOS sed requires -i ''
sed_no_backup=$(grep -rn 'sed -i "' scripts/ --include="*.sh" 2>/dev/null | grep -v "check-portability.sh" || true)
if [[ -n "$sed_no_backup" ]]; then
  echo "  [warn] sed -i without backup suffix — differs between GNU and BSD:"
  echo "$sed_no_backup" | sed 's/^/    /' | head -5
  echo "    → Use sed -i '' (BSD) or avoid in-place; pipe to temp file instead"
  WARN=1
fi

# readarray/mapfile — bash 4+ only, not available on macOS default bash (3.2)
readarray_use=$(grep -rn '\(readarray\|mapfile\)' scripts/ --include="*.sh" 2>/dev/null | grep -v "check-portability.sh" || true)
if [[ -n "$readarray_use" ]]; then
  echo "  [fail] readarray/mapfile — requires bash 4+ (macOS ships bash 3.2):"
  echo "$readarray_use" | sed 's/^/    /' | head -5
  echo "    → Use 'while IFS= read -r' loop instead"
  FAIL=1
fi

# associative arrays — bash 4+ only
assoc_arrays=$(grep -rn 'declare -A' scripts/ --include="*.sh" 2>/dev/null | grep -v "check-portability.sh" || true)
if [[ -n "$assoc_arrays" ]]; then
  echo "  [warn] declare -A (associative arrays) — requires bash 4+:"
  echo "$assoc_arrays" | sed 's/^/    /' | head -5
  echo "    → macOS default bash is 3.2; use case/if or source a lookup file"
  WARN=1
fi

# seq command — not available on all systems; use {1..N} or $((...))
seq_use=$(grep -rn '\bseq\b' scripts/ --include="*.sh" 2>/dev/null | grep -v '#\|check-portability.sh' || true)
if [[ -n "$seq_use" ]]; then
  echo "  [warn] seq — not POSIX, missing on some minimal systems:"
  echo "$seq_use" | sed 's/^/    /' | head -3
  echo "    → Use brace expansion {1..N} or arithmetic loop"
  WARN=1
fi

# echo -e — not portable (POSIX echo has no flags); use printf instead
echo_e=$(grep -rn "echo -e\b" scripts/ --include="*.sh" 2>/dev/null | grep -v "check-portability.sh" || true)
if [[ -n "$echo_e" ]]; then
  echo "  [warn] echo -e — behavior varies across shells and platforms:"
  echo "$echo_e" | sed 's/^/    /' | head -3
  echo "    → Use printf '%s\\n' for portable escape sequences"
  WARN=1
fi

# GNU-specific flags: grep --include used outside find piping (works on GNU, not all BSD)
# (We use it ourselves, so just flag if used with non-standard flags)
# stat command differs: GNU stat -c vs BSD stat -f
stat_gnu=$(grep -rn "stat -c\|stat --format" scripts/ --include="*.sh" 2>/dev/null | grep -v "check-portability.sh" || true)
if [[ -n "$stat_gnu" ]]; then
  echo "  [fail] stat -c/--format — GNU-only (BSD/macOS uses stat -f):"
  echo "$stat_gnu" | sed 's/^/    /' | head -3
  echo "    → Use 'wc -c < file' for size, or conditional stat per OS"
  FAIL=1
fi

# date GNU-specific: date -d (GNU) vs date -j -f (BSD)
date_gnu=$(grep -rn "date -d\|date --date" scripts/ --include="*.sh" 2>/dev/null | grep -v "check-portability.sh" || true)
if [[ -n "$date_gnu" ]]; then
  echo "  [warn] date -d/--date — GNU-only (BSD/macOS uses date -j -f):"
  echo "$date_gnu" | sed 's/^/    /' | head -3
  echo "    → Use portable date formats or conditional per OS"
  WARN=1
fi

# --- Summary ---
echo ""
if [[ $FAIL -gt 0 ]]; then
  echo "  ⚠ ${FAIL} portability issue(s) found (non-blocking, fix before release)"
fi
if [[ $WARN -gt 0 && $FAIL -eq 0 ]]; then
  echo "  ✓ no failures (${WARN} warning(s) — informational)"
fi
echo "  [done] portability"
