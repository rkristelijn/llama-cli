/**
 * @file quadrant.cpp
 * @brief Quadrant chart renderer — 2x2 grid with positioned items.
 *
 * Parses mermaid quadrantChart syntax:
 *   quadrantChart
 *     title Prioritization
 *     x-axis Low Effort --> High Effort
 *     y-axis Low Impact --> High Impact
 *     quadrant-1 Do First
 *     quadrant-2 Schedule
 *     quadrant-3 Delegate
 *     quadrant-4 Eliminate
 *     Item A: [0.8, 0.9]
 *     Item B: [0.2, 0.3]
 *
 * Renders as:
 *              Low Effort ──────────── High Effort
 *   ┌─────────────────────┬─────────────────────┐
 *   │ Do First            │ Schedule             │ High
 *   │  • Item A           │                      │ Impact
 *   ├─────────────────────┼─────────────────────┤
 *   │ Delegate            │ Eliminate            │ Low
 *   │                     │  • Item B            │ Impact
 *   └─────────────────────┴─────────────────────┘
 */
#include "tui/mermaid/quadrant.h"

#include <sstream>
#include <string>
#include <vector>

namespace tui {

struct QuadrantItem {
  std::string label;
  float x = 0.5f;
  float y = 0.5f;
};

static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

bool QuadrantRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 13, "quadrantChart") == 0;
}

bool QuadrantRenderer::render(const std::string& input, std::ostream& out, int cols, int /*rows*/) const {
  std::string title;
  std::string x_left = "Low";
  std::string x_right = "High";
  std::string y_bottom = "Low";
  std::string y_top = "High";
  std::string q1 = "Q1";
  std::string q2 = "Q2";
  std::string q3 = "Q3";
  std::string q4 = "Q4";
  std::vector<QuadrantItem> items;

  // Parse input
  std::istringstream stream(input);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (!header_found) {
      if (line.find("quadrantChart") == 0) {
        header_found = true;
      }
      continue;
    }
    if (line.find("title") == 0) {
      title = trim(line.substr(5));
    } else if (line.find("x-axis") == 0) {
      std::string axis = trim(line.substr(6));
      auto arrow = axis.find("-->");
      if (arrow != std::string::npos) {
        x_left = trim(axis.substr(0, arrow));
        x_right = trim(axis.substr(arrow + 3));
      }
    } else if (line.find("y-axis") == 0) {
      std::string axis = trim(line.substr(6));
      auto arrow = axis.find("-->");
      if (arrow != std::string::npos) {
        y_bottom = trim(axis.substr(0, arrow));
        y_top = trim(axis.substr(arrow + 3));
      }
    } else if (line.find("quadrant-1") == 0) {
      q1 = trim(line.substr(10));
    } else if (line.find("quadrant-2") == 0) {
      q2 = trim(line.substr(10));
    } else if (line.find("quadrant-3") == 0) {
      q3 = trim(line.substr(10));
    } else if (line.find("quadrant-4") == 0) {
      q4 = trim(line.substr(10));
    } else {
      // Item: "Label: [x, y]"
      auto colon = line.find(':');
      if (colon != std::string::npos) {
        std::string label = trim(line.substr(0, colon));
        std::string coords = line.substr(colon + 1);
        auto bracket = coords.find('[');
        auto comma = coords.find(',', bracket);
        auto close = coords.find(']', comma);
        if (bracket != std::string::npos && comma != std::string::npos && close != std::string::npos) {
          float x = 0.5f;
          float y = 0.5f;
          try {
            x = std::stof(trim(coords.substr(bracket + 1, comma - bracket - 1)));
            y = std::stof(trim(coords.substr(comma + 1, close - comma - 1)));
          } catch (...) {
          }
          items.push_back({label, x, y});
        }
      }
    }
  }

  // Render the 2x2 grid
  int width = (cols > 0) ? std::min(cols - 4, 70) : 66;
  int half_w = width / 2;

  // Title
  if (!title.empty()) {
    out << "\033[1m" << title << "\033[0m\n\n";
  }

  // X-axis label
  int axis_pad = (width - static_cast<int>(x_left.size()) - static_cast<int>(x_right.size())) / 2;
  out << "    " << x_left;
  for (int i = 0; i < axis_pad; i++) {
    out << "\xe2\x94\x80";  // ─
  }
  out << x_right << "\n";

  // Top border
  out << "    \xe2\x94\x8c";  // ┌
  for (int i = 0; i < half_w - 1; i++) out << "\xe2\x94\x80";
  out << "\xe2\x94\xac";  // ┬
  for (int i = 0; i < half_w - 1; i++) out << "\xe2\x94\x80";
  out << "\xe2\x94\x90\n";  // ┐

  // Helper: render a quadrant row with items
  auto render_quad = [&](const std::string& qlabel, const std::string& qright, float /*x_min*/, float /*x_max*/, float y_min, float y_max,
                         const std::string& y_label) {
    // Quadrant label line
    std::string left_cell = " " + qlabel;
    while (static_cast<int>(left_cell.size()) < half_w - 1) left_cell += ' ';
    std::string right_cell = " " + qright;
    while (static_cast<int>(right_cell.size()) < half_w - 1) right_cell += ' ';

    out << "    \xe2\x94\x82" << left_cell << "\xe2\x94\x82" << right_cell << "\xe2\x94\x82 " << y_label << "\n";

    // Items in this quadrant
    std::string left_items;
    std::string right_items;
    for (const auto& item : items) {
      if (item.y >= y_min && item.y < y_max) {
        if (item.x < 0.5f) {
          left_items += " \xe2\x80\xa2 " + item.label;  // • item
        } else {
          right_items += " \xe2\x80\xa2 " + item.label;
        }
      }
    }
    // Print items line
    std::string li = left_items.empty() ? "" : left_items;
    while (static_cast<int>(li.size()) < half_w - 1) li += ' ';
    if (static_cast<int>(li.size()) > half_w - 1) li = li.substr(0, half_w - 1);
    std::string ri = right_items.empty() ? "" : right_items;
    while (static_cast<int>(ri.size()) < half_w - 1) ri += ' ';
    if (static_cast<int>(ri.size()) > half_w - 1) ri = ri.substr(0, half_w - 1);
    out << "    \xe2\x94\x82" << li << "\xe2\x94\x82" << ri << "\xe2\x94\x82\n";
  };

  // Top quadrants (Q1=top-left, Q2=top-right in mermaid convention)
  render_quad(q1, q2, 0, 0.5f, 0.5f, 1.0f, y_top);

  // Middle separator
  out << "    \xe2\x94\x9c";  // ├
  for (int i = 0; i < half_w - 1; i++) out << "\xe2\x94\x80";
  out << "\xe2\x94\xbc";  // ┼
  for (int i = 0; i < half_w - 1; i++) out << "\xe2\x94\x80";
  out << "\xe2\x94\xa4\n";  // ┤

  // Bottom quadrants (Q3=bottom-left, Q4=bottom-right)
  render_quad(q3, q4, 0, 0.5f, 0, 0.5f, y_bottom);

  // Bottom border
  out << "    \xe2\x94\x94";  // └
  for (int i = 0; i < half_w - 1; i++) out << "\xe2\x94\x80";
  out << "\xe2\x94\xb4";  // ┴
  for (int i = 0; i < half_w - 1; i++) out << "\xe2\x94\x80";
  out << "\xe2\x94\x98\n";  // ┘

  return true;
}

}  // namespace tui
