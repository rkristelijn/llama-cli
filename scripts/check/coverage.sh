#!/usr/bin/env bash
#
# Check that test coverage meets the minimum threshold per source file.
# Uses the test binary that directly tests each source for accurate results.

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi


THRESHOLD=80
BUILD_DIR=build-cov

# Build with coverage
cmake -B "$BUILD_DIR" -S . -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage" > /dev/null 2>&1
cmake --build "$BUILD_DIR" > /dev/null 2>&1

# Run all tests
for t in test_config test_json test_repl test_command test_annotation; do
  ./"$BUILD_DIR"/"$t" > /dev/null 2>&1
done

# Check coverage per source file
fail=0
echo "Coverage per file (minimum: ${THRESHOLD}%):"
for src in config json repl command annotation; do
  gcda="$BUILD_DIR/CMakeFiles/test_${src}.dir/src/${src}.cpp.gcda"
  if [ ! -f "$gcda" ]; then continue; fi
  pct=$(gcov -n "$gcda" 2>&1 | grep -A1 "src/${src}.cpp" | grep "Lines" | head -1 | grep -o '[0-9]*\.[0-9]*' | head -1 | cut -d. -f1)
  if [ -z "$pct" ]; then pct=0; fi
  printf "  %-30s %s%%\n" "src/${src}.cpp" "$pct"
  if [ "$pct" -lt "$THRESHOLD" ]; then
    echo "  FAIL: src/${src}.cpp is below ${THRESHOLD}%"
    fail=1
  fi
done

rm -rf "$BUILD_DIR"

if [ "$fail" -eq 1 ]; then
  echo "FAIL: one or more files below ${THRESHOLD}% coverage"
  exit 1
fi

echo "PASS"
