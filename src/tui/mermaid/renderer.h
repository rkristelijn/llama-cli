/**
 * @file renderer.h
 * @brief Pluggable diagram renderer interface and registry.
 *
 * Strategy pattern: DiagramRenderer is the interface. Each diagram type
 * (flowchart, sequence, pie, state) registers itself in the global
 * registry. The registry dispatches based on the diagram type keyword
 * found in the first line of mermaid input.
 *
 * @see src/tui/mermaid/mermaid.h
 */
#ifndef LLAMA_CLI_MERMAID_RENDERER_H
#define LLAMA_CLI_MERMAID_RENDERER_H

#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace tui {

/// Base class for all diagram renderers.
/// Each subclass handles one mermaid diagram type.
class DiagramRenderer {
 public:
  virtual ~DiagramRenderer() = default;
  /// Returns true if this renderer can handle the given input.
  /// Checks the first line for the diagram type keyword.
  virtual bool can_render(const std::string& input) const = 0;
  /// Render the diagram to the output stream.
  /// Returns true on success, false if parsing fails.
  virtual bool render(const std::string& input, std::ostream& out, int cols, int rows) const = 0;
  /// Human-readable name for diagnostics
  virtual std::string name() const = 0;
};

/// Registry of diagram renderers — dispatches to the first matching renderer.
class DiagramRegistry {
 public:
  /// Register a renderer (order matters — first match wins)
  void add(std::unique_ptr<DiagramRenderer> renderer);
  /// Try all registered renderers. Returns true if one succeeded.
  bool render(const std::string& input, std::ostream& out, int cols = 0, int rows = 0) const;

 private:
  std::vector<std::unique_ptr<DiagramRenderer>> renderers_;
};

/// Global diagram registry — populated at startup with all built-in renderers.
DiagramRegistry& diagram_registry();

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_RENDERER_H
