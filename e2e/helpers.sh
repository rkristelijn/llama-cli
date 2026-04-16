#!/bin/bash
# Set mock provider for deterministic E2E tests by default
export LLAMA_PROVIDER="${LLAMA_PROVIDER:-mock}"

# Shared e2e test helpers

# Die with error message
die() {
    echo "FAIL: $1" >&2
    exit 1
}

# Check binary exists and is executable
check_binary() {
    local binary="$1"
    if [ ! -x "$binary" ]; then
        die "binary not found: $binary"
    fi
}

# Assert output is non-empty
assert_nonempty() {
    local output="$1"
    local label="$2"
    if [ -z "$output" ]; then
        die "empty response for: $label"
    fi
}

# Assert output contains expected substring (fixed string, case-insensitive)
assert_contains() {
    local output="$1"
    local expected="$2"
    local label="$3"
    if ! printf '%s' "$output" | grep -F -i -- "$expected" > /dev/null 2>&1; then
        die "expected '$expected' (case-insensitive) in output for: $label"
    fi
}

# Assert command succeeds
assert_success() {
    local cmd="$1"
    local label="$2"
    if ! eval "$cmd" > /dev/null 2>&1; then
        die "command failed: $label"
    fi
}

# Cleanup trap for temp files
setup_cleanup() {
    local temp_file="${1:-}"
    if [ -n "$temp_file" ]; then
        trap 'rm -f "$temp_file"' EXIT
    fi
}
