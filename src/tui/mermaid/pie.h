/**
 * @file pie.h
 * @brief Pie chart renderer — horizontal ASCII bar chart.
 *
 * Handles "pie" mermaid blocks. Renders as horizontal bars with
 * percentages since actual pie shapes don't work in terminal.
 */
#ifndef LLAMA_CLI_MERMAID_PIE_H
#define LLAMA_CLI_MERMAID_PIE_H

#include "tui/mermaid/renderer.h"

namespace tui {

class PieRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "pie"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_PIE_H
