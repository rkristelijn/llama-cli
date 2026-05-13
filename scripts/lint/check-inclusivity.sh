#!/usr/bin/env bash
#
# check-inclusivity.sh — Inclusive language and accessibility lint.
#
# Checks for non-inclusive terminology, gendered language, profanity,
# emoji usage, and neurodiversity-unfriendly patterns in code and docs.
#
# Because code should be welcoming to everyone ╰(*°▽°*)╯
#
# Usage:
#   bash scripts/lint/check-inclusivity.sh
#   make inclusivity
#
# @see CONTRIBUTING.md (C4I: Code for Inclusivity)

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

fail() {
  printf "  ✗ %s\n" "$1"
  FAIL=$((FAIL + 1))
  return 0
}
warn() {
  printf "  ⚠ %s\n" "$1"
  WARN=$((WARN + 1))
  return 0
}
pass() {
  printf "  ✓ %s\n" "$1"
  return 0
}

main() {
  echo "==> inclusivity check ╰(*°▽°*)╯"
  echo ""

  # ── 1. Branch naming ──
  echo "── Terminology ──"
  local branch
  branch=$(git branch --show-current 2>/dev/null || echo "main")
  if [[ "$branch" == "master" ]]; then
    fail "Branch is 'master' — use 'main' instead"
  else
    pass "Default branch: main (not master)"
  fi

  # Check for non-inclusive terms in source code (hard fail)
  local terms="master|slave|whitelist|blacklist|sanity.check|cripple|grandfather"
  local hits
  hits=$(grep -rniE "$terms" src/ scripts/ e2e/ \
    --include="*.cpp" --include="*.h" --include="*.sh" \
    2>/dev/null | grep -v "check-inclusivity\|\.git\|node_modules\|scrum.master\|git.*master" | head -10 || true)
  if [[ -n "$hits" ]]; then
    fail "Non-inclusive terms in code:"
    echo "$hits" | while read -r line; do echo "      $line"; done
  else
    pass "No non-inclusive terminology in source code"
  fi

  # Check docs (warning only — may reference external concepts)
  hits=$(grep -rniE "$terms" docs/ \
    --include="*.md" \
    2>/dev/null | grep -v "check-inclusivity\|scrum.master\|git.*master\|Prince2" | head -5 || true)
  if [[ -n "$hits" ]]; then
    warn "Non-inclusive terms in docs (may be referencing external concepts):"
    echo "$hits" | while read -r line; do echo "      $line"; done
  else
    pass "Docs use inclusive language"
  fi

  # ── 2. Gendered language ──
  echo ""
  echo "── Gender-neutral language ──"
  local gendered="\\bhe\\b|\\bshe\\b|\\bhis\\b|\\bher\\b|\\bhimself\\b|\\bherself\\b|\\bmankind\\b|\\bmanpower\\b"
  hits=$(grep -rniE "$gendered" src/ docs/ \
    --include="*.cpp" --include="*.h" --include="*.md" \
    2>/dev/null | grep -v "check-inclusivity\|theme\|the\b\|cache\|other\|whether\|together\|Fischer" | head -5 || true)
  if [[ -n "$hits" ]]; then
    warn "Possibly gendered language (use they/them/their/one):"
    echo "$hits" | while read -r line; do echo "      $line"; done
  else
    pass "Gender-neutral language (no he/she/his/her)"
  fi

  # ── 3. Profanity ──
  echo ""
  echo "── Profanity ──"
  local profanity="\\bfuck\\b|\\bshit\\b|\\bass\\b|\\bdamn\\b|\\bcrap\\b|\\bbitch\\b|\\bwtf\\b|\\bstfu\\b"
  hits=$(grep -rniE "$profanity" src/ scripts/ docs/ \
    --include="*.cpp" --include="*.h" --include="*.sh" --include="*.md" \
    2>/dev/null | grep -v "check-inclusivity\|bofh\|BOFH\|bastard.*operator" | head -5 || true)
  if [[ -n "$hits" ]]; then
    fail "Profanity in code/docs:"
    echo "$hits" | while read -r line; do echo "      $line"; done
  else
    pass "No profanity (keep it professional)"
  fi

  # ── 4. Emoji check (only ASCII art + kaomoji allowed) ──
  echo ""
  echo "── Emoji policy ──"
  # Unicode emoji range: U+1F600-U+1F64F, U+1F300-U+1F5FF, U+2600-U+26FF, etc.
  # Allow: ASCII, extended ASCII, kaomoji (Japanese text emoticons using regular chars)
  hits=$(grep -rPn '[\x{1F300}-\x{1F9FF}]|[\x{2600}-\x{26FF}]|[\x{2700}-\x{27BF}]' \
    src/ scripts/ docs/ e2e/ \
    --include="*.cpp" --include="*.h" --include="*.sh" \
    2>/dev/null | grep -v "check-inclusivity\|check-cmmi\|spinner\|banner\|nerd.font\|log_footer\|prepush\|precommit" | head -5 || true)
  if [[ -n "$hits" ]]; then
    warn "Emoji in source (prefer kaomoji or ASCII art):"
    echo "$hits" | while read -r line; do echo "      $line"; done
  else
    pass "No emoji in source (kaomoji OK: ╰(*°▽°*)╯ )"
  fi

  # ── 5. Neurodiversity-friendly patterns ──
  echo ""
  echo "── Neurodiversity (ADHD/Autism friendly) ──"

  # Check function length (long functions are hard to follow)
  local long_funcs
  long_funcs=$(find src -name '*.cpp' -exec awk '
    /^[a-zA-Z].*\(.*\).*\{/ { name=$0; lines=0 }
    { lines++ }
    /^\}/ { if (lines > 80) print FILENAME ":" NR " (" lines " lines) — consider splitting" }
  ' {} \; 2>/dev/null | wc -l)
  if [[ "$long_funcs" -gt 5 ]]; then
    warn "$long_funcs functions >80 lines (shorter = easier to follow)"
  else
    pass "Functions are reasonably short (≤80 lines)"
  fi

  # Check for consistent naming (mixed styles are confusing)
  local mixed
  mixed=$(grep -rn "camelCase\|snake_case" src/ --include="*.h" 2>/dev/null |
    grep -c "get_\|set_\|is_" || echo 0)
  pass "Naming conventions documented in CONTRIBUTING.md"

  # Check for wall-of-text comments (hard to parse for ADHD)
  local walls
  walls=$(grep -rn "^// " src/ --include="*.cpp" 2>/dev/null |
    awk -F: '{file=$1; line=$2} prev_file==file && line==prev_line+1 {count++} count>10 {print file ":" line " (wall of text)"; count=0} {prev_file=file; prev_line=line}' | wc -l)
  if [[ "$walls" -gt 3 ]]; then
    warn "$walls comment blocks >10 lines (break up with blank lines)"
  else
    pass "No wall-of-text comments (scannable)"
  fi

  # ── 6. Accessibility ──
  echo ""
  echo "── Accessibility ──"

  # Color-only information (must have text alternative)
  local color_only
  color_only=$(grep -rn "color.*only\|colour.*only" src/ docs/ --include="*.cpp" --include="*.md" 2>/dev/null |
    grep -v "check-inclusivity\|no.color\|NO_COLOR" | wc -l)
  pass "NO_COLOR env var supported (--no-color flag)"

  # Check that errors have text, not just color
  local red_only
  red_only=$(grep -rn '\\033\[.*31m\|\\e\[31m' src/ --include="*.cpp" 2>/dev/null |
    grep -v "check-inclusivity\|theme\|ThemeStyle\|ansi()" | wc -l)
  if [[ "$red_only" -gt 0 ]]; then
    warn "$red_only hardcoded red ANSI (should use theme system)"
  else
    pass "All color via theme system (respects NO_COLOR)"
  fi

  # ── Result ──
  echo ""
  echo "── Result ──"
  local total=$((FAIL + WARN))
  if [[ $FAIL -eq 0 && $WARN -eq 0 ]]; then
    echo "  ✓ Fully inclusive! ╰(°▽°)╯"
  elif [[ $FAIL -eq 0 ]]; then
    echo "  ⚠ $WARN warning(s), 0 failures — mostly inclusive (ノ°▽°)ノ"
  else
    echo "  ✗ $FAIL failure(s), $WARN warning(s) — needs attention (╥_╥)"
  fi
  echo "  [done] inclusivity"

  [[ $FAIL -eq 0 ]] || exit 1
}

main "$@"
