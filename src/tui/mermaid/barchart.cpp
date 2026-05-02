/**
 * @file barchart.cpp
 * @brief Bar chart renderer — horizontal bars showing absolute values.
 *
 * Unlike pie (which shows percentages of a whole), bar chart shows
 * absolute values with bars scaled to the maximum value.
 *
 * Custom syntax:
 *   barchart
 *     title Response Times (ms)
 *     "GET /api" : 45
 *     "POST /login" : 120
 *     "GET /users" : 89
 *
 * Renders as:
 *   Response Times (ms)
 *   GET /api     ████████████░░░░░░░░░░░░░░░░░░  45
 *   POST /login  ██████████████████████████████  120
 *   GET /users   ██████████████████████░░░░░░░░   89
 */
#include "tui/mermaid/barchart.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace tui {

struct BarItem {
  std::string label;
  double value = 0;
};

static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

bool BarChartRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 8, "barchart") == 0;
}

bool BarChartRenderer::render(const std::string& input, std::ostream& out, int cols, int /*rows*/) const {
  std::string title;
  std::vector<BarItem> items;

  std::istringstream stream(input);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (!header_found) {
      if (line.find("barchart") == 0) {
        header_found = true;
      }
      continue;
    }
    if (line.find("title") == 0) {
      title = trim(line.substr(5));
      continue;
    }
    // Parse "Label" : value
    auto quote_start = line.find('"');
    auto quote_end = line.find('"', quote_start + 1);
    auto colon = line.find(':', quote_end);
    if (quote_start != std::string::npos && quote_end != std::string::npos && colon != std::string::npos) {
      std::string label = line.substr(quote_start + 1, quote_end - quote_start - 1);
      std::string val_str = trim(line.substr(colon + 1));
      double val = 0;
      try {
        val = std::stod(val_str);
      } catch (...) {
        continue;
      }
      items.push_back({label, val});
    }
  }

  if (items.empty()) {
    return false;
  }

  // Find max value for scaling
  double max_val = 0;
  for (const auto& item : items) {
    if (item.value > max_val) {
      max_val = item.value;
    }
  }
  if (max_val <= 0) {
    return false;
  }

  // Find longest label
  size_t max_label = 0;
  for (const auto& item : items) {
    max_label = std::max(max_label, item.label.size());
  }

  // Bar width
  int bar_width = (cols > 0 ? cols : 80) - static_cast<int>(max_label) - 10;
  if (bar_width < 15) {
    bar_width = 15;
  }

  // Colors
  static const char* colors[] = {"\033[36m", "\033[33m", "\033[32m", "\033[35m", "\033[34m", "\033[31m"};
  static const int num_colors = 6;

  if (!title.empty()) {
    out << title << "\n\n";
  }

  for (size_t idx = 0; idx < items.size(); idx++) {
    const auto& item = items[idx];
    int filled = static_cast<int>((item.value / max_val) * bar_width);
    const char* color = colors[idx % num_colors];

    // Label (right-padded)
    out << item.label;
    for (size_t i = item.label.size(); i < max_label + 2; i++) {
      out << ' ';
    }

    // Bar
    out << color;
    for (int i = 0; i < filled; i++) {
      out << "\xe2\x96\x88";  // █
    }
    out << "\033[0m\033[2m";
    for (int i = filled; i < bar_width; i++) {
      out << "\xe2\x96\x91";  // ░
    }
    out << "\033[0m";

    // Value (absolute, not percentage)
    out << " " << static_cast<int>(item.value) << "\n";
  }

  return true;
}

}  // namespace tui
