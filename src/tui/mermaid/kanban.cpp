/**
 * @file kanban.cpp
 * @brief Kanban board renderer — columns with task items.
 *
 * Custom syntax (not standard mermaid):
 *   kanban
 *     title Sprint Board
 *     column Todo
 *       Design API
 *       Write tests
 *     column In Progress
 *       Implement auth
 *     column Done
 *       Setup CI
 *       Add logging
 *
 * Renders as side-by-side columns with box-drawing borders:
 *   Sprint Board
 *   ┌──────────────┬──────────────┬──────────────┐
 *   │ Todo         │ In Progress  │ Done         │
 *   ├──────────────┼──────────────┼──────────────┤
 *   │ • Design API │ • Impl auth  │ • Setup CI   │
 *   │ • Write tests│              │ • Add logging│
 *   └──────────────┴──────────────┴──────────────┘
 */
#include "tui/mermaid/kanban.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace tui {

struct KanbanColumn {
  std::string name;
  std::vector<std::string> items;
};

static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

bool KanbanRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 6, "kanban") == 0;
}

bool KanbanRenderer::render(const std::string& input, std::ostream& out, int cols, int /*rows*/) const {
  std::string title;
  std::vector<KanbanColumn> columns;

  std::istringstream stream(input);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    std::string trimmed = trim(line);
    if (trimmed.empty()) {
      continue;
    }
    if (!header_found) {
      if (trimmed.find("kanban") == 0) {
        header_found = true;
      }
      continue;
    }
    if (trimmed.find("title") == 0) {
      title = trim(trimmed.substr(5));
    } else if (trimmed.find("column") == 0) {
      columns.push_back({trim(trimmed.substr(6)), {}});
    } else if (!columns.empty()) {
      columns.back().items.push_back(trimmed);
    }
  }

  if (columns.empty()) {
    return false;
  }

  // Calculate column width
  int term_cols = (cols > 0) ? cols : 80;
  int num_cols = static_cast<int>(columns.size());
  int col_w = (term_cols - num_cols - 1) / num_cols;  // account for borders
  if (col_w < 10) {
    col_w = 10;
  }

  // Find max items in any column (determines height)
  int max_items = 0;
  for (const auto& c : columns) {
    int n = static_cast<int>(c.items.size());
    if (n > max_items) {
      max_items = n;
    }
  }

  // Print title
  if (!title.empty()) {
    out << "\033[1m" << title << "\033[0m\n";
  }

  // Top border: ┌───┬───┬───┐
  out << "\xe2\x94\x8c";
  for (int i = 0; i < num_cols; i++) {
    for (int j = 0; j < col_w; j++) out << "\xe2\x94\x80";
    out << (i < num_cols - 1 ? "\xe2\x94\xac" : "\xe2\x94\x90");
  }
  out << "\n";

  // Header row: │ Name │ Name │
  out << "\xe2\x94\x82";
  for (int i = 0; i < num_cols; i++) {
    std::string name = " \033[1m" + columns[i].name + "\033[0m";
    out << name;
    // Pad to col_w (account for ANSI escape length)
    int visible_len = static_cast<int>(columns[i].name.size()) + 1;
    for (int j = visible_len; j < col_w; j++) out << ' ';
    out << "\xe2\x94\x82";
  }
  out << "\n";

  // Separator: ├───┼───┼───┤
  out << "\xe2\x94\x9c";
  for (int i = 0; i < num_cols; i++) {
    for (int j = 0; j < col_w; j++) out << "\xe2\x94\x80";
    out << (i < num_cols - 1 ? "\xe2\x94\xbc" : "\xe2\x94\xa4");
  }
  out << "\n";

  // Item rows
  for (int row = 0; row < max_items; row++) {
    out << "\xe2\x94\x82";
    for (int i = 0; i < num_cols; i++) {
      std::string cell;
      if (row < static_cast<int>(columns[i].items.size())) {
        cell = " \xe2\x80\xa2 " + columns[i].items[row];  // • item
      }
      // Truncate if too long
      if (static_cast<int>(cell.size()) > col_w - 1) {
        cell = cell.substr(0, col_w - 2) + "\xe2\x80\xa6";  // …
      }
      out << cell;
      int pad = col_w - static_cast<int>(cell.size());
      for (int j = 0; j < pad; j++) out << ' ';
      out << "\xe2\x94\x82";
    }
    out << "\n";
  }

  // Bottom border: └───┴───┴───┘
  out << "\xe2\x94\x94";
  for (int i = 0; i < num_cols; i++) {
    for (int j = 0; j < col_w; j++) out << "\xe2\x94\x80";
    out << (i < num_cols - 1 ? "\xe2\x94\xb4" : "\xe2\x94\x98");
  }
  out << "\n";

  return true;
}

}  // namespace tui
