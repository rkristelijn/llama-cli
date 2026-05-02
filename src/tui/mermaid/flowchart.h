/**
 * @file flowchart.h
 * @brief Flowchart diagram renderer — wraps existing braille engine.
 *
 * Handles "graph TD", "graph LR", "flowchart TD", "flowchart LR".
 */
#ifndef LLAMA_CLI_MERMAID_FLOWCHART_H
#define LLAMA_CLI_MERMAID_FLOWCHART_H

#include "tui/mermaid/renderer.h"

namespace tui {

class FlowchartRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "flowchart"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_FLOWCHART_H
