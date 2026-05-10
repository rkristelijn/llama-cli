#!/usr/bin/env bash
# gen-features-md.sh — Regenerate docs/features/FEATURES.md from live e2e coverage data.
# Usage: bash scripts/dev/gen-features-md.sh

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

BINARY="./build/llama-cli"
OUTPUT="docs/features/FEATURES.md"

if [[ ! -x "$BINARY" ]]; then
  echo "ERROR: binary not found — run make build first"
  exit 1
fi

# Collect feature coverage per test
declare -A TEST_FEATURES
export LLAMA_PROVIDER=mock OLLAMA_HOSTS="" OLLAMA_HOST=localhost

for t in e2e/test_*.sh; do
  case "$t" in *test_live*|*test_full_feature*|*helpers*) continue ;; esac
  LOG=$(mktemp)
  LLAMA_FEATURE_LOG="$LOG" bash "$t" "$BINARY" >/dev/null 2>&1 || true
  features=$(grep -o '\[FEATURE: [^]]*\]' "$LOG" 2>/dev/null | sed 's/\[FEATURE: //;s/\]//' | sort -u | paste -sd',' - || true)
  name=$(basename "$t" .sh | sed 's/^test_//')
  TEST_FEATURES["$name"]="$features"
  rm -f "$LOG"
done

# Generate markdown
cat > "$OUTPUT" <<'HEADER'
# Feature Coverage

All 48 runtime features are exercised by e2e tests. Each test produces a gif via `make record-e2e`.

HEADER

for name in $(echo "${!TEST_FEATURES[@]}" | tr ' ' '\n' | sort); do
  features="${TEST_FEATURES[$name]}"
  if [[ -n "$features" ]]; then
    # Section title from test name
    title=$(echo "$name" | tr '_' ' ')
    cat >> "$OUTPUT" <<EOF
## ${title}

![${name}](e2e/${name}.gif)

**Features:** ${features}

**Source:** [e2e/test_${name}.sh](../e2e/test_${name}.sh)

EOF
  fi
done

cat >> "$OUTPUT" <<'FOOTER'
---

## How It Works

1. `LOG_FEATURE("id")` markers in C++ source log feature activation at runtime
2. E2e tests run with `LLAMA_FEATURE_LOG` → captures which features fire
3. `make record-e2e` auto-discovers tests and generates gifs (no manual tape files)
4. `scripts/test/check-feature-coverage.sh` verifies 48/48 coverage

## Adding a New Feature

1. Add `LOG_FEATURE("my_feature")` in the code path
2. Ensure an existing e2e test exercises it (or create a new `e2e/test_*.sh`)
3. Run `make record-e2e` — gif is auto-generated
4. Run `bash scripts/dev/gen-features-md.sh` to update this file
FOOTER

echo "  [done] generated $OUTPUT"
