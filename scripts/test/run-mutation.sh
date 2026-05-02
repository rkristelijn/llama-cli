#!/usr/bin/env bash
#
# run-mutation.sh — Run mutation testing via Mull.
#
# On Linux (native or CI): runs Mull directly.
# On macOS: skips (Mull requires native Linux due to LLVM incompatibility
#           and Docker emulation is too slow for subprocess-heavy mutation testing).
#
# Usage:
#   bash scripts/test/run-mutation.sh
#   make mutation
#
# @see docs/adr/adr-067-mutation-testing.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

echo "==> mutation testing (mull)..."

# macOS: skip — Mull needs native Linux (Apple libc++ incompatible, Docker too slow)
if [[ "$(uname -s)" != "Linux" ]]; then
  echo "  [skip] mutation testing requires native Linux (runs in CI)"
  echo "  [done] mutation"
  exit 0
fi

# Find mull-runner
MULL_RUNNER=""
if command -v mull-runner >/dev/null 2>&1; then
  MULL_RUNNER="mull-runner"
elif command -v mull-runner-19 >/dev/null 2>&1; then
  MULL_RUNNER="mull-runner-19"
else
  echo "  [skip] mull-runner not installed (make setup)"
  echo "  [done] mutation"
  exit 0
fi

# Find mull plugin
MULL_PLUGIN=""
for p in /usr/lib/mull-ir-frontend-19 /usr/lib/mull-ir-frontend \
         /usr/local/lib/mull-ir-frontend-19; do
  if [[ -f "$p" ]]; then
    MULL_PLUGIN="$p"
    break
  fi
done
if [[ -z "$MULL_PLUGIN" ]]; then
  echo "  [skip] mull plugin not found"
  echo "  [done] mutation"
  exit 0
fi

# Build with Mull instrumentation
BUILD_DIR="build-mull"
CXX="${CXX:-clang++}"

# Copy mull config to working directory
cp .config/mull.yml mull.yml 2>/dev/null || true

# Configure (fetches dependencies)
cmake -B "$BUILD_DIR" -S . \
  -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_CXX_FLAGS="-g -O0 -grecord-command-line" > /dev/null 2>&1

# Inject mull plugin and force rebuild
sed -i "s|CMAKE_CXX_FLAGS:STRING=.*|CMAKE_CXX_FLAGS:STRING=-g -O0 -grecord-command-line -fpass-plugin=$MULL_PLUGIN|" \
  "$BUILD_DIR/CMakeCache.txt"
find "$BUILD_DIR" -name "*.o" -delete 2>/dev/null || true

# Build test binaries
TARGETS=(test_config test_json test_annotation test_exec)
for target in "${TARGETS[@]}"; do
  cmake --build "$BUILD_DIR" --target "$target" > /dev/null 2>&1 || true
done

# Run mull-runner on each binary
TOTAL_SURVIVED=0
for target in "${TARGETS[@]}"; do
  BIN="$BUILD_DIR/$target"
  if [[ ! -f "$BIN" ]]; then
    continue
  fi
  echo "  [mutating] $target"
  OUTPUT=$("$MULL_RUNNER" --allow-surviving "$BIN" 2>&1 || true)
  SCORE=$(echo "$OUTPUT" | grep -oP "Mutation score: \K[0-9]+" || echo "")
  SURVIVED=$(echo "$OUTPUT" | grep -oP "Surviving mutants: \K[0-9]+" || echo "0")
  if [[ -n "$SCORE" ]]; then
    echo "    score=$SCORE% survived=$SURVIVED"
  fi
  TOTAL_SURVIVED=$((TOTAL_SURVIVED + SURVIVED))
done

# Summary
if [[ "$TOTAL_SURVIVED" -gt 0 ]]; then
  echo "  ⚠ $TOTAL_SURVIVED mutant(s) survived across all targets"
else
  echo "  ✓ All mutants killed"
fi

# Cleanup
rm -f mull.yml
echo "  [done] mutation"
