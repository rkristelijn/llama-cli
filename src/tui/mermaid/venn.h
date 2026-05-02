/**
 * @file venn.h
 * @brief Venn diagram renderer — overlapping ASCII circles.
 *
 * Handles "venn" mermaid blocks. Renders 2-3 overlapping circles
 * with labels using braille characters for smooth curves.
 */
#ifndef LLAMA_CLI_MERMAID_VENN_H
#define LLAMA_CLI_MERMAID_VENN_H

#include "tui/mermaid/renderer.h"

namespace tui {

/// Renders venn diagrams with 2-3 sets as overlapping braille circles.
class VennRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "venn"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_VENN_H
