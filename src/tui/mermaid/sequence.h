/**
 * @file sequence.h
 * @brief Sequence diagram renderer — ASCII columns with arrows.
 *
 * Handles "sequenceDiagram" mermaid blocks. Renders participants as
 * columns with labeled arrows between them.
 */
#ifndef LLAMA_CLI_MERMAID_SEQUENCE_H
#define LLAMA_CLI_MERMAID_SEQUENCE_H

#include "tui/mermaid/renderer.h"

namespace tui {

class SequenceRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "sequence"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_SEQUENCE_H
