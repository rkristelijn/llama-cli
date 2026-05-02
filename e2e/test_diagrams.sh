#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# e2e/test_diagrams.sh — Visual demo + assertion test for all diagram renderers.
# Usage: bash e2e/test_diagrams.sh [build/llama-cli]
# Verifies that each mermaid diagram type renders correctly in the TUI.

BINARY="${1:-./build/llama-cli}"
BUILD_DIR="${BINARY%/*}"
PASS=0
FAIL=0

# We test via the StreamRenderer by feeding markdown with mermaid blocks
# through the binary in sync mode. But since that requires a running model,
# we use the test_diagram binary directly for assertions.
TEST_BIN="${BUILD_DIR}/test_diagram"

assert_contains() {
  local label="$1"
  local haystack="$2"
  local needle="$3"
  if echo "$haystack" | grep -qF "$needle"; then
    echo "  ✓ $label"
    PASS=$((PASS + 1))
  else
    echo "  ✗ $label — expected '$needle'"
    FAIL=$((FAIL + 1))
  fi
}

assert_not_contains() {
  local label="$1"
  local haystack="$2"
  local needle="$3"
  if echo "$haystack" | grep -qF "$needle"; then
    echo "  ✗ $label — should NOT contain '$needle'"
    FAIL=$((FAIL + 1))
  else
    echo "  ✓ $label"
    PASS=$((PASS + 1))
  fi
}

# Build a small test helper that outputs rendered diagrams
DEMO_SRC=$(mktemp /tmp/diagram_e2e_XXXX.cpp)
DEMO_BIN=$(mktemp /tmp/diagram_e2e_XXXX)

cat > "$DEMO_SRC" <<'CPP'
#include <iostream>
#include <sstream>
#include "tui/mermaid/renderer.h"

int main(int argc, char* argv[]) {
  auto& reg = tui::diagram_registry();
  std::string type = argc > 1 ? argv[1] : "all";
  int cols = 70;

  if (type == "flowchart_td" || type == "all") {
    std::ostringstream o;
    reg.render("graph TD\n    A[Start] --> B[Process] --> C[End]\n", o, cols);
    std::cout << "---FLOWCHART_TD---\n" << o.str() << "---END---\n";
  }
  if (type == "flowchart_lr" || type == "all") {
    std::ostringstream o;
    reg.render("graph LR\n    A[Input] --> B[Parse] --> C[Output]\n", o, cols);
    std::cout << "---FLOWCHART_LR---\n" << o.str() << "---END---\n";
  }
  if (type == "sequence" || type == "all") {
    std::ostringstream o;
    reg.render("sequenceDiagram\n    Alice->>Bob: Hello\n    Bob-->>Alice: Hi back\n", o, cols);
    std::cout << "---SEQUENCE---\n" << o.str() << "---END---\n";
  }
  if (type == "pie" || type == "all") {
    std::ostringstream o;
    reg.render("pie title Languages\n    \"C++\" : 60\n    \"Python\" : 40\n", o, cols);
    std::cout << "---PIE---\n" << o.str() << "---END---\n";
  }
  if (type == "state" || type == "all") {
    std::ostringstream o;
    reg.render("stateDiagram-v2\n    [*] --> Active\n    Active --> [*]\n", o, cols);
    std::cout << "---STATE---\n" << o.str() << "---END---\n";
  }
  if (type == "venn" || type == "all") {
    std::ostringstream o;
    reg.render("venn title Sets\n    \"A\" : 50\n    \"B\" : 50\n", o, cols);
    std::cout << "---VENN---\n" << o.str() << "---END---\n";
  }
  if (type == "unsupported" || type == "all") {
    std::ostringstream o;
    bool ok = reg.render("classDiagram\n    class Animal\n", o, cols);
    std::cout << "---UNSUPPORTED---\n" << (ok ? "rendered" : "fallback") << "\n---END---\n";
  }
  if (type == "gantt" || type == "all") {
    std::ostringstream o;
    reg.render("gantt\n    title Plan\n    section Dev\n    Coding :a1, 2024-01-01, 10d\n    Testing :a2, after a1, 5d\n", o, cols);
    std::cout << "---GANTT---\n" << o.str() << "---END---\n";
  }
  if (type == "mindmap" || type == "all") {
    std::ostringstream o;
    reg.render("mindmap\n  root((Topic))\n    Branch A\n      Leaf 1\n      Leaf 2\n    Branch B\n", o, cols);
    std::cout << "---MINDMAP---\n" << o.str() << "---END---\n";
  }
  if (type == "quadrant" || type == "all") {
    std::ostringstream o;
    reg.render("quadrantChart\n    title Priority\n    x-axis Low --> High\n    y-axis Low --> High\n    quadrant-1 Do\n    quadrant-2 Plan\n    quadrant-3 Delegate\n    quadrant-4 Drop\n    Task A: [0.2, 0.8]\n    Task B: [0.7, 0.3]\n", o, cols);
    std::cout << "---QUADRANT---\n" << o.str() << "---END---\n";
  }
  if (type == "timeline" || type == "all") {
    std::ostringstream o;
    reg.render("timeline\n    title History\n    section 2024\n      Jan : Start\n      Jun : Release\n", o, cols);
    std::cout << "---TIMELINE---\n" << o.str() << "---END---\n";
  }
  if (type == "kanban" || type == "all") {
    std::ostringstream o;
    reg.render("kanban\n    title Board\n    column Todo\n      Task A\n      Task B\n    column Done\n      Task C\n", o, cols);
    std::cout << "---KANBAN---\n" << o.str() << "---END---\n";
  }
  if (type == "barchart" || type == "all") {
    std::ostringstream o;
    reg.render("barchart\n    title Metrics\n    \"Alpha\" : 100\n    \"Beta\" : 75\n    \"Gamma\" : 50\n", o, cols);
    std::cout << "---BARCHART---\n" << o.str() << "---END---\n";
  }
  if (type == "orgchart" || type == "all") {
    std::ostringstream o;
    reg.render("orgchart\n    Boss\n      Lead A\n        Dev 1\n      Lead B\n", o, cols);
    std::cout << "---ORGCHART---\n" << o.str() << "---END---\n";
  }
  return 0;
}
CPP

echo "==> Building diagram e2e test..."
# shellcheck disable=SC2046
c++ -std=c++17 -I src -o "$DEMO_BIN" "$DEMO_SRC" \
  src/tui/mermaid/renderer.cpp src/tui/mermaid/flowchart.cpp \
  src/tui/mermaid/sequence.cpp src/tui/mermaid/pie.cpp \
  src/tui/mermaid/state.cpp src/tui/mermaid/venn.cpp \
  src/tui/mermaid/gantt.cpp src/tui/mermaid/mindmap.cpp \
  src/tui/mermaid/quadrant.cpp src/tui/mermaid/timeline.cpp \
  src/tui/mermaid/kanban.cpp src/tui/mermaid/barchart.cpp \
  src/tui/mermaid/orgchart.cpp src/tui/mermaid/mermaid.cpp \
  src/tui/markdown.cpp src/tui/markdown_stream.cpp \
  src/tui/highlight.cpp src/tui/table.cpp

OUTPUT=$("$DEMO_BIN" all)

echo ""
echo "==> Testing diagram renderers"
echo ""

# --- Flowchart TD ---
echo "[Flowchart TD]"
FC_TD=$(echo "$OUTPUT" | sed -n '/---FLOWCHART_TD---/,/---END---/p')
assert_contains "labels in boxes" "$FC_TD" "Start"
assert_contains "labels in boxes" "$FC_TD" "Process"
assert_contains "labels in boxes" "$FC_TD" "End"
assert_contains "box-drawing top-left" "$FC_TD" "┌"
assert_contains "box-drawing bottom-right" "$FC_TD" "┘"
assert_contains "vertical connector" "$FC_TD" "│"

# --- Flowchart LR ---
echo "[Flowchart LR]"
FC_LR=$(echo "$OUTPUT" | sed -n '/---FLOWCHART_LR---/,/---END---/p')
assert_contains "labels in boxes" "$FC_LR" "Input"
assert_contains "labels in boxes" "$FC_LR" "Parse"
assert_contains "labels in boxes" "$FC_LR" "Output"
assert_contains "arrow connector" "$FC_LR" "▶"

# --- Sequence ---
echo "[Sequence]"
SEQ=$(echo "$OUTPUT" | sed -n '/---SEQUENCE---/,/---END---/p')
assert_contains "participant Alice" "$SEQ" "Alice"
assert_contains "participant Bob" "$SEQ" "Bob"
assert_contains "message label" "$SEQ" "Hello"
assert_contains "return message" "$SEQ" "Hi back"
assert_contains "lifeline" "$SEQ" "|"

# --- Pie ---
echo "[Pie]"
PIE=$(echo "$OUTPUT" | sed -n '/---PIE---/,/---END---/p')
assert_contains "title" "$PIE" "Languages"
assert_contains "label C++" "$PIE" "C++"
assert_contains "label Python" "$PIE" "Python"
assert_contains "percentage" "$PIE" "60%"
assert_contains "percentage" "$PIE" "40%"

# --- State ---
echo "[State]"
STATE=$(echo "$OUTPUT" | sed -n '/---STATE---/,/---END---/p')
assert_contains "state label" "$STATE" "Active"
assert_contains "start marker" "$STATE" "START"
assert_contains "end marker" "$STATE" "END"
assert_contains "box-drawing" "$STATE" "┌"

# --- Venn ---
echo "[Venn]"
VENN=$(echo "$OUTPUT" | sed -n '/---VENN---/,/---END---/p')
assert_contains "title" "$VENN" "Sets"
assert_contains "legend A" "$VENN" "A (50)"
assert_contains "legend B" "$VENN" "B (50)"

# --- Unsupported ---
echo "[Unsupported]"
UNSUP=$(echo "$OUTPUT" | sed -n '/---UNSUPPORTED---/,/---END---/p')
assert_contains "graceful fallback" "$UNSUP" "fallback"

# --- Gantt ---
echo "[Gantt]"
GANTT=$(echo "$OUTPUT" | sed -n '/---GANTT---/,/---END---/p')
assert_contains "title" "$GANTT" "Plan"
assert_contains "task label" "$GANTT" "Coding"
assert_contains "task label" "$GANTT" "Testing"
assert_contains "section header" "$GANTT" "Dev"

# --- Mindmap ---
echo "[Mindmap]"
MIND=$(echo "$OUTPUT" | sed -n '/---MINDMAP---/,/---END---/p')
assert_contains "root label" "$MIND" "Topic"
assert_contains "branch" "$MIND" "Branch A"
assert_contains "leaf" "$MIND" "Leaf 1"
assert_contains "tree connector" "$MIND" "├"
assert_contains "last connector" "$MIND" "└"

# --- Quadrant ---
echo "[Quadrant]"
QUAD=$(echo "$OUTPUT" | sed -n '/---QUADRANT---/,/---END---/p')
assert_contains "title" "$QUAD" "Priority"
assert_contains "quadrant label" "$QUAD" "Do"
assert_contains "quadrant label" "$QUAD" "Plan"
assert_contains "item" "$QUAD" "Task A"
assert_contains "item" "$QUAD" "Task B"

# --- Timeline ---
echo "[Timeline]"
TL=$(echo "$OUTPUT" | sed -n '/---TIMELINE---/,/---END---/p')
assert_contains "title" "$TL" "History"
assert_contains "section" "$TL" "2024"
assert_contains "event date" "$TL" "Jan"
assert_contains "event description" "$TL" "Start"
assert_contains "event marker" "$TL" "●"

# --- Kanban ---
echo "[Kanban]"
KB=$(echo "$OUTPUT" | sed -n '/---KANBAN---/,/---END---/p')
assert_contains "title" "$KB" "Board"
assert_contains "column header" "$KB" "Todo"
assert_contains "column header" "$KB" "Done"
assert_contains "item" "$KB" "Task A"
assert_contains "item" "$KB" "Task C"
assert_contains "box border" "$KB" "┌"

# --- Bar Chart ---
echo "[Bar Chart]"
BC=$(echo "$OUTPUT" | sed -n '/---BARCHART---/,/---END---/p')
assert_contains "title" "$BC" "Metrics"
assert_contains "label" "$BC" "Alpha"
assert_contains "label" "$BC" "Beta"
assert_contains "absolute value" "$BC" "100"
assert_contains "absolute value" "$BC" "75"

# --- Org Chart ---
echo "[Org Chart]"
OC=$(echo "$OUTPUT" | sed -n '/---ORGCHART---/,/---END---/p')
assert_contains "root" "$OC" "Boss"
assert_contains "child" "$OC" "Lead A"
assert_contains "grandchild" "$OC" "Dev 1"
assert_contains "box-drawing" "$OC" "┌"

# --- Visual demo output ---
echo ""
echo "==> Visual demo (for manual inspection):"
echo ""
echo "$OUTPUT" | grep -v "^---"

# --- Summary ---
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Results: $PASS passed, $FAIL failed"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Cleanup
rm -f "$DEMO_SRC" "$DEMO_BIN"

if [[ $FAIL -gt 0 ]]; then
  exit 1
fi
