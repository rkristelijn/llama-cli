/**
 * @file timeline.h
 * @brief Timeline renderer — vertical event markers.
 *
 * Handles "timeline" mermaid blocks. Renders events as a vertical
 * timeline with markers and descriptions.
 */
#ifndef LLAMA_CLI_MERMAID_TIMELINE_H
#define LLAMA_CLI_MERMAID_TIMELINE_H

#include "tui/mermaid/renderer.h"

namespace tui {

class TimelineRenderer : public DiagramRenderer {
 public:
  bool can_render(const std::string& input) const override;
  bool render(const std::string& input, std::ostream& out, int cols, int rows) const override;
  std::string name() const override { return "timeline"; }
};

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_TIMELINE_H
