#!/usr/bin/env bash
# precommit-check.sh — Auto-fix formatting + secret scan (smart: skips unchanged file types).

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Detect which file types are staged
STAGED=$(git diff --cached --name-only --diff-filter=ACM 2>/dev/null || true)
HAS_CPP=false; HAS_MD=false; HAS_YAML=false; HAS_SH=false

for f in $STAGED; do
  case "$f" in
    *.cpp|*.h) HAS_CPP=true ;;
    *.md) HAS_MD=true ;;
    *.yml|*.yaml) HAS_YAML=true ;;
    *.sh|Makefile) HAS_SH=true ;;
  esac
done

STEP=0
TOTAL=0

# Count steps dynamically
$HAS_CPP && (( TOTAL++ )) || true
$HAS_YAML && (( TOTAL++ )) || true
$HAS_MD && (( TOTAL++ )) || true
$HAS_SH && (( TOTAL++ )) || true
(( TOTAL++ )) || true  # index (always — cheap)
(( TOTAL++ )) || true  # sast-secret (always)

run_step() {
  local name="$1"; shift
  (( STEP++ )) || true
  printf "  [%d/%d] %s... " "${STEP}" "${TOTAL}" "${name}"
  if output=$("$@" 2>&1); then
    printf "✓\n"
  else
    printf "✗\n"
    printf '%s\n' "${output}" | sed 's/^/    /'
    exit 1
  fi
}

echo ""
echo "── Formatting ──"
$HAS_CPP  && run_step "format-code" make -s format-code
$HAS_YAML && run_step "format-yaml" make -s format-yaml
$HAS_MD   && run_step "format-md" make -s format-md
$HAS_SH   && run_step "format-scripts" make -s format-scripts

echo ""
echo "── Documentation ──"
run_step "index" make -s index

echo ""
echo "── Security ──"
run_step "sast-secret" make -s sast-secret

echo ""
echo "All ${TOTAL} checks passed."
