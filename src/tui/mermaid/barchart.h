/**
 * @file barchart.h
 * @brief Bar chart renderer — horizontal bars with absolute values.
 */
#ifndef LLAMA_CLI_MERMAID_BARCHART_H
#define LLAMA_CLI_MERMAID_BARCHART_H

#include "tui/mermaid/renderer.h"

namespace tui {

class BarChartRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "barchart"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_BARCHART_H
