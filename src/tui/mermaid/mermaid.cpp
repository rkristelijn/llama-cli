/**
 * @file mermaid.cpp
 * @brief Mermaid diagram rendering — parse, layout, render pipeline.
 *
 * Self-contained mermaid renderer: parses flowchart syntax, assigns
 * layers via topological sort, renders to braille canvas, outputs to
 * ostream. No external dependencies beyond standard C++.
 *
 * Integrated from ../mermaid-tui/ project and adapted to llama-cli
 * conventions (ostream output, namespace, no global state).
 */
#include "tui/mermaid/mermaid.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstring>
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
  float x = 0, y = 0, w = 0, h = 0;
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

static int parse_node_token(Graph& g, const std::string& token) {
  auto bracket = token.find('[');
  if (bracket == std::string::npos) {
    return g.get_or_add(token);
  }
  std::string id = token.substr(0, bracket);
  auto close = token.find(']', bracket);
  std::string label = token.substr(bracket + 1, close - bracket - 1);
  int idx = g.get_or_add(id);
  g.nodes[idx].label = label;
  return idx;
}

static void parse_edge_line(Graph& g, const std::string& line) {
  std::string remaining = line;
  std::string arrow = "-->";
  std::vector<int> chain;

  while (true) {
    auto pos = remaining.find(arrow);
    std::string token;
    if (pos == std::string::npos) {
      token = trim(remaining);
      if (!token.empty()) {
        chain.push_back(parse_node_token(g, token));
      }
      break;
    }
    token = trim(remaining.substr(0, pos));
    if (!token.empty()) {
      chain.push_back(parse_node_token(g, token));
    }
    remaining = remaining.substr(pos + arrow.size());
  }

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
      // Accept "graph TD", "graph LR", "flowchart TD", etc.
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

// --- Layout ---

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

static void layout(Graph& g, int dw, int dh) {
  if (g.nodes.empty()) {
    return;
  }
  int max_layer = assign_layers(g);
  int layer_count = max_layer + 1;

  std::vector<int> layer_sizes(layer_count, 0);
  for (auto& nd : g.nodes) {
    nd.order = layer_sizes[nd.layer]++;
  }

  constexpr float NODE_W_FRAC = 0.6f;
  constexpr float NODE_H_FRAC = 0.5f;

  for (auto& nd : g.nodes) {
    int count = layer_sizes[nd.layer];
    if (g.dir == Direction::TD) {
      float cell_h = static_cast<float>(dh) / layer_count;
      float cell_w = static_cast<float>(dw) / count;
      nd.w = cell_w * NODE_W_FRAC;
      nd.h = cell_h * NODE_H_FRAC;
      nd.x = cell_w * nd.order + (cell_w - nd.w) / 2;
      nd.y = cell_h * nd.layer + (cell_h - nd.h) / 2;
    } else {
      float cell_w = static_cast<float>(dw) / layer_count;
      float cell_h = static_cast<float>(dh) / count;
      nd.w = cell_w * NODE_W_FRAC;
      nd.h = cell_h * NODE_H_FRAC;
      nd.x = cell_w * nd.layer + (cell_w - nd.w) / 2;
      nd.y = cell_h * nd.order + (cell_h - nd.h) / 2;
    }
  }
}

// --- Canvas (braille) ---

static constexpr int MAX_COLS = 400;
static constexpr int MAX_ROWS = 200;

struct Canvas {
  unsigned char dots[MAX_ROWS][MAX_COLS] = {};
  int dw = 0;
  int dh = 0;
  int term_cols = 0;
  int term_rows = 0;

  void init(int cols, int rows) {
    term_cols = cols;
    term_rows = rows;
    dw = cols * 2;
    dh = rows * 4;
    std::memset(dots, 0, sizeof(dots));
  }

  void dot(int dr, int dc) {
    if (dr >= 0 && dr < dh && dc >= 0 && dc < dw) {
      dots[dr][dc] = 1;
    }
  }

  void line(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    int steps = static_cast<int>(std::sqrt(dx * dx + dy * dy) * 2) + 1;
    for (int i = 0; i <= steps; i++) {
      float t = static_cast<float>(i) / steps;
      dot(static_cast<int>(y0 + dy * t), static_cast<int>(x0 + dx * t));
    }
  }

  void rect(float x, float y, float w, float h) {
    line(x, y, x + w, y);
    line(x + w, y, x + w, y + h);
    line(x + w, y + h, x, y + h);
    line(x, y + h, x, y);
  }

  /// Render to ostream as braille characters
  void print(std::ostream& out) const {
    // Braille dot-to-bit mapping per Unicode standard
    static const int BIT[4][2] = {{0, 3}, {1, 4}, {2, 5}, {6, 7}};
    for (int r = 0; r < term_rows; r++) {
      for (int c = 0; c < term_cols; c++) {
        unsigned int bits = 0;
        for (int dr = 0; dr < 4; dr++) {
          for (int dc = 0; dc < 2; dc++) {
            if (dots[r * 4 + dr][c * 2 + dc]) {
              bits |= (1u << BIT[dr][dc]);
            }
          }
        }
        // Encode U+2800+bits as 3-byte UTF-8
        unsigned int cp = 0x2800 + bits;
        out.put(static_cast<char>(0xE0 | (cp >> 12)));
        out.put(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.put(static_cast<char>(0x80 | (cp & 0x3F)));
      }
      out.put('\n');
    }
  }
};

// --- Render pipeline ---

static void render_edges(const Graph& g, Canvas& canvas) {
  for (const auto& e : g.edges) {
    const Node& from = g.nodes[e.from];
    const Node& to = g.nodes[e.to];
    float x0, y0, x1, y1;
    if (g.dir == Direction::TD) {
      x0 = from.x + from.w / 2;
      y0 = from.y + from.h;
      x1 = to.x + to.w / 2;
      y1 = to.y;
    } else {
      x0 = from.x + from.w;
      y0 = from.y + from.h / 2;
      x1 = to.x;
      y1 = to.y + to.h / 2;
    }
    canvas.line(x0, y0, x1, y1);
  }
}

// --- Public API ---

bool render_mermaid(const std::string& input, std::ostream& out, int cols, int rows) {
  Graph g = parse(input);
  if (g.nodes.empty()) {
    return false;
  }

  // Auto-detect terminal size if not specified
  if (cols <= 0 || rows <= 0) {
    struct winsize w = {};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
      if (cols <= 0) {
        cols = w.ws_col;
      }
    } else {
      if (cols <= 0) {
        cols = 60;
      }
    }
    if (rows <= 0) {
      // Use compact height: ~3 rows per layer, minimum 6
      int layer_count = 1;
      for (const auto& nd : g.nodes) {
        if (nd.layer + 1 > layer_count) {
          layer_count = nd.layer + 1;
        }
      }
      rows = std::max(6, layer_count * 3);
    }
  }

  Canvas canvas;
  canvas.init(cols, rows);
  layout(g, canvas.dw, canvas.dh);

  // Draw nodes and edges
  for (const auto& nd : g.nodes) {
    canvas.rect(nd.x, nd.y, nd.w, nd.h);
  }
  render_edges(g, canvas);
  canvas.print(out);

  // Print node labels below the diagram (braille can't show text)
  // Group nodes by layer, print each layer's labels on one line
  int max_layer = 0;
  for (const auto& nd : g.nodes) {
    if (nd.layer > max_layer) {
      max_layer = nd.layer;
    }
  }
  for (int layer = 0; layer <= max_layer; layer++) {
    std::string label_line(cols, ' ');
    for (const auto& nd : g.nodes) {
      if (nd.layer != layer) {
        continue;
      }
      // Place label at the node's horizontal center (in terminal columns)
      int center_col = static_cast<int>(nd.x + nd.w / 2) / 2;
      int start = center_col - static_cast<int>(nd.label.size()) / 2;
      if (start < 0) {
        start = 0;
      }
      for (size_t i = 0; i < nd.label.size() && start + static_cast<int>(i) < cols; i++) {
        label_line[start + i] = nd.label[i];
      }
    }
    // Only print if there's actual content
    auto last = label_line.find_last_not_of(' ');
    if (last != std::string::npos) {
      out << label_line.substr(0, last + 1) << "\n";
    }
  }

  return true;
}

}  // namespace tui
