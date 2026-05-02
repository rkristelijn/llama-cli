/**
 * @file state.cpp
 * @brief State diagram renderer — converts state transitions to flowchart.
 *
 * Parses "stateDiagram-v2" syntax (State1 --> State2) and rewrites it
 * as "graph TD" with edges, then delegates to the existing braille engine.
 * This avoids duplicating the layout/render pipeline.
 */
#include "tui/mermaid/state.h"

#include <sstream>
#include <string>

#include "tui/mermaid/mermaid.h"

namespace tui {

static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

bool StateRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 12, "stateDiagram") == 0;
}

bool StateRenderer::render(const std::string& input, std::ostream& out, int cols, int rows) const {
  // Convert state diagram syntax to flowchart syntax:
  //   "State1 --> State2" becomes "State1 --> State2" (same arrow syntax)
  //   "[*]" becomes "START" or "END"
  // Then delegate to the existing braille flowchart renderer.
  std::ostringstream flowchart;
  flowchart << "graph TD\n";

  std::istringstream stream(input);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (!header_found) {
      if (line.find("stateDiagram") != std::string::npos) {
        header_found = true;
      }
      continue;
    }
    // Skip direction, note, and state descriptions
    if (line.find("direction") == 0 || line.find("note") == 0 || line.find("state") == 0) {
      continue;
    }
    // Convert transitions: replace [*] with special node names
    if (line.find("-->") != std::string::npos) {
      std::string converted = line;
      // Replace [*] on left side with START, on right with END
      auto arrow_pos = converted.find("-->");
      std::string left = trim(converted.substr(0, arrow_pos));
      std::string right = trim(converted.substr(arrow_pos + 3));
      // Strip colon labels (State1 --> State2 : label)
      auto colon = right.find(':');
      if (colon != std::string::npos) {
        right = trim(right.substr(0, colon));
      }
      if (left == "[*]") {
        left = "START";
      }
      if (right == "[*]") {
        right = "END";
      }
      flowchart << "    " << left << " --> " << right << "\n";
    }
  }

  std::string fc = flowchart.str();
  return render_mermaid(fc, out, cols, rows);
}

}  // namespace tui
