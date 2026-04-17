#!/usr/bin/env bash
#
# log-viewer.sh — Read and display event logs in human-readable format
# Usage: ./log-viewer.sh [filter] [--context N]
#   filter   — Show only events matching this string (default: show all)
#   --context N — Show N lines before/after match (default: 2)

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi


set -e

LOG_FILE="${LLAMA_LOG_FILE:-$HOME/.llama-cli/events.jsonl}"
CONTEXT=2
FILTER=""

# Parse args
while [ $# -gt 0 ]; do
    case "$1" in
        --context)
            CONTEXT="$2"
            shift 2
            ;;
        --context=*)
            CONTEXT="${1#*=}"
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [filter] [--context N]"
            echo ""
            echo "Read and display event logs in human-readable format."
            echo ""
            echo "Environment:"
            echo "  LLAMA_LOG_FILE  — Path to log file (default: ~/.llama-cli/events.jsonl)"
            echo ""
            echo "Options:"
            echo "  filter          — Show only events matching this string"
            echo "  --context N     — Show N lines before/after match (default: 2)"
            exit 0
            ;;
        *)
            FILTER="$1"
            shift
            ;;
    esac
done

if [ ! -f "$LOG_FILE" ]; then
    echo "No log file found: $LOG_FILE"
    echo "Set LLAMA_LOG_FILE or ensure events are being logged."
    exit 1
fi

# Format a single JSON line to human-readable
format_line() {
    line="$1"
    ts=""
    agent=""
    action=""
    duration=""
    tokens=""

    # Extract fields using grep/sed (portable, no jq dependency)
    ts=$(echo "$line" | grep -o '"timestamp":"[^"]*"' | sed 's/.*:"\([^"]*\)"/\1/' | cut -d'T' -f2 | cut -d'.' -f1)
    agent=$(echo "$line" | grep -o '"agent":"[^"]*"' | sed 's/.*:"\([^"]*\)"/\1/')
    action=$(echo "$line" | grep -o '"action":"[^"]*"' | sed 's/.*:"\([^"]*\)"/\1/')
    duration=$(echo "$line" | grep -o '"duration_ms":[0-9]*' | sed 's/.*://')
    tokens=$(echo "$line" | grep -o '"tokens_[a-z]*":[0-9]*' | sed 's/.*://' | tr '\n' ' ')

    # Format: [HH:MM:SS] AGENT ACTION | DURATION | TOKENS
    printf "[%s] %-10s %-12s" "$ts" "$agent" "$action"
    if [ -n "$duration" ]; then
        printf " | %4sms" "$duration"
    fi
    if [ -n "$tokens" ]; then
        printf " | %s" "$tokens"
    fi
    echo ""

    # Show input/output truncated
    input=$(echo "$line" | grep -o '"input":"[^"]*"' | sed 's/.*:"\([^"]*\)"/\1/' | cut -c1-60)
    output=$(echo "$line" | grep -o '"output":"[^"]*"' | sed 's/.*:"\([^"]*\)"/\1/' | cut -c1-60)

    if [ -n "$input" ]; then
        echo "  input:  $input"
    fi
    if [ -n "$output" ]; then
        echo "  output: $output"
    fi
}

# Main: filter and display
if [ -z "$FILTER" ]; then
    # Show all, last 50 lines
    tail -50 "$LOG_FILE" | while IFS= read -r line; do
        format_line "$line"
        echo "---"
    done
else
    # Filter with context using grep -B -A
    grep -- "$FILTER" "$LOG_FILE" | while IFS= read -r line; do
        format_line "$line"
        echo "---"
    done
fi
