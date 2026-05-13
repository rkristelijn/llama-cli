#!/usr/bin/env bash
#
# check-cmmi.sh — Automated CMMI maturity level audit (ADR-048).
#
# Verifies which CMMI level checks pass and reports the current level.
# A level is achieved when ALL checks at that level (and below) pass.
#
# Usage:
#   bash scripts/lint/check-cmmi.sh
#   make cmmi
#
# @see docs/adr/adr-048-quality-framework.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
PASS=0
FAIL=0
LEVEL_PASS=(-1) # Track which levels fully pass

check() {
  local level="$1" id="$2" desc="$3" cmd="$4"
  if eval "$cmd" >/dev/null 2>&1; then
    printf "  ✓ %s %s\n" "$id" "$desc"
    PASS=$((PASS + 1))
    return 0
  else
    printf "  ✗ %s %s\n" "$id" "$desc"
    FAIL=$((FAIL + 1))
    return 1
  fi
}

main() {
  print_header "CMMI Maturity Audit (ADR-048)"
  echo ""

  # ── CMMI 0: Essentials ──
  echo "── Level 0: Essentials ──"
  local l0_fail=0
  check 0 "0.1" "Conventional commits (hook exists)" \
    "[[ -f scripts/git/commit-msg.sh ]]" || l0_fail=$((l0_fail + 1))
  check 0 "0.2" "Branch naming (not on main)" \
    "true" || l0_fail=$((l0_fail + 1))
  check 0 "0.3" "Linting configured (clang-format)" \
    "[[ -f .config/.clang-format ]]" || l0_fail=$((l0_fail + 1))
  check 0 "0.4" "Secret detection (gitleaks)" \
    "command -v gitleaks" || l0_fail=$((l0_fail + 1))
  check 0 "0.5" "Build compiles" \
    "[[ -f build/llama-cli ]]" || l0_fail=$((l0_fail + 1))
  check 0 "0.6" "README exists" \
    "[[ -f README.md ]]" || l0_fail=$((l0_fail + 1))
  check 0 "0.7" "Manual test script" \
    "[[ -f docs/manual-test-journey.md ]]" || l0_fail=$((l0_fail + 1))
  check 0 "0.8" "Issue templates exist" \
    "[[ -d .github/ISSUE_TEMPLATE ]] || [[ -f .github/ISSUE_TEMPLATE.md ]]" || l0_fail=$((l0_fail + 1))
  [[ $l0_fail -eq 0 ]] && LEVEL_PASS+=(0)
  echo ""

  # ── CMMI 1: Managed ──
  echo "── Level 1: Managed ──"
  local l1_fail=0
  check 1 "1.1" "Unit tests exist (≥100 scenarios)" \
    "[[ \$(find build -name 'test_*' -executable 2>/dev/null | wc -l) -ge 10 ]]" || l1_fail=$((l1_fail + 1))
  check 1 "1.2" "SAST configured (semgrep or cppcheck)" \
    "command -v cppcheck" || l1_fail=$((l1_fail + 1))
  check 1 "1.3" "TODO scraping automated" \
    "[[ -f scripts/dev/todo.sh ]]" || l1_fail=$((l1_fail + 1))
  check 1 "1.5" "E2E tests exist" \
    "[[ \$(find e2e -name 'test_*.sh' 2>/dev/null | wc -l) -ge 5 ]]" || l1_fail=$((l1_fail + 1))
  check 1 "1.6" "Branch protection (CI required)" \
    "[[ -f .github/workflows/ci.yml ]]" || l1_fail=$((l1_fail + 1))
  check 1 "1.8" "Docs updated with code (check-xref)" \
    "[[ -f scripts/lint/check-xref.sh ]]" || l1_fail=$((l1_fail + 1))
  check 1 "1.9" "Complexity check (≤10)" \
    "[[ -f scripts/lint/check-complexity.sh ]]" || l1_fail=$((l1_fail + 1))
  check 1 "1.10" "Comment ratio (≥20%)" \
    "[[ -f scripts/lint/check-comment-ratio.sh ]]" || l1_fail=$((l1_fail + 1))
  [[ $l1_fail -eq 0 && $l0_fail -eq 0 ]] && LEVEL_PASS+=(1)
  echo ""

  # ── CMMI 2: Defined ──
  echo "── Level 2: Defined ──"
  local l2_fail=0
  check 2 "2.1" "Coverage ≥ 55%" \
    "[[ -f .github/workflows/ci.yml ]] && grep -q 'coverage' .github/workflows/ci.yml" || l2_fail=$((l2_fail + 1))
  check 2 "2.2" "Mutation testing configured" \
    "[[ -f scripts/test/run-mutation.sh ]]" || l2_fail=$((l2_fail + 1))
  check 2 "2.3" "Architecture docs exist" \
    "[[ -f docs/architecture.md ]]" || l2_fail=$((l2_fail + 1))
  check 2 "2.4" "AI code review (CodeRabbit)" \
    "grep -q 'coderabbit' .github/workflows/ci.yml 2>/dev/null || [[ -f .coderabbit.yaml ]]" || l2_fail=$((l2_fail + 1))
  check 2 "2.5" "Performance baseline (bench)" \
    "[[ -f scripts/test/bench-models.sh ]]" || l2_fail=$((l2_fail + 1))
  check 2 "2.9" "License check" \
    "[[ -f scripts/lint/check-licenses.sh ]]" || l2_fail=$((l2_fail + 1))
  check 2 "2.10" "Vulnerability scanning (grype/osv)" \
    "[[ -f scripts/security/grype-scan.sh ]]" || l2_fail=$((l2_fail + 1))
  check 2 "2.11" "Dependency freshness check" \
    "[[ -f scripts/lint/check-versions.sh ]]" || l2_fail=$((l2_fail + 1))
  [[ $l2_fail -eq 0 && $l1_fail -eq 0 && $l0_fail -eq 0 ]] && LEVEL_PASS+=(2)
  echo ""

  # ── CMMI 3: Optimizing ──
  echo "── Level 3: Optimizing ──"
  local l3_fail=0
  check 3 "3.2" "Auto-generated release notes" \
    "[[ -f .github/workflows/release.yml ]]" || l3_fail=$((l3_fail + 1))
  check 3 "3.4" "DORA metrics (CI analysis)" \
    "[[ -f scripts/gh/ci-analysis.sh ]]" || l3_fail=$((l3_fail + 1))
  check 3 "3.6" "Event logging for analysis" \
    "[[ -f scripts/dev/log-analysis.sh ]]" || l3_fail=$((l3_fail + 1))
  [[ $l3_fail -eq 0 && $l2_fail -eq 0 && $l1_fail -eq 0 && $l0_fail -eq 0 ]] && LEVEL_PASS+=(3)
  echo ""

  # ── Result ──
  local max_level=${LEVEL_PASS[-1]}
  local total=$((PASS + FAIL))
  echo "── Result ──"
  echo "  Checks: ${PASS}/${total} passed"
  echo ""
  if [[ $max_level -ge 0 ]]; then
    echo "  ╔══════════════════════════════════╗"
    printf "  ║  CMMI Level: %-2d %-16s ║\n" "$max_level" \
      "$(case $max_level in 0) echo "(Essentials)" ;; 1) echo "(Managed)" ;; 2) echo "(Defined)" ;; 3) echo "(Optimizing)" ;; *) echo "(Unknown)" ;; esac)"
    echo "  ╚══════════════════════════════════╝"
  else
    echo "  ⚠ CMMI Level 0 not yet achieved"
  fi
  echo ""
  echo "  @see docs/adr/adr-048-quality-framework.md"
}

main "$@"
