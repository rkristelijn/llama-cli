/**
 * @file mermaid.cpp
 * @brief Mermaid flowchart rendering — ASCII box-drawing with labels inside.
 *
 * Renders flowcharts using Unicode box-drawing characters (┌─┐│└─┘)
 * with node labels inside the boxes and connecting lines between them.
 * Labels are always readable — no braille-only output.
 *
 * Supports: graph TD/LR, flowchart TD/LR, node shapes []/{}/()/([]),
 * edge labels -->|text|, chained edges A --> B --> C.
 */
#include "tui/mermaid/mermaid.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace tui {

// --- Graph data structures ---

enum class Direction { TD, LR };

struct Node {
  std::string id;
  std::string label;
  int layer = 0;
  int order = 0;
};

struct Edge {
  int from = 0;
  int to = 0;
};

struct Graph {
  Direction dir = Direction::TD;
  std::vector<Node> nodes;
  std::vector<Edge> edges;

  int find_node(const std::string& id) const {
    for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
      if (nodes[i].id == id) {
        return i;
      }
    }
    return -1;
  }

  int get_or_add(const std::string& id) {
    int idx = find_node(id);
    if (idx >= 0) {
      return idx;
    }
    nodes.push_back({id, id});
    return static_cast<int>(nodes.size()) - 1;
  }
};

// --- Parser ---

static std::string trim(const std::string& str) {
  auto start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  auto end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

/// Parse a node token like "A", "A[Label]", "A{Label}", "A(Label)"
static int parse_node_token(Graph& g, const std::string& token) {
  // Find first delimiter: [ { (
  size_t delim = std::string::npos;
  char close_char = 0;
  for (size_t i = 0; i < token.size(); i++) {
    if (token[i] == '[') {
      delim = i;
      close_char = ']';
      break;
    }
    if (token[i] == '{') {
      delim = i;
      close_char = '}';
      break;
    }
    if (token[i] == '(') {
      delim = i;
      close_char = ')';
      break;
    }
  }
  if (delim == std::string::npos) {
    return g.get_or_add(token);
  }
  std::string id = token.substr(0, delim);
  // Find matching close, handling nested parens for ([...])
  auto close = token.find(close_char, delim + 1);
  if (close == std::string::npos) {
    close = token.size() - 1;
  }
  std::string label = token.substr(delim + 1, close - delim - 1);
  // Strip extra delimiters for ([ ]) style
  if (!label.empty() && label.front() == '[' && label.back() == ']') {
    label = label.substr(1, label.size() - 2);
  }
  int idx = g.get_or_add(id);
  g.nodes[idx].label = label;
  return idx;
}

/// Parse an edge line, handling -->|label| syntax and chained edges
static void parse_edge_line(Graph& g, const std::string& line) {
  std::string remaining = line;
  std::vector<int> chain;
  std::vector<std::string> edge_labels;

  while (true) {
    auto pos = remaining.find("-->");
    if (pos == std::string::npos) {
      std::string token = trim(remaining);
      if (!token.empty()) {
        chain.push_back(parse_node_token(g, token));
      }
      break;
    }
    std::string token = trim(remaining.substr(0, pos));
    if (!token.empty()) {
      chain.push_back(parse_node_token(g, token));
    }
    remaining = remaining.substr(pos + 3);

    // Check for edge label: |text|
    std::string edge_label;
    std::string trimmed = trim(remaining);
    if (!trimmed.empty() && trimmed[0] == '|') {
      auto close_pipe = trimmed.find('|', 1);
      if (close_pipe != std::string::npos) {
        edge_label = trimmed.substr(1, close_pipe - 1);
        remaining = trimmed.substr(close_pipe + 1);
      }
    }
    edge_labels.push_back(edge_label);
  }

  // Build edges from chain
  for (size_t i = 1; i < chain.size(); i++) {
    g.edges.push_back({chain[i - 1], chain[i]});
  }
}

static Graph parse(const std::string& input) {
  Graph g;
  std::string normalized = input;
  std::replace(normalized.begin(), normalized.end(), ';', '\n');

  std::istringstream stream(normalized);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (!header_found) {
      if (line.find("graph") == 0 || line.find("flowchart") == 0) {
        if (line.find("LR") != std::string::npos) {
          g.dir = Direction::LR;
        }
        header_found = true;
      }
      continue;
    }
    if (line.find("-->") == std::string::npos) {
      continue;
    }
    parse_edge_line(g, line);
  }
  return g;
}

// --- Layout: assign layers via topological sort ---

static int assign_layers(Graph& g) {
  std::vector<int> in_deg(g.nodes.size(), 0);
  for (const auto& e : g.edges) {
    in_deg[e.to]++;
  }
  std::vector<int> queue;
  for (int i = 0; i < static_cast<int>(g.nodes.size()); i++) {
    if (in_deg[i] == 0) {
      g.nodes[i].layer = 0;
      queue.push_back(i);
    }
  }
  int max_layer = 0;
  for (size_t qi = 0; qi < queue.size(); qi++) {
    int cur = queue[qi];
    for (const auto& e : g.edges) {
      if (e.from != cur) {
        continue;
      }
      int new_layer = g.nodes[cur].layer + 1;
      if (new_layer > g.nodes[e.to].layer) {
        g.nodes[e.to].layer = new_layer;
      }
      if (new_layer > max_layer) {
        max_layer = new_layer;
      }
      in_deg[e.to]--;
      if (in_deg[e.to] == 0) {
        queue.push_back(e.to);
      }
    }
  }
  return max_layer;
}

// --- ASCII box-drawing renderer ---
// Renders nodes as boxes with labels inside, connected by lines.
// Uses ┌─┐│└─┘ for boxes and │ ─ ┬ ┴ for connections.

/// Render a top-down flowchart using box-drawing characters
static void render_td(const Graph& g, std::ostream& out, int max_layer, int cols) {
  // Compute layer sizes and assign order within each layer
  std::vector<int> layer_sizes(max_layer + 1, 0);
  // Need mutable copy for order assignment
  std::vector<Node> nodes = g.nodes;
  for (auto& nd : nodes) {
    nd.order = layer_sizes[nd.layer]++;
  }

  // For each layer, render the boxes side by side
  for (int layer = 0; layer <= max_layer; layer++) {
    // Collect nodes in this layer
    std::vector<int> layer_nodes;
    for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
      if (nodes[i].layer == layer) {
        layer_nodes.push_back(i);
      }
    }
    // Sort by order
    std::sort(layer_nodes.begin(), layer_nodes.end(), [&](int a, int b) { return nodes[a].order < nodes[b].order; });

    int count = static_cast<int>(layer_nodes.size());
    int cell_w = cols / std::max(count, 1);
    // Cap box width to label + padding, min 5
    int max_label = 0;
    for (int ni : layer_nodes) {
      int len = static_cast<int>(nodes[ni].label.size());
      if (len > max_label) {
        max_label = len;
      }
    }
    int box_w = std::min(cell_w - 2, max_label + 4);
    if (box_w < 5) {
      box_w = 5;
    }

    // Top border: ┌───┐
    std::string top_line;
    std::string mid_line;
    std::string bot_line;
    for (int idx = 0; idx < count; idx++) {
      int ni = layer_nodes[idx];
      int lbl_len = static_cast<int>(nodes[ni].label.size());
      int bw = std::max(box_w, lbl_len + 4);
      int pad_left = (cell_w - bw) / 2;

      std::string pad(pad_left, ' ');
      // Top
      top_line += pad + "\xe2\x94\x8c";  // ┌
      for (int i = 0; i < bw - 2; i++) {
        top_line += "\xe2\x94\x80";  // ─
      }
      top_line += "\xe2\x94\x90";  // ┐
      int remaining = cell_w - pad_left - bw;
      if (remaining > 0) {
        top_line += std::string(remaining, ' ');
      }

      // Middle with label
      mid_line += pad + "\xe2\x94\x82";  // │
      int inner = bw - 2;
      int lpad = (inner - lbl_len) / 2;
      int rpad = inner - lbl_len - lpad;
      mid_line += std::string(lpad, ' ') + nodes[ni].label + std::string(rpad, ' ');
      mid_line += "\xe2\x94\x82";  // │
      if (remaining > 0) {
        mid_line += std::string(remaining, ' ');
      }

      // Bottom
      bot_line += pad + "\xe2\x94\x94";  // └
      for (int i = 0; i < bw - 2; i++) {
        bot_line += "\xe2\x94\x80";  // ─
      }
      bot_line += "\xe2\x94\x98";  // ┘
      if (remaining > 0) {
        bot_line += std::string(remaining, ' ');
      }
    }
    out << top_line << "\n" << mid_line << "\n" << bot_line << "\n";

    // Draw connecting lines to next layer
    if (layer < max_layer) {
      // Find edges from this layer to next
      // Draw vertical connector from center of each source node
      std::string conn_line(cols, ' ');
      for (const auto& e : g.edges) {
        if (nodes[e.from].layer != layer) {
          continue;
        }
        // Find center column of source node
        int src_idx = nodes[e.from].order;
        int src_center = src_idx * cell_w + cell_w / 2;
        if (src_center < cols) {
          // Place │ at center — use raw bytes for box-drawing
          conn_line[src_center] = '|';
        }
      }
      // Replace '|' with proper UTF-8 │
      std::string utf_conn;
      for (char c : conn_line) {
        if (c == '|') {
          utf_conn += "\xe2\x94\x82";  // │
        } else {
          utf_conn += c;
        }
      }
      out << utf_conn << "\n";
    }
  }
}

/// Render a left-to-right flowchart using box-drawing characters
static void render_lr(const Graph& g, std::ostream& out, int max_layer, int cols) {
  // Compute layer sizes
  std::vector<int> layer_sizes(max_layer + 1, 0);
  std::vector<Node> nodes = g.nodes;
  for (auto& nd : nodes) {
    nd.order = layer_sizes[nd.layer]++;
  }

  // Find max nodes in any layer (determines height)
  int max_in_layer = *std::max_element(layer_sizes.begin(), layer_sizes.end());

  // For LR: each layer is a column, each node in a layer is a row
  int col_w = cols / (max_layer + 1);
  int box_w = col_w - 4;  // leave room for arrows
  if (box_w < 7) {
    box_w = 7;
  }

  // Render row by row
  for (int row = 0; row < max_in_layer; row++) {
    std::string top_line;
    std::string mid_line;
    std::string bot_line;

    for (int layer = 0; layer <= max_layer; layer++) {
      // Find node at this layer+row
      int ni = -1;
      for (int i = 0; i < static_cast<int>(nodes.size()); i++) {
        if (nodes[i].layer == layer && nodes[i].order == row) {
          ni = i;
          break;
        }
      }
      if (ni < 0) {
        // Empty cell
        top_line += std::string(col_w, ' ');
        mid_line += std::string(col_w, ' ');
        bot_line += std::string(col_w, ' ');
        continue;
      }

      int lbl_len = static_cast<int>(nodes[ni].label.size());
      int bw = std::max(box_w, lbl_len + 4);
      if (bw > col_w - 2) {
        bw = col_w - 2;
      }

      // Top
      top_line += " \xe2\x94\x8c";
      for (int i = 0; i < bw - 2; i++) {
        top_line += "\xe2\x94\x80";
      }
      top_line += "\xe2\x94\x90";
      int rem = col_w - bw - 1;
      if (rem > 0) {
        top_line += std::string(rem, ' ');
      }

      // Mid
      int inner = bw - 2;
      std::string lbl = nodes[ni].label;
      if (static_cast<int>(lbl.size()) > inner) {
        lbl = lbl.substr(0, inner - 1) + "\xe2\x80\xa6";  // …
      }
      int lpad = (inner - static_cast<int>(lbl.size())) / 2;
      int rpad = inner - static_cast<int>(lbl.size()) - lpad;
      mid_line += " \xe2\x94\x82";
      mid_line += std::string(lpad, ' ') + lbl + std::string(rpad, ' ');
      mid_line += "\xe2\x94\x82";
      // Arrow to next layer
      if (layer < max_layer) {
        // Check if there's an edge from this node
        bool has_edge = false;
        for (const auto& e : g.edges) {
          if (e.from == ni) {
            has_edge = true;
            break;
          }
        }
        if (has_edge && rem > 1) {
          mid_line += "\xe2\x94\x80\xe2\x94\x80\xe2\x96\xb6";  // ──▶
          rem -= 3;
        }
      }
      if (rem > 0) {
        mid_line += std::string(rem, ' ');
      }

      // Bot
      bot_line += " \xe2\x94\x94";
      for (int i = 0; i < bw - 2; i++) {
        bot_line += "\xe2\x94\x80";
      }
      bot_line += "\xe2\x94\x98";
      int brem = col_w - bw - 1;
      if (brem > 0) {
        bot_line += std::string(brem, ' ');
      }
    }
    out << top_line << "\n" << mid_line << "\n" << bot_line << "\n";
  }
}

// --- Public API ---

bool render_mermaid(const std::string& input, std::ostream& out, int cols, int /*rows*/) {
  Graph g = parse(input);
  if (g.edges.empty()) {
    return false;
  }

  // Auto-detect terminal width
  if (cols <= 0) {
    struct winsize w = {};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
      cols = w.ws_col;
    } else {
      cols = 80;
    }
  }

  int max_layer = assign_layers(g);

  if (g.dir == Direction::TD) {
    render_td(g, out, max_layer, cols);
  } else {
    render_lr(g, out, max_layer, cols);
  }

  return true;
}

}  // namespace tui
