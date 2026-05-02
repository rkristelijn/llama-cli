/**
 * @file mindmap.cpp
 * @brief Mindmap renderer — tree with box-drawing connectors.
 *
 * Parses mermaid mindmap syntax (indentation-based):
 *   mindmap
 *     root((Central Topic))
 *       Branch A
 *         Leaf 1
 *         Leaf 2
 *       Branch B
 *         Leaf 3
 *
 * Renders as:
 *   Central Topic
 *   ├── Branch A
 *   │   ├── Leaf 1
 *   │   └── Leaf 2
 *   └── Branch B
 *       └── Leaf 3
 */
#include "tui/mermaid/mindmap.h"

#include <sstream>
#include <string>
#include <vector>

namespace tui {

// --- Internal structures ---

struct MindmapNode {
  std::string label;
  int depth = 0;
};

static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

/// Strip mermaid node shape markers: ((text)), (text), [text], {{text}}
static std::string strip_shape(const std::string& s) {
  std::string t = s;
  // ((text)) → text
  if (t.size() >= 4 && t[0] == '(' && t[1] == '(' && t.back() == ')' && t[t.size() - 2] == ')') {
    return t.substr(2, t.size() - 4);
  }
  // (text) → text
  if (t.size() >= 2 && t[0] == '(' && t.back() == ')') {
    return t.substr(1, t.size() - 2);
  }
  // [text] → text
  if (t.size() >= 2 && t[0] == '[' && t.back() == ']') {
    return t.substr(1, t.size() - 2);
  }
  // {{text}} → text
  if (t.size() >= 4 && t[0] == '{' && t[1] == '{' && t.back() == '}' && t[t.size() - 2] == '}') {
    return t.substr(2, t.size() - 4);
  }
  return t;
}

bool MindmapRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 7, "mindmap") == 0;
}

bool MindmapRenderer::render(const std::string& input, std::ostream& out, int /*cols*/, int /*rows*/) const {
  std::vector<MindmapNode> nodes;

  // Parse: indentation determines depth
  std::istringstream stream(input);
  std::string line;
  bool header_found = false;
  int base_indent = -1;

  while (std::getline(stream, line)) {
    if (line.empty()) {
      continue;
    }
    std::string trimmed = trim(line);
    if (trimmed.empty()) {
      continue;
    }
    if (!header_found) {
      if (trimmed.find("mindmap") == 0) {
        header_found = true;
      }
      continue;
    }
    // Calculate indentation (number of leading spaces)
    int indent = 0;
    for (char c : line) {
      if (c == ' ') {
        indent++;
      } else if (c == '\t') {
        indent += 2;
      } else {
        break;
      }
    }
    if (base_indent < 0) {
      base_indent = indent;
    }
    int depth = (indent - base_indent) / 2;
    if (depth < 0) {
      depth = 0;
    }
    // Strip "root" prefix from root node
    std::string label = trimmed;
    if (depth == 0 && label.find("root") == 0) {
      label = label.substr(4);
    }
    nodes.push_back({strip_shape(label), depth});
  }

  if (nodes.empty()) {
    return false;
  }

  // Render as tree with box-drawing connectors
  // For each node, determine if it's the last child at its depth
  for (size_t i = 0; i < nodes.size(); i++) {
    const auto& node = nodes[i];

    if (node.depth == 0) {
      // Root node — print bold
      out << "\033[1m" << node.label << "\033[0m\n";
      continue;
    }

    // Build prefix: for each ancestor level, check if there are more siblings
    std::string prefix;
    for (int d = 1; d < node.depth; d++) {
      // Check if there's a node after i at depth d (meaning the vertical line continues)
      bool has_more = false;
      for (size_t j = i + 1; j < nodes.size(); j++) {
        if (nodes[j].depth < d) {
          break;
        }
        if (nodes[j].depth == d) {
          has_more = true;
          break;
        }
      }
      prefix += has_more ? "\xe2\x94\x82   " : "    ";  // │ or space
    }

    // Check if this is the last sibling at its depth
    bool is_last = true;
    for (size_t j = i + 1; j < nodes.size(); j++) {
      if (nodes[j].depth < node.depth) {
        break;
      }
      if (nodes[j].depth == node.depth) {
        is_last = false;
        break;
      }
    }

    // Connector: └── for last, ├── for others
    if (is_last) {
      prefix += "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 ";  // └──
    } else {
      prefix += "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ";  // ├──
    }

    out << prefix << node.label << "\n";
  }

  return true;
}

}  // namespace tui
