/**
 * @file renderer.cpp
 * @brief DiagramRegistry implementation and global instance.
 *
 * The global registry is lazily initialized with all built-in renderers
 * on first access. Order: flowchart, sequence, pie, state.
 */
#include "tui/mermaid/renderer.h"

#include "tui/mermaid/flowchart.h"
#include "tui/mermaid/pie.h"
#include "tui/mermaid/sequence.h"
#include "tui/mermaid/state.h"

namespace tui {

void DiagramRegistry::add(std::unique_ptr<DiagramRenderer> renderer) { renderers_.push_back(std::move(renderer)); }

bool DiagramRegistry::render(const std::string& input, std::ostream& out, int cols, int rows) const {
  for (const auto& r : renderers_) {
    if (r->can_render(input)) {
      return r->render(input, out, cols, rows);
    }
  }
  return false;
}

/// Lazily construct the global registry with all built-in renderers.
/// Flowchart first (most common), then sequence, pie, state.
DiagramRegistry& diagram_registry() {
  static DiagramRegistry reg = []() {
    DiagramRegistry r;
    r.add(std::make_unique<FlowchartRenderer>());
    r.add(std::make_unique<SequenceRenderer>());
    r.add(std::make_unique<PieRenderer>());
    r.add(std::make_unique<StateRenderer>());
    return r;
  }();
  return reg;
}

}  // namespace tui
