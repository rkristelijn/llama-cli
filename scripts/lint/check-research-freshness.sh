#!/usr/bin/env bash
# check-research-freshness.sh — Warn when research-backed scripts are stale (>30 days).
#
# Reads .config/research-dates.env and compares each date to today.
# When a topic is stale, prints a ready-to-use research prompt.
#
# Usage:
#   bash scripts/lint/check-research-freshness.sh
#
# To update after research:
#   make research-update TOPIC=SLOP_DETECTION
#
# @see docs/adr/adr-120-research-freshness.md

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
DATES_FILE=".config/research-dates.env"
MAX_AGE_DAYS=30
STALE=0

print_header "checking research freshness..."

if [[ ! -f "$DATES_FILE" ]]; then
  print_step "" "$(basename "$0" .sh)" skip "$DATES_FILE not found"
  exit 0
fi

# Portable date diff (works on macOS and Linux)
days_since() {
  local date_str="$1"
  local today_s date_s
  if date -j >/dev/null 2>&1; then
    # macOS/BSD
    today_s=$(date +%s)
    date_s=$(date -j -f "%Y-%m-%d" "$date_str" +%s 2>/dev/null || echo 0)
  else
    # GNU/Linux
    today_s=$(date +%s)
    date_s=$(date -d "$date_str" +%s 2>/dev/null || echo 0)
  fi
  if [[ $date_s -eq 0 ]]; then
    echo 999
    return 0
  fi
  echo $(((today_s - date_s) / 86400))
}

# Research prompts per topic
prompt_for() {
  local topic="$1"
  case "$topic" in
  SLOP_DETECTION)
    cat <<'EOF'
Research AI slop detection: search for new academic papers (arxiv), GitHub repos
(slop-scan, antislop-sampler, slop-guard updates), community discussions (HN, Reddit
r/dotnet r/ExperiencedDevs), and blog posts about AI code patterns. Focus on:
- New detectable patterns in AI-generated code
- Updated word lists / phrase lists
- New tools or approaches
- False positive reports on existing patterns
Update scripts/lint/check-slop.sh and docs/adr/adr-119-slop-detection.md with findings.
EOF
    ;;
  MODEL_GUIDE)
    cat <<'EOF'
Research AI model landscape: check Ollama model library for new releases, benchmark
results (lmsys arena, artificial analysis), hardware requirements updates, and
community recommendations. Focus on:
- New models worth testing locally (7B-32B range)
- Performance/quality changes in existing models
- New quantization formats or speed improvements
Update docs/model-guide.md and docs/model-bench.md with findings.
EOF
    ;;
  SECURITY_TOOLS)
    cat <<'EOF'
Research security tooling: check for new versions of gitleaks, semgrep, trivy, grype,
osv-scanner, trufflehog. Look for new CVE patterns, rule updates, and emerging
security scanning approaches. Focus on:
- Tool version updates and new features
- New vulnerability patterns relevant to C++ CLI tools
- Supply chain security developments
Update .config/versions.env and scripts/security/ as needed.
EOF
    ;;
  PORTABILITY)
    cat <<'EOF'
Research shell portability: check POSIX compatibility tables, Alpine/musl gotchas,
macOS bash/zsh changes, and CI runner updates. Focus on:
- New non-portable patterns found in the wild
- Changes in CI runner default tools (ubuntu-24.04 updates)
- macOS Sequoia/Sonoma shell changes
Update scripts/lint/check-portability.sh with new patterns.
EOF
    ;;
  *)
    echo "No prompt defined for topic: $topic. Add one to check-research-freshness.sh."
    ;;
  esac
}

# Read each entry and check age
while IFS='=' read -r key value; do
  # Skip comments and empty lines
  [[ -z "$key" || "$key" == \#* ]] && continue
  # Strip whitespace
  key=$(echo "$key" | tr -d ' ')
  value=$(echo "$value" | tr -d ' ')

  age=$(days_since "$value")
  if [[ $age -gt $MAX_AGE_DAYS ]]; then
    echo ""
    echo "  ⚠ STALE: $key — last researched $value ($age days ago, max $MAX_AGE_DAYS)"
    echo "  ┌─────────────────────────────────────────────────────────────"
    prompt_for "$key" | sed 's/^/  │ /'
    echo "  └─────────────────────────────────────────────────────────────"
    echo "  After research, run: make research-update TOPIC=$key"
    ((STALE++)) || true
  fi
done <"$DATES_FILE"

echo ""
if [[ $STALE -eq 0 ]]; then
  echo "  ✓ All research topics are fresh (within ${MAX_AGE_DAYS} days)"
else
  echo "  $STALE topic(s) need re-research"
fi
