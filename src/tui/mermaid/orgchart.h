/**
 * @file orgchart.h
 * @brief Org chart renderer — hierarchical tree (delegates to flowchart TD).
 */
#ifndef LLAMA_CLI_MERMAID_ORGCHART_H
#define LLAMA_CLI_MERMAID_ORGCHART_H

#include "tui/mermaid/renderer.h"

namespace tui {

class OrgChartRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "orgchart"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_ORGCHART_H
