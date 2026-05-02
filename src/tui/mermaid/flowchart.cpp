/**
 * @file flowchart.cpp
 * @brief FlowchartRenderer — delegates to existing braille engine in mermaid.cpp.
 */
#include "tui/mermaid/flowchart.h"

#include <sstream>

#include "tui/mermaid/mermaid.h"

namespace tui {

/// Matches "graph" or "flowchart" as the first non-empty line keyword.
bool FlowchartRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 5, "graph") == 0 || input.compare(pos, 9, "flowchart") == 0;
}

bool FlowchartRenderer::render(const std::string& input, std::ostream& out, int cols, int rows) const {
  // Delegate to existing braille renderer
  return render_mermaid(input, out, cols, rows);
}

}  // namespace tui
