#!/usr/bin/env bash
# record-e2e.sh — Auto-generate VHS recordings for all e2e tests.
# Usage: bash scripts/dev/record-e2e.sh [--force]
# Discovers e2e/*.sh tests and records each as a gif in docs/features/.
# Self-expanding: new tests get recordings automatically.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

source lib/cpm/shell/init.sh 2>/dev/null || true
FORCE="${1:-}"
OUTPUT_DIR="docs/features"
BINARY="./build/llama-cli"

# Ensure VHS is available
if ! command -v vhs >/dev/null 2>&1; then
  echo "ERROR: vhs not installed. Run: brew install charmbracelet/tap/vhs"
  exit 1
fi

# Ensure binary exists
if [[ ! -x "$BINARY" ]]; then
  echo "ERROR: binary not found at $BINARY — run make build first"
  exit 1
fi

mkdir -p "$OUTPUT_DIR"

recorded=0
skipped=0

for test_script in e2e/*.sh; do
  # Same filter as run-e2e.sh: skip live tests and helpers
  case "$test_script" in
  *test_live* | *helpers* | *test_full_feature*) continue ;;
  esac

  # Derive name: e2e/test_foo.sh -> foo
  name=$(basename "$test_script" .sh | sed 's/^test_//')
  gif="$OUTPUT_DIR/${name}.gif"

  # Skip if gif already exists (unless --force)
  if [[ -f "$gif" && "$FORCE" != "--force" ]]; then
    skipped=$((skipped + 1))
    continue
  fi

  echo "  [record] $name"

  # Generate tape inline via stdin
  vhs - <<EOF
Output $gif
Set FontSize 18
Set Width 1100
Set Height 700
Set Shell bash
Set Padding 20

Env PS1 "$ "
Env HOSTNAME "localhost"
Env HOME "/home/user"
Env LLAMA_PROVIDER "mock"
Env OLLAMA_HOSTS ""
Env OLLAMA_HOST "localhost"

Type "bash $test_script $BINARY"
Enter
Sleep 15s
EOF

  recorded=$((recorded + 1))
done

echo "  [done] record-e2e: $recorded recorded, $skipped skipped (use --force to re-record)"
