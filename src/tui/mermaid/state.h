/**
 * @file state.h
 * @brief State diagram renderer — reuses flowchart braille engine.
 *
 * Handles "stateDiagram" and "stateDiagram-v2" mermaid blocks.
 * Converts state transitions to flowchart edges and delegates rendering.
 */
#ifndef LLAMA_CLI_MERMAID_STATE_H
#define LLAMA_CLI_MERMAID_STATE_H

#include "tui/mermaid/renderer.h"

namespace tui {

class StateRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "state"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_STATE_H
