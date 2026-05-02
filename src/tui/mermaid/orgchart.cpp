/**
 * @file orgchart.cpp
 * @brief Org chart renderer — converts to flowchart TD and delegates.
 *
 * Custom syntax:
 *   orgchart
 *     CEO
 *       CTO
 *         Dev Lead
 *         QA Lead
 *       CFO
 *         Finance
 *
 * Converts indentation to flowchart edges and delegates to render_mermaid().
 */
#include "tui/mermaid/orgchart.h"

#include <sstream>
#include <string>
#include <vector>

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

bool OrgChartRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 8, "orgchart") == 0;
}

bool OrgChartRenderer::render(const std::string& input, std::ostream& out, int cols, int rows) const {
  // Parse indentation-based hierarchy into flowchart edges
  struct OrgNode {
    std::string label;
    int depth;
  };
  std::vector<OrgNode> nodes;

  std::istringstream stream(input);
  std::string line;
  bool header_found = false;
  int base_indent = -1;

  while (std::getline(stream, line)) {
    if (line.empty()) {
      continue;
    }
    std::string trimmed = trim(line);
    if (trimmed.empty()) {
      continue;
    }
    if (!header_found) {
      if (trimmed.find("orgchart") == 0) {
        header_found = true;
      }
      continue;
    }
    int indent = 0;
    for (char c : line) {
      if (c == ' ') {
        indent++;
      } else if (c == '\t') {
        indent += 2;
      } else {
        break;
      }
    }
    if (base_indent < 0) {
      base_indent = indent;
    }
    int depth = (indent - base_indent) / 2;
    if (depth < 0) {
      depth = 0;
    }
    nodes.push_back({trimmed, depth});
  }

  if (nodes.empty()) {
    return false;
  }

  // Convert to flowchart syntax: find parent for each node
  // Parent is the nearest preceding node with depth - 1
  std::string flowchart = "graph TD\n";
  for (size_t i = 0; i < nodes.size(); i++) {
    if (nodes[i].depth == 0) {
      continue;  // root has no parent
    }
    // Find parent: walk backwards to find depth - 1
    for (int j = static_cast<int>(i) - 1; j >= 0; j--) {
      if (nodes[j].depth == nodes[i].depth - 1) {
        // Create edge: use sanitized IDs
        std::string from_id = "N" + std::to_string(j);
        std::string to_id = "N" + std::to_string(i);
        flowchart += "    " + from_id + "[" + nodes[j].label + "] --> " + to_id + "[" + nodes[i].label + "]\n";
        break;
      }
    }
  }

  // Delegate to flowchart renderer
  return render_mermaid(flowchart, out, cols, rows);
}

}  // namespace tui
