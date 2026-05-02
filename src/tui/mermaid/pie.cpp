/**
 * @file pie.cpp
 * @brief Pie chart renderer — renders as horizontal bar chart.
 *
 * Parses mermaid pie syntax:
 *   pie title My Title
 *     "Label A" : 40
 *     "Label B" : 60
 *
 * Renders as:
 *   My Title
 *   Label A  ████████████████░░░░░░░░░░░░░░░░  40%
 *   Label B  ████████████████████████████████  60%
 */
#include "tui/mermaid/pie.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace tui {

// --- Internal structures ---

struct Slice {
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

bool PieRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 3, "pie") == 0;
}

bool PieRenderer::render(const std::string& input, std::ostream& out, int cols, int /*rows*/) const {
  std::vector<Slice> slices;
  std::string title;

  // Default bar width
  constexpr int DEFAULT_BAR_WIDTH = 30;
  int bar_width = (cols > 0) ? std::min(cols / 2, 40) : DEFAULT_BAR_WIDTH;

  std::istringstream stream(input);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (!header_found) {
      if (line.find("pie") == 0) {
        // Check for "pie title ..."
        if (line.size() > 3) {
          std::string rest = trim(line.substr(3));
          if (rest.find("title") == 0 && rest.size() > 5) {
            title = trim(rest.substr(5));
          }
        }
        header_found = true;
      }
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
      slices.push_back({label, val});
    }
  }

  if (slices.empty()) {
    return false;
  }

  // Calculate total for percentages
  double total = 0;
  for (const auto& s : slices) {
    total += s.value;
  }
  if (total <= 0) {
    return false;
  }

  // Find longest label for alignment
  size_t max_label = 0;
  for (const auto& s : slices) {
    max_label = std::max(max_label, s.label.size());
  }

  // Print title if present
  if (!title.empty()) {
    out << title << "\n\n";
  }

  // Render each slice as a horizontal bar
  // Use block characters: █ (full) and ░ (empty)
  for (const auto& s : slices) {
    double pct = (s.value / total) * 100.0;
    int filled = static_cast<int>((s.value / total) * bar_width);

    // Label (right-padded)
    out << s.label;
    for (size_t i = s.label.size(); i < max_label + 2; i++) {
      out << ' ';
    }

    // Bar: filled portion with █, empty with ░
    for (int i = 0; i < filled; i++) {
      out << "\xe2\x96\x88";  // █ (U+2588)
    }
    for (int i = filled; i < bar_width; i++) {
      out << "\xe2\x96\x91";  // ░ (U+2591)
    }

    // Percentage
    out << " " << std::fixed << std::setprecision(0) << pct << "%\n";
  }

  return true;
}

}  // namespace tui
