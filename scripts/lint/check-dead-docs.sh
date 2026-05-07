#!/usr/bin/env bash
#
# check-dead-docs.sh — Detect unreferenced docs, configs, and backlog items.
#
# Checks:
#   1. ADR docs not referenced from their README, code, or other docs
#   2. Config files not referenced from Makefile, scripts, or CI
#   3. Top-level docs not linked from any README or INDEX
#
# Warning-only: does not fail the build.
#
# Usage:
#   bash scripts/lint/check-dead-docs.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

main() {
  echo "==> checking for dead docs and configs..."
  local count=0

  # 1. ADRs not referenced anywhere (README counts as a valid reference)
  for adr in docs/adr/adr-*.md; do
    [[ -f "$adr" ]] || continue
    local base
    base=$(basename "$adr")
    # Referenced if filename appears in any file other than itself
    if ! grep -rl "$base" docs/ src/ scripts/ Makefile .github/ README.md CONTRIBUTING.md 2>/dev/null |
      grep -v "$adr" >/dev/null 2>&1; then
      echo "  [warn] unreferenced ADR: $adr"
      count=$((count + 1))
    fi
  done

  # 2. Config files not referenced from scripts, Makefile, or CI
  for cfg in .config/*; do
    [[ -f "$cfg" ]] || continue
    local base
    base=$(basename "$cfg")
    # Skip dynamic files (used at runtime, not statically referenced)
    [[ "$base" == "lessons.yml" || "$base" == "patterns.yml" ]] && continue
    if ! grep -rl "$base" scripts/ Makefile .github/ src/ CMakeLists.txt 2>/dev/null >/dev/null 2>&1; then
      echo "  [warn] unreferenced config: $cfg"
      count=$((count + 1))
    fi
  done

  # 3. Top-level docs not linked from any README, INDEX, or other doc
  for doc in docs/*.md; do
    [[ -f "$doc" ]] || continue
    local base
    base=$(basename "$doc")
    [[ "$base" == "README.md" ]] && continue
    if ! grep -rl "$base" docs/ README.md CONTRIBUTING.md 2>/dev/null |
      grep -v "$doc" >/dev/null 2>&1; then
      echo "  [warn] unreferenced doc: $doc"
      count=$((count + 1))
    fi
  done

  if [[ $count -eq 0 ]]; then
    echo "  ✓ no dead docs or configs detected"
  else
    echo "  [${count} items — review and link or remove]"
  fi
  echo "  [done] dead-docs"
}

main "$@"
