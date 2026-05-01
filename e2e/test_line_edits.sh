#!/usr/bin/env bash

# Test line-by-line editing functionality

# Source helpers.sh from the correct path
source e2e/helpers.sh

# Path to the llama-cli executable
LLAMA_CLI_BIN="./build/llama-cli"

# Counter for passed tests
PASSED_TESTS=0
TOTAL_TESTS=0

# Custom test_eq function
test_eq() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    local actual="$1"
    local expected="$2"
    local label="$3"
    if [ "$actual" = "$expected" ]; then
        echo "PASS: $label"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo "FAIL: $label"
        echo "  Expected: '$expected'"
        echo "  Actual:   '$actual'"
        die "Test failed: $label" # Use die from helpers.sh
    fi
}

# Custom report_results function
report_results() {
    echo "--- Test Summary ---"
    echo "Passed: $PASSED_TESTS / $TOTAL_TESTS"
    if [ "$PASSED_TESTS" -eq "$TOTAL_TESTS" ]; then
        echo "All tests passed!"
    else
        die "Some tests failed."
    fi
}


# Setup: Create a temporary test file
echo "Line 1" > test_file.txt
echo "Line 2" >> test_file.txt
echo "Line 3" >> test_file.txt

# Ensure cleanup even on failure
trap 'rm -f test_file.txt' EXIT

# --- Test 1: Add a line in the middle ---
echo "--- Test 1: Add a line in the middle ---"
MOCK_RESPONSE=$(cat <<EOF
<add_line path="test_file.txt" line_number="2" content="Added Line"/>
EOF
)
echo "DEBUG: MOCK_RESPONSE for Test 1: '$MOCK_RESPONSE'"
echo "DEBUG: test_file.txt before llama-cli call:"
cat test_file.txt

export LLAMA_CLI_MOCK_RESPONSE="$MOCK_RESPONSE"
"$LLAMA_CLI_BIN" --provider=mock --sync --capabilities=write "add a line" 2>&1
unset LLAMA_CLI_MOCK_RESPONSE
unset LLAMA_CLI_MOCK_RESPONSE # ADDED 2>&1

echo "DEBUG: test_file.txt after llama-cli call:"
cat test_file.txt

EXPECTED="Line 1
Added Line
Line 2
Line 3"
test_eq "$(cat test_file.txt)" "$EXPECTED" "Add line in middle"

# Reset file for next test
echo "Line 1" > test_file.txt
echo "Line 2" >> test_file.txt
echo "Line 3" >> test_file.txt

# --- Test 2: Add a line at the end ---
echo "--- Test 2: Add a line at the end ---"
MOCK_RESPONSE=$(cat <<EOF
<add_line path="test_file.txt" line_number="4" content="End Line"/>
EOF
)
echo "DEBUG: MOCK_RESPONSE for Test 2: '$MOCK_RESPONSE'"
echo "DEBUG: test_file.txt before llama-cli call:"
cat test_file.txt
export LLAMA_CLI_MOCK_RESPONSE="$MOCK_RESPONSE"
"$LLAMA_CLI_BIN" --provider=mock --sync --capabilities=write "add a line at the end" 2>&1 # ADDED 2>&1

echo "DEBUG: test_file.txt after llama-cli call:"
cat test_file.txt

EXPECTED="Line 1
Line 2
Line 3
End Line"
test_eq "$(cat test_file.txt)" "$EXPECTED" "Add line at end"

# Reset file for next test
echo "Line 1" > test_file.txt
echo "Line 2" >> test_file.txt
echo "Line 3" >> test_file.txt

# --- Test 3: Delete a line ---
echo "--- Test 3: Delete a line ---"
MOCK_RESPONSE=$(cat <<EOF
<delete_line path="test_file.txt" content="Line 2"/>
EOF
)
echo "DEBUG: MOCK_RESPONSE for Test 3: '$MOCK_RESPONSE'"
echo "DEBUG: test_file.txt before llama-cli call:"
cat test_file.txt
export LLAMA_CLI_MOCK_RESPONSE="$MOCK_RESPONSE"
"$LLAMA_CLI_BIN" --provider=mock --sync --capabilities=write "delete line 2" 2>&1 # ADDED 2>&1

echo "DEBUG: test_file.txt after llama-cli call:"
cat test_file.txt

EXPECTED="Line 1
Line 3"
test_eq "$(cat test_file.txt)" "$EXPECTED" "Delete a line"

# Reset file for next test
echo "Line 1" > test_file.txt
echo "Line 2" >> test_file.txt
echo "Line 3" >> test_file.txt

# --- Test 4: Modify a line (delete then add) ---
echo "--- Test 4: Modify a line (delete then add) ---"
MOCK_RESPONSE=$(cat <<EOF
<delete_line path="test_file.txt" content="Line 2"/>
<add_line path="test_file.txt" line_number="2" content="Modified Line 2"/>
EOF
)
echo "DEBUG: MOCK_RESPONSE for Test 4: '$MOCK_RESPONSE'"
echo "DEBUG: test_file.txt before llama-cli call:"
cat test_file.txt
export LLAMA_CLI_MOCK_RESPONSE="$MOCK_RESPONSE"
"$LLAMA_CLI_BIN" --provider=mock --sync --capabilities=write "modify line 2" 2>&1 # ADDED 2>&1

echo "DEBUG: test_file.txt after llama-cli call:"
cat test_file.txt

EXPECTED="Line 1
Modified Line 2
Line 3"
test_eq "$(cat test_file.txt)" "$EXPECTED" "Modify a line"

# Ensure all tests passed
report_results
