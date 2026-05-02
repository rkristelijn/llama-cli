/**
 * @file venn.cpp
 * @brief Venn diagram renderer — overlapping braille circles with ANSI colors.
 *
 * Parses a simple venn syntax:
 *   venn
 *     "Set A" : 40
 *     "Set B" : 30
 *     "Set C" : 20
 *
 * Renders 2-3 overlapping circles using braille characters.
 * Each set gets a distinct ANSI color; overlap regions are white/bold.
 */
#include "tui/mermaid/venn.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace tui {

// --- Internal data ---

struct VennSet {
  std::string label;
  double value = 0;
};

/// Trim whitespace from both ends
static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

bool VennRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 4, "venn") == 0;
}

bool VennRenderer::render(const std::string& input, std::ostream& out, int cols, int /*rows*/) const {
  std::vector<VennSet> sets;
  std::string title;

  // Parse input lines
  std::istringstream stream(input);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (!header_found) {
      if (line.find("venn") == 0) {
        // Check for "venn title ..."
        if (line.size() > 4) {
          std::string rest = trim(line.substr(4));
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
      sets.push_back({label, val});
    }
  }

  if (sets.empty() || sets.size() > 3) {
    return false;
  }

  // ANSI colors for each set
  static const char* set_colors[] = {
      "\033[31m",  // red
      "\033[34m",  // blue
      "\033[33m",  // yellow
  };
  static const char* overlap_color = "\033[1;37m";  // bold white for overlap
  static const char* reset = "\033[0m";

  // Canvas dimensions (braille: 2 dots wide, 4 dots tall per cell)
  // This gives us sub-character resolution for smooth circles
  int term_cols = (cols > 0) ? std::min(cols, 60) : 50;
  int term_rows = 14;
  int dw = term_cols * 2;  // dot-width (horizontal resolution)
  int dh = term_rows * 4;  // dot-height (vertical resolution)

  // Circle centers and radii based on number of sets
  struct Circle {
    float cx, cy, r;
  };
  std::vector<Circle> circles;
  float radius = static_cast<float>(std::min(dw, dh)) * 0.35f;

  if (sets.size() == 2) {
    // Two circles side by side with overlap
    float offset = radius * 0.6f;
    circles.push_back({static_cast<float>(dw) / 2.0f - offset, static_cast<float>(dh) / 2.0f, radius});
    circles.push_back({static_cast<float>(dw) / 2.0f + offset, static_cast<float>(dh) / 2.0f, radius});
  } else {
    // Three circles in triangle arrangement
    float offset = radius * 0.55f;
    float cy_top = static_cast<float>(dh) / 2.0f - offset * 0.5f;
    float cy_bot = static_cast<float>(dh) / 2.0f + offset * 0.5f;
    circles.push_back({static_cast<float>(dw) / 2.0f, cy_top, radius});
    circles.push_back({static_cast<float>(dw) / 2.0f - offset, cy_bot, radius});
    circles.push_back({static_cast<float>(dw) / 2.0f + offset, cy_bot, radius});
  }

  // Render to a color-tagged dot grid
  // 0=empty, 1..3=single set, 4=overlap
  std::vector<std::vector<int>> grid(dh, std::vector<int>(dw, 0));

  for (int y = 0; y < dh; y++) {
    for (int x = 0; x < dw; x++) {
      int hit_count = 0;
      int last_hit = -1;
      for (int ci = 0; ci < static_cast<int>(circles.size()); ci++) {
        float dx = static_cast<float>(x) - circles[ci].cx;
        float dy = static_cast<float>(y) - circles[ci].cy;
        if (dx * dx + dy * dy <= circles[ci].r * circles[ci].r) {
          hit_count++;
          last_hit = ci;
        }
      }
      if (hit_count > 1) {
        grid[y][x] = 4;  // overlap
      } else if (hit_count == 1) {
        grid[y][x] = last_hit + 1;  // single set (1-indexed)
      }
    }
  }

  // Print title
  if (!title.empty()) {
    out << title << "\n\n";
  }

  // Render grid as braille with colors per cell
  static const int BIT[4][2] = {{0, 3}, {1, 4}, {2, 5}, {6, 7}};

  for (int r = 0; r < term_rows; r++) {
    for (int c = 0; c < term_cols; c++) {
      unsigned int bits = 0;
      int dominant = 0;  // which color dominates this cell

      // Count which set has most dots in this 4x2 cell
      int counts[5] = {0, 0, 0, 0, 0};
      for (int dr = 0; dr < 4; dr++) {
        for (int dc = 0; dc < 2; dc++) {
          int gy = r * 4 + dr;
          int gx = c * 2 + dc;
          if (gy < dh && gx < dw && grid[gy][gx] > 0) {
            bits |= (1u << BIT[dr][dc]);
            counts[grid[gy][gx]]++;
          }
        }
      }

      // Pick dominant color for this cell
      int max_count = 0;
      for (int i = 1; i <= 4; i++) {
        if (counts[i] > max_count) {
          max_count = counts[i];
          dominant = i;
        }
      }

      // Encode braille character U+2800+bits
      unsigned int cp = 0x2800 + bits;
      char utf8[4];
      utf8[0] = static_cast<char>(0xE0 | (cp >> 12));
      utf8[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
      utf8[2] = static_cast<char>(0x80 | (cp & 0x3F));

      // Apply color based on dominant set
      if (dominant == 4) {
        out << overlap_color;
      } else if (dominant > 0 && dominant <= 3) {
        out << set_colors[dominant - 1];
      }
      out.write(utf8, 3);
      if (dominant > 0) {
        out << reset;
      }
    }
    out << '\n';
  }

  // Print legend below diagram
  out << "\n";
  for (size_t i = 0; i < sets.size(); i++) {
    out << "  " << set_colors[i] << "\xe2\x97\x8f" << reset;  // ● colored dot
    out << " " << sets[i].label;
    if (sets[i].value > 0) {
      out << " (" << static_cast<int>(sets[i].value) << ")";
    }
    out << "\n";
  }

  return true;
}

}  // namespace tui
