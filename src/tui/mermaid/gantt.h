/**
 * @file gantt.h
 * @brief Gantt chart renderer — horizontal time bars.
 *
 * Handles "gantt" mermaid blocks. Renders tasks as horizontal bars
 * with labels, showing relative duration and ordering.
 */
#ifndef LLAMA_CLI_MERMAID_GANTT_H
#define LLAMA_CLI_MERMAID_GANTT_H

#include "tui/mermaid/renderer.h"

namespace tui {

class GanttRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "gantt"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_GANTT_H
