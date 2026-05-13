#!/usr/bin/env bash
# junit.sh — JUnit XML renderer for cpm check results.
#
# Usage (called by cpm-check.sh at the end of a run):
#   source lib/cpm/shell/junit.sh
#   junit_start "cpm"
#   junit_testcase "format-code" "success" "523" ""
#   junit_testcase "complexity" "warning" "6500" "src/config.cpp:78 load_dotenv complexity=49"
#   junit_finish
#
# Output: .tmp/reports/cpm-junit.xml
#
# @see docs/adr/adr-121-cpm-quality-layer.md

JUNIT_DIR="${CPM_LOG_DIR:-.tmp}/reports"
mkdir -p "$JUNIT_DIR"

_junit_file=""
_junit_tests=0
_junit_failures=0
_junit_time=0
_junit_cases=""

junit_start() {
  local suite_name="$1"
  _junit_file="$JUNIT_DIR/${suite_name}-junit.xml"
  _junit_tests=0
  _junit_failures=0
  _junit_time=0
  _junit_cases=""
}

junit_testcase() {
  local name="$1"
  local status="$2"       # success | error | warning | skip
  local duration_ms="$3"
  local message="$4"      # failure message (empty if success)

  _junit_tests=$((_junit_tests + 1))
  local time_sec
  time_sec=$(echo "scale=3; $duration_ms / 1000" | bc 2>/dev/null || echo "0.000")
  _junit_time=$(echo "$_junit_time + $time_sec" | bc 2>/dev/null || echo "0")

  if [[ "$status" == "error" ]]; then
    _junit_failures=$((_junit_failures + 1))
    _junit_cases+="    <testcase name=\"$name\" time=\"$time_sec\">
      <failure message=\"$name failed\" type=\"$status\"><![CDATA[$message]]></failure>
    </testcase>
"
  elif [[ "$status" == "skip" ]]; then
    _junit_cases+="    <testcase name=\"$name\" time=\"0\">
      <skipped message=\"$message\"/>
    </testcase>
"
  else
    _junit_cases+="    <testcase name=\"$name\" time=\"$time_sec\"/>
"
  fi
}

junit_finish() {
  [[ -z "$_junit_file" ]] && return

  cat > "$_junit_file" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="cpm" tests="$_junit_tests" failures="$_junit_failures" time="$_junit_time">
$_junit_cases  </testsuite>
</testsuites>
EOF
}
