/**
 * @file kanban.h
 * @brief Kanban board renderer — multi-column task board.
 */
#ifndef LLAMA_CLI_MERMAID_KANBAN_H
#define LLAMA_CLI_MERMAID_KANBAN_H

#include "tui/mermaid/renderer.h"

namespace tui {

class KanbanRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "kanban"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_KANBAN_H
