/**
 * @file mindmap.h
 * @brief Mindmap renderer — tree structure with box-drawing lines.
 *
 * Handles "mindmap" mermaid blocks. Renders as indented tree
 * using ├── and └── connectors.
 */
#ifndef LLAMA_CLI_MERMAID_MINDMAP_H
#define LLAMA_CLI_MERMAID_MINDMAP_H

#include "tui/mermaid/renderer.h"

namespace tui {

class MindmapRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "mindmap"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_MINDMAP_H
