#!/usr/bin/env bash
#
# check-pipeline-coverage.sh — Verify all quality make targets are in CI.
#
# Ensures every check/lint/test/sast target is either:
#   1. Present in .github/workflows/ci.yml
#   2. Explicitly listed in the denylist (with reason)
#
# Usage:
#   bash scripts/lint/check-pipeline-coverage.sh

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
# Targets that are intentionally NOT in CI (with reason)
declare -A DENYLIST=(
  [mutation]="too slow for every PR (30+ min)"
  [summarize - safe]="requires Ollama LLM"
  [summarize]="requires Ollama LLM"
  [sbom]="informational only, not a gate"
  [feature - density]="informational, no hard threshold"
  [live]="requires real LLM (manual/scheduled)"
  [live - full]="requires real LLM (manual/scheduled)"
  [log - analysis]="local dev tool, not a gate"
  [ci - analysis]="local dev tool, not a gate"
  [commit - stats]="local dev tool, not a gate"
  [new]="scaffolding tool, not a gate"
  [gh - pr - ready]="interactive, not a CI job"
  [pre - pr]="aggregator target, not a CI job"
  [cmmi]="audit tool, not a gate"
  [smells]="advisory, not a gate"
  [inclusivity]="advisory, not a gate"
  [licenses]="audit tool, not a gate"
  [build - gcc]="cross-compile check, too slow for every PR (~6 min)"
  [fuzz]="requires LLVM fuzzer build, runs separately"
  [coverage]="covered by test-coverage job (different build)"
  [coverage - report]="covered by test-coverage job"
  [live]="requires running Ollama + real LLM"
  [bench]="benchmark, not a quality gate"
  [preflight]="requires running Ollama"
  [sonar]="requires SONAR_TOKEN, runs via SonarCloud app"
  [sonar - report]="informational query, not a check"
  [sast - codeql]="extremely slow (30+ min), separate workflow"
  [learn]="AI-assisted, not a check"
  [check - all]="aggregator, individual targets are in CI"
  [check]="aggregator"
  [check - fast]="aggregator"
  [full - check]="alias for check-all"
  [check - ai]="condensed output wrapper"
  [lint]="aggregator"
  [lint - code]="aggregator (lint-format-code + lint-cppcheck are in CI)"
  [test]="aggregator"
  [sast]="aggregator"
  [format]="runs in pre-commit, not CI gate"
  [format - code]="runs in pre-commit"
  [format - md]="runs in pre-commit"
  [format - yaml]="runs in pre-commit"
  [format - scripts]="runs in pre-commit"
  [check - theme]="fast, included in lint aggregator — add to CI if violations appear"
  [check - xref]="fast, included in lint aggregator — add to CI if violations appear"
  [consistency]="fast, included in lint aggregator — add to CI if violations appear"
  [features]="informational listing, not a gate"
  [sast - iac]="trivy not available in CI runner (install too slow)"
  [sast - osv]="osv-scanner not in CI runner yet"
  [sast - stegano]="zsteg (Ruby gem) not in CI runner"
  [quick]="dev shortcut, not CI"
  [todo]="informational"
  [index]="informational"
  [trivi]="informational"
  [log]="dev tool"
  [update]="dev tool"
  [precommit]="local hook"
  [prepush]="local hook"
  [check - unicode]="fast, included in lint aggregator"
  [check - portability]="fast, included in lint aggregator"
  [check - interactive - input]="fast, included in lint aggregator"
  [check - pii]="fast, included in lint aggregator"
  [slop]="fast, included in lint aggregator"
  [kill]="dev utility, not a check"
  [record - e2e]="requires VHS, local dev tool for gif generation"
  [check - casts]="slow (compiles each file), check-all only"
  [check - conversions]="slow (full rebuild with -Wconversion), check-all only"
  [check - shadowing]="slow (compiles each file), check-all only"
  [check - traceability]="needs feature-registry.yml, check-all only"
  [pipeline - coverage]="meta-check, check-all only"
)

echo "==> checking pipeline coverage..."

CI_FILE=".github/workflows/ci.yml"
if [[ ! -f "$CI_FILE" ]]; then
  echo "  [error] $CI_FILE not found"
  exit 1
fi

# Extract all quality-related make targets from Makefile
# (targets with ## comments, excluding aliases, getting-started, github, help)
targets=$(awk -F: '/^[a-z].*:.*##/ && !/^(setup|build|start|run|install|hooks|clean|help|s|r|t|bump|major|minor|patch|gpls|gps|gpr|gdi|gpf):/' Makefile |
  sed 's/:.*//; s/ .*//' | sort -u)

missing=0
for target in $targets; do
  # Skip GitHub/dev targets
  [[ "$target" == gh-* ]] && continue
  [[ "$target" == create-issue ]] && continue

  # Check denylist
  if [[ -n "${DENYLIST[$target]:-}" ]]; then
    continue
  fi

  # Check if target appears in CI workflow
  if ! grep -q "$target" "$CI_FILE" 2>/dev/null; then
    echo "  [warn] $target — not in CI and not on denylist"
    missing=$((missing + 1))
  fi
done

if [[ $missing -eq 0 ]]; then
  echo "  ✓ all quality targets covered (CI or denylist)"
else
  echo "  [${missing} targets missing — add to CI or denylist in this script]"
  exit 1
fi
echo "  [done] pipeline-coverage"
