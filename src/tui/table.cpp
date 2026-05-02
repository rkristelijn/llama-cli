/**
 * @file table.cpp
 * @brief Markdown table parsing and rendering with terminal-width awareness.
 *
 * SRP: All table-related logic lives here — parsing, width calculation, rendering.
 * Extracted from markdown.cpp to reduce file size (ADR-061, ADR-065).
 *
 * @see docs/adr/adr-052-markdown-renderer.md
 */

#include "tui/table.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>

// Forward declaration — render_inline is in markdown.cpp
namespace tui {
std::string render_inline(const std::string& line, bool color);
}  // namespace tui

namespace tui {

// --- Terminal utilities ---

/// Get terminal width in columns via ioctl. Falls back to 80 if not a TTY.
int get_terminal_width() {
  struct winsize w = {};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
    return w.ws_col;
  }
  return 80;
}

/// Check if a line is a markdown table row (starts with |).
bool is_table_row(const std::string& line) { return !line.empty() && line[0] == '|'; }

/// Check if a line is a table separator (| --- | --- |).
/// Only pipe, dash, colon, and space characters allowed.
bool is_table_separator(const std::string& line) {
  if (!is_table_row(line)) {
    return false;
  }
  for (char c : line) {
    if (c != '|' && c != '-' && c != ':' && c != ' ') {
      return false;
    }
  }
  return true;
}

/// Split a table row into trimmed cell strings (pipe-delimited).
/// Handles leading/trailing pipes and trims whitespace from each cell.
/// Empty cells between pipes are preserved as empty strings.
std::vector<std::string> parse_table_cells(const std::string& line) {
  std::vector<std::string> cells;
  size_t start = 0;
  if (!line.empty() && line[0] == '|') {
    start = 1;
  }
  while (start < line.size()) {
    auto pipe = line.find('|', start);
    std::string cell = (pipe == std::string::npos) ? line.substr(start) : line.substr(start, pipe - start);
    size_t first = cell.find_first_not_of(' ');
    size_t last = cell.find_last_not_of(' ');
    if (first != std::string::npos) {
      cells.push_back(cell.substr(first, last - first + 1));
    } else if (pipe != std::string::npos) {
      cells.push_back("");
    }
    if (pipe == std::string::npos) {
      break;
    }
    start = pipe + 1;
  }
  return cells;
}

/// Compute visible text length after markdown markers are stripped.
/// Removes bold, italic, strikethrough, code delimiters and link syntax
/// to match what render_inline produces visually.
// pmccabe:skip-complexity
size_t visible_length(const std::string& text) {
  size_t len = 0;
  size_t i = 0;
  while (i < text.size()) {
    // Skip ***bold italic*** markers — count only inner text
    if (i + 2 < text.size() && text[i] == '*' && text[i + 1] == '*' && text[i + 2] == '*') {
      auto end = text.find("***", i + 3);
      if (end != std::string::npos) {
        len += end - i - 3;
        i = end + 3;
        continue;
      }
    }
    // Skip **bold** markers — count only inner text
    if (i + 1 < text.size() && text[i] == '*' && text[i + 1] == '*') {
      auto end = text.find("**", i + 2);
      if (end != std::string::npos) {
        len += end - i - 2;
        i = end + 2;
        continue;
      }
    }
    // Skip ~~strikethrough~~ markers — count only inner text
    if (i + 1 < text.size() && text[i] == '~' && text[i + 1] == '~') {
      auto end = text.find("~~", i + 2);
      if (end != std::string::npos) {
        len += end - i - 2;
        i = end + 2;
        continue;
      }
    }
    // Skip `code` backticks — count only inner text
    if (text[i] == '`' && (i + 1 >= text.size() || text[i + 1] != '`')) {
      auto end = text.find('`', i + 1);
      if (end != std::string::npos) {
        len += end - i - 1;
        i = end + 1;
        continue;
      }
    }
    // Skip [text](url) link syntax — count text + url visible chars
    if (text[i] == '[') {
      auto cb = text.find(']', i + 1);
      if (cb != std::string::npos && cb + 1 < text.size() && text[cb + 1] == '(') {
        auto cp = text.find(')', cb + 2);
        if (cp != std::string::npos) {
          len += (cb - i - 1) + 1 + (cp - cb - 2) + 1;
          i = cp + 1;
          continue;
        }
      }
    }
    // Skip *italic* single-star markers — count only inner text
    if (text[i] == '*' && (i == 0 || text[i - 1] != '*') && i + 1 < text.size() && text[i + 1] != '*') {
      auto end = text.find('*', i + 1);
      if (end != std::string::npos && (end + 1 >= text.size() || text[end + 1] != '*')) {
        len += end - i - 1;
        i = end + 1;
        continue;
      }
    }
    len++;
    i++;
  }
  return len;
}

/// Render a collected table block with terminal-width-aware column padding.
/// Distributes available width proportionally across columns.
/// Separator rows become thin dim lines; data rows are padded to align.
/// Falls back gracefully when terminal width cannot be detected.
// pmccabe:skip-complexity
std::string render_table(const std::vector<std::string>& rows, bool color) {
  if (rows.empty()) {
    return "";
  }
  std::vector<std::vector<std::string>> parsed;
  std::vector<bool> is_sep;
  size_t num_cols = 0;
  for (const auto& row : rows) {
    auto cells = parse_table_cells(row);
    num_cols = std::max(num_cols, cells.size());
    parsed.push_back(cells);
    is_sep.push_back(is_table_separator(row));
  }
  if (num_cols == 0) {
    return "";
  }

  // Calculate column widths based on content and terminal width
  std::vector<size_t> max_widths(num_cols, 0);
  for (const auto& cells : parsed) {
    for (size_t c = 0; c < cells.size(); c++) {
      size_t w = color ? visible_length(cells[c]) : cells[c].size();
      max_widths[c] = std::max(max_widths[c], w);
    }
  }

  // Distribute available terminal width proportionally across columns
  int term_width = get_terminal_width();
  int overhead = static_cast<int>(num_cols + 1 + num_cols * 2);
  int available = std::max(static_cast<int>(num_cols), term_width - overhead);
  size_t total_content = 0;
  for (size_t c = 0; c < num_cols; c++) {
    total_content += std::max(max_widths[c], static_cast<size_t>(1));
  }

  std::vector<size_t> col_widths(num_cols);
  for (size_t c = 0; c < num_cols; c++) {
    size_t content = std::max(max_widths[c], static_cast<size_t>(1));
    col_widths[c] = std::max(content, content * available / total_content);
  }

  // Render each row: separators as dim lines, data rows with padded cells
  std::string result;
  for (size_t r = 0; r < parsed.size(); r++) {
    if (is_sep[r]) {
      std::string sep_line = "|";
      for (size_t c = 0; c < num_cols; c++) {
        sep_line += std::string(col_widths[c] + 2, '-');
        sep_line += "|";
      }
      result += color ? "\033[2m" + sep_line + "\033[0m\n" : sep_line + "\n";
      continue;
    }
    std::string row_line = "| ";
    for (size_t c = 0; c < num_cols; c++) {
      std::string cell = (c < parsed[r].size()) ? parsed[r][c] : "";
      std::string rendered = color ? render_inline(cell, color) : cell;
      size_t vis_len = color ? visible_length(cell) : cell.size();
      size_t pad = (vis_len < col_widths[c]) ? col_widths[c] - vis_len : 0;
      row_line += rendered + std::string(pad, ' ') + " | ";
    }
    result += row_line + "\n";
  }
  return result;
}

}  // namespace tui
