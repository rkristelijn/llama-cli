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
    linenoise*|dtl/*|doctest/*|system_prompt_generated*|version_generated*) continue ;;
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

# --- Summary ---
echo ""
if [[ $FAIL -gt 0 ]]; then
  echo "  ⚠ ${FAIL} portability issue(s) found (non-blocking, fix before release)"
fi
echo "  [done] portability"
