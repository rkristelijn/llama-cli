/**
 * @file quadrant.h
 * @brief Quadrant chart renderer — 2x2 grid with items.
 *
 * Handles "quadrantChart" mermaid blocks. Renders as a 2x2 grid
 * with axis labels and positioned items.
 */
#ifndef LLAMA_CLI_MERMAID_QUADRANT_H
#define LLAMA_CLI_MERMAID_QUADRANT_H

#include "tui/mermaid/renderer.h"

namespace tui {

class QuadrantRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "quadrant"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_QUADRANT_H
