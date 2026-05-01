/**
 * @file tui.h
 * @brief Terminal UI helpers — ANSI colors, markdown rendering, spinner.
 * @see docs/adr/adr-016-tui-design.md
 */
#ifndef TUI_H
#define TUI_H

#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace tui {

/** Check if color output is enabled (TTY + no NO_COLOR + no --no-color) */
inline bool use_color(bool no_color_flag = false) {
  if (no_color_flag) {
    return false;
  }
  if (std::getenv("NO_COLOR")) {
    return false;
  }
  return isatty(STDOUT_FILENO) != 0;
}

/** Print ASCII art banner at startup */
inline void banner(std::ostream& out, bool color) {
  const char* art =
      "    __    __    ___    __  ___  ___           ________    ____\n"
      "   / /   / /   /   |  /  |/  / /   |         / ____/ /   /  _/\n"
      "  / /   / /   / /| | / /|_/ / / /| | ______ / /   / /    / /  \n"
      " / /___/ /___/ ___ |/ /  / / / ___ |/_____// /___/ /____/ /   \n"
      "/_____/_____/_/  |_/_/  /_/ /_/  |_|       \\____/_____/___/   \n";
  if (color) {
    out << "\033[1;33m" << art << "\033[0m\n";
  } else {
    out << art << "\n";
  }
}

/** Print colored prompt marker */
inline void prompt(std::ostream& out, bool color) { out << (color ? "\033[1;32m> \033[0m" : "> "); }

/** Print a dim system message */
inline void system_msg(std::ostream& out, bool color, const std::string& msg) {
  out << (color ? "\033[2m" : "") << msg << (color ? "\033[0m" : "") << "\n";
}

/** Print a bold red error message */
inline void error(std::ostream& out, bool color, const std::string& msg) {
  out << "\n" << (color ? "\033[1;31m" : "") << msg << (color ? "\033[0m" : "") << "\n";
}

/** Print cyan command output */
inline void cmd_output(std::ostream& out, bool color, const std::string& msg) {
  out << (color ? "\033[36m" : "") << msg << (color ? "\033[0m" : "") << "\n";
}

/** Print yellow write proposal */
inline void write_proposal(std::ostream& out, bool color, const std::string& path) {
  out << (color ? "\033[33m" : "") << "[proposed: write " << path << "]" << (color ? "\033[0m" : "") << "\n";
}

// --- Markdown rendering ---

/** Check if a line is a horizontal rule (---, ***, ___).
 * Must be 3+ of the same char with optional spaces, nothing else. */
inline bool is_horizontal_rule(const std::string& line) {
  if (line.size() < 3) {
    return false;
  }
  // Find the rule character (-, *, _)
  char rule_char = 0;
  int count = 0;
  for (char c : line) {
    if (c == ' ') {
      continue;
    }
    if (c == '-' || c == '*' || c == '_') {
      if (rule_char == 0) {
        rule_char = c;
      }
      if (c != rule_char) {
        return false;
      }
      count++;
    } else {
      return false;
    }
  }
  return count >= 3;
}

/** Try to match ~~strikethrough~~ at position i.
 * Looks for matching closing ~~ delimiter.
 * Returns true if matched, false if no match found. */
inline bool try_strikethrough(const std::string& line, size_t& i, std::string& out) {
  if (i + 1 >= line.size() || line[i] != '~' || line[i + 1] != '~') {
    return false;
  }
  auto end = line.find("~~", i + 2);
  if (end == std::string::npos) {
    return false;
  }
  // ANSI strikethrough: ESC[9m
  out += "\033[9m" + line.substr(i + 2, end - i - 2) + "\033[0m";
  i = end + 2;
  return true;
}

/** Try to match ***bold+italic*** at position i.
 * Must check before bold/italic to avoid partial matches.
 * Returns true if matched, false if no match found. */
inline bool try_bold_italic(const std::string& line, size_t& i, std::string& out) {
  if (i + 2 >= line.size() || line[i] != '*' || line[i + 1] != '*' || line[i + 2] != '*') {
    return false;
  }
  auto end = line.find("***", i + 3);
  if (end == std::string::npos) {
    return false;
  }
  // Bold (1) + italic (3)
  out += "\033[1;3m" + line.substr(i + 3, end - i - 3) + "\033[0m";
  i = end + 3;
  return true;
}

/** Try to match [text](url) link at position i.
 * Renders text normally with url dimmed in parentheses.
 * Returns true if matched, false if no match found. */
inline bool try_link(const std::string& line, size_t& i, std::string& out) {
  if (line[i] != '[') {
    return false;
  }
  auto close_bracket = line.find(']', i + 1);
  if (close_bracket == std::string::npos) {
    return false;
  }
  // Must be immediately followed by (
  if (close_bracket + 1 >= line.size() || line[close_bracket + 1] != '(') {
    return false;
  }
  auto close_paren = line.find(')', close_bracket + 2);
  if (close_paren == std::string::npos) {
    return false;
  }
  std::string text = line.substr(i + 1, close_bracket - i - 1);
  std::string url = line.substr(close_bracket + 2, close_paren - close_bracket - 2);
  // Show text underlined, url dimmed
  out += "\033[4m" + text + "\033[0m \033[2m(" + url + ")\033[0m";
  i = close_paren + 1;
  return true;
}

/** Try to match **bold** at position i, append ANSI and advance i.
 * Looks for matching closing ** delimiter.
 * Returns true if matched, false if no match found. */
inline bool try_bold(const std::string& line, size_t& i, std::string& out) {
  if (i + 1 >= line.size() || line[i] != '*' || line[i + 1] != '*') {
    return false;
  }
  auto end = line.find("**", i + 2);
  if (end == std::string::npos) {
    return false;
  }
  out += "\033[1m" + line.substr(i + 2, end - i - 2) + "\033[0m";
  i = end + 2;
  return true;
}

/** Try to match *italic* at position i (not adjacent to **).
 * Checks that the star is not part of a ** pair.
 * Returns true if matched, false if no match found. */
inline bool try_italic(const std::string& line, size_t& i, std::string& out) {
  if (line[i] != '*' || (i > 0 && line[i - 1] == '*')) {
    return false;
  }
  if (i + 1 >= line.size() || line[i + 1] == '*') {
    return false;
  }
  auto end = line.find('*', i + 1);
  if (end == std::string::npos || (end + 1 < line.size() && line[end + 1] == '*')) {
    return false;
  }
  out += "\033[3m" + line.substr(i + 1, end - i - 1) + "\033[0m";
  i = end + 1;
  return true;
}

/** Try to match `code` at position i (not ```).
 * Skips triple-backtick code fences.
 * Returns true if matched, false if no match found. */
inline bool try_code(const std::string& line, size_t& i, std::string& out) {
  if (line[i] != '`' || (i + 1 < line.size() && line[i + 1] == '`')) {
    return false;
  }
  auto end = line.find('`', i + 1);
  if (end == std::string::npos) {
    return false;
  }
  out += "\033[36m" + line.substr(i + 1, end - i - 1) + "\033[0m";
  i = end + 1;
  return true;
}

/** Render inline markdown: ***bold+italic***, **bold**, *italic*,
 * ~~strikethrough~~, `code`, [links](url).
 * Walks the string char-by-char, matching paired delimiters.
 * Order matters: bold_italic before bold, bold before italic.
 * Returns the original string unchanged when color is disabled. */
inline std::string render_inline(const std::string& line, bool color) {
  if (!color) {
    return line;
  }
  std::string out;
  size_t i = 0;
  while (i < line.size()) {
    // Order: bold_italic > bold > italic > strikethrough > code > link
    if (try_bold_italic(line, i, out)) {
      continue;
    }
    if (try_bold(line, i, out)) {
      continue;
    }
    if (try_italic(line, i, out)) {
      continue;
    }
    if (try_strikethrough(line, i, out)) {
      continue;
    }
    if (try_code(line, i, out)) {
      continue;
    }
    if (try_link(line, i, out)) {
      continue;
    }
    out += line[i++];
  }
  return out;
}

/** Try to render a heading line (# ## ###).
 * Strips the # prefix and renders bold+underline.
 * Returns empty if not a heading. */
inline std::string try_heading(const std::string& line) {
  if (line.empty() || line[0] != '#') {
    return "";
  }
  size_t lvl = line.find_first_not_of('#');
  if (lvl == std::string::npos || line[lvl] != ' ') {
    return "";
  }
  return "\033[1;4m" + line.substr(lvl + 1) + "\033[0m\n";
}

/** Try to render a list item (- or * or 1.), including indented sub-items.
 * Preserves nesting depth with additional indentation.
 * Returns empty if not a list. */
inline std::string try_list(const std::string& line, bool color) {
  // Count leading whitespace for nesting
  size_t indent = 0;
  while (indent < line.size() && (line[indent] == ' ' || line[indent] == '\t')) {
    indent++;
  }
  std::string rest = line.substr(indent);
  // Each 2 spaces = 1 nesting level (extra indent beyond base "  ")
  std::string pad(2 + indent, ' ');

  // Unordered: - item or * item
  if (rest.size() >= 2 && (rest[0] == '-' || rest[0] == '*') && rest[1] == ' ') {
    return pad + "• " + render_inline(rest.substr(2), color) + "\n";
  }
  // Ordered: 1. item
  if (!rest.empty() && rest[0] >= '1' && rest[0] <= '9') {
    auto dot = rest.find(". ");
    if (dot != std::string::npos && dot <= 3) {
      return pad + rest.substr(0, dot + 2) + render_inline(rest.substr(dot + 2), color) + "\n";
    }
  }
  return "";
}

// --- Terminal width ---

/** Get terminal width in columns via ioctl. Falls back to 80. */
inline int get_terminal_width() {
  struct winsize w {};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
    return w.ws_col;
  }
  return 80;
}

// --- Table rendering ---

/** Check if a line is a markdown table row (starts with |). */
inline bool is_table_row(const std::string& line) { return !line.empty() && line[0] == '|'; }

/** Check if a line is a table separator (| --- | --- |). */
inline bool is_table_separator(const std::string& line) {
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

/** Split a table row into trimmed cell strings. */
inline std::vector<std::string> parse_table_cells(const std::string& line) {
  std::vector<std::string> cells;
  size_t start = 0;
  // Skip leading |
  if (!line.empty() && line[0] == '|') {
    start = 1;
  }
  while (start < line.size()) {
    auto pipe = line.find('|', start);
    std::string cell = (pipe == std::string::npos) ? line.substr(start) : line.substr(start, pipe - start);
    // Trim whitespace
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

/** Compute visible text length after markdown markers are stripped.
 * Removes bold, italic, strikethrough, code delimiters and link syntax
 * to match what render_inline produces visually. */
inline size_t visible_length(const std::string& text) {
  size_t len = 0;
  size_t i = 0;
  while (i < text.size()) {
    // ***bold+italic***
    if (i + 2 < text.size() && text[i] == '*' && text[i + 1] == '*' && text[i + 2] == '*') {
      auto end = text.find("***", i + 3);
      if (end != std::string::npos) {
        len += end - i - 3;
        i = end + 3;
        continue;
      }
    }
    // **bold**
    if (i + 1 < text.size() && text[i] == '*' && text[i + 1] == '*') {
      auto end = text.find("**", i + 2);
      if (end != std::string::npos) {
        len += end - i - 2;
        i = end + 2;
        continue;
      }
    }
    // ~~strikethrough~~
    if (i + 1 < text.size() && text[i] == '~' && text[i + 1] == '~') {
      auto end = text.find("~~", i + 2);
      if (end != std::string::npos) {
        len += end - i - 2;
        i = end + 2;
        continue;
      }
    }
    // `code`
    if (text[i] == '`' && (i + 1 >= text.size() || text[i + 1] != '`')) {
      auto end = text.find('`', i + 1);
      if (end != std::string::npos) {
        len += end - i - 1;
        i = end + 1;
        continue;
      }
    }
    // [text](url) → "text (url)"
    if (text[i] == '[') {
      auto cb = text.find(']', i + 1);
      if (cb != std::string::npos && cb + 1 < text.size() && text[cb + 1] == '(') {
        auto cp = text.find(')', cb + 2);
        if (cp != std::string::npos) {
          len += (cb - i - 1) + 1 + (cp - cb - 2) + 1;  // "text (url)"
          i = cp + 1;
          continue;
        }
      }
    }
    // *italic* — single star not part of ** pair
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

/** Render a collected table block with terminal-width-aware column padding.
 * Distributes available width proportionally across columns.
 * Separator rows become thin dim lines. */
inline std::string render_table(const std::vector<std::string>& rows, bool color) {
  if (rows.empty()) {
    return "";
  }

  // Parse all rows into cells and find column count
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

  // Find max visible width per column
  // With color: use visible_length (markdown markers stripped by render_inline)
  // Without color: use raw size (markers stay in output)
  std::vector<size_t> max_widths(num_cols, 0);
  for (const auto& cells : parsed) {
    for (size_t c = 0; c < cells.size(); c++) {
      size_t w = color ? visible_length(cells[c]) : cells[c].size();
      max_widths[c] = std::max(max_widths[c], w);
    }
  }

  // Distribute terminal width across columns
  // Budget: terminal_width - (num_cols + 1) for pipe chars - (num_cols * 2) for padding spaces
  int term_width = get_terminal_width();
  int overhead = static_cast<int>(num_cols + 1 + num_cols * 2);
  int available = std::max(static_cast<int>(num_cols), term_width - overhead);

  // Compute total content width
  size_t total_content = 0;
  for (size_t c = 0; c < num_cols; c++) {
    total_content += std::max(max_widths[c], static_cast<size_t>(1));
  }

  // Scale columns proportionally to fill available width
  std::vector<size_t> col_widths(num_cols);
  for (size_t c = 0; c < num_cols; c++) {
    size_t content = std::max(max_widths[c], static_cast<size_t>(1));
    col_widths[c] = std::max(content, content * available / total_content);
  }

  // Render each row
  std::string result;
  for (size_t r = 0; r < parsed.size(); r++) {
    if (is_sep[r]) {
      // Separator: dim line with dashes
      std::string sep_line = "|";
      for (size_t c = 0; c < num_cols; c++) {
        sep_line += std::string(col_widths[c] + 2, '-');
        sep_line += "|";
      }
      if (color) {
        result += "\033[2m" + sep_line + "\033[0m\n";
      } else {
        result += sep_line + "\n";
      }
      continue;
    }

    // Data row: pad based on visible length (after markdown stripping)
    std::string row_line = "| ";
    for (size_t c = 0; c < num_cols; c++) {
      std::string cell = (c < parsed[r].size()) ? parsed[r][c] : "";
      std::string rendered = color ? render_inline(cell, color) : cell;
      // With color: markdown markers are stripped, so pad by visible length
      // Without color: markers stay, so pad by raw cell length
      size_t vis_len = color ? visible_length(cell) : cell.size();
      size_t pad = (vis_len < col_widths[c]) ? col_widths[c] - vis_len : 0;
      row_line += rendered + std::string(pad, ' ');
      row_line += " | ";
    }
    result += row_line + "\n";
  }
  return result;
}

/** Render a single markdown line (not inside a code block) to ANSI.
 * Dispatches to heading, horizontal rule, blockquote, list, or inline. */
inline std::string render_line(const std::string& line, bool color) {
  // Horizontal rule: ---, ***, ___ (3+ chars, optional spaces)
  if (is_horizontal_rule(line)) {
    return color ? "\033[2m────────────────────\033[0m\n" : "--------------------\n";
  }
  std::string result = try_heading(line);
  if (!result.empty()) {
    return result;
  }
  // Blockquote: > text — render with dim bar prefix
  if (!line.empty() && line[0] == '>') {
    std::string content = (line.size() > 1 && line[1] == ' ') ? line.substr(2) : line.substr(1);
    if (color) {
      return "\033[2m│\033[0m " + render_inline(content, color) + "\n";
    }
    return "| " + content + "\n";
  }
  result = try_list(line, color);
  if (!result.empty()) {
    return result;
  }
  return render_inline(line, color) + "\n";
}

/** Render a single line inside a code block or at a code-fence boundary.
 * Toggles in_code_block when a triple-backtick fence is encountered.
 * @param line The input line to render.
 * @param in_code_block Current code-block state; toggled on fence lines.
 * @returns The line wrapped with cyan ANSI styling and a newline. */
inline std::string render_code_block_line(const std::string& line, bool& in_code_block) {
  if (line.rfind("```", 0) == 0) {
    in_code_block = !in_code_block;
    return "\033[36m" + line + "\033[0m\n";
  }
  return "\033[36m" + line + "\033[0m\n";
}

/** Render multiline Markdown-like text into an ANSI-styled string.
 *
 * Splits the input on newline boundaries and renders each line using block
 * and inline renderers; lines that start or are inside a triple-backtick code
 * fence are rendered as code block lines. Table rows are buffered and
 * rendered as a block when the table ends.
 *
 * @param text Input text that may contain multiple lines and Markdown-like constructs.
 * @param color If true, apply ANSI styling to rendered output; if false, return content without styling.
 * @returns The concatenated rendered output with per-line rendering and preserved line breaks.
 */
inline std::string render_lines(const std::string& text, bool color) {
  std::string result;
  bool in_code_block = false;
  std::vector<std::string> table_buf;  // buffer for consecutive table rows
  size_t pos = 0;
  while (pos < text.size()) {
    auto nl = text.find('\n', pos);
    std::string line = (nl == std::string::npos) ? text.substr(pos) : text.substr(pos, nl - pos);
    pos = (nl == std::string::npos) ? text.size() : nl + 1;
    if (line.rfind("```", 0) == 0 || in_code_block) {
      // Flush any pending table before code block
      if (!table_buf.empty()) {
        result += render_table(table_buf, color);
        table_buf.clear();
      }
      result += render_code_block_line(line, in_code_block);
    } else if (is_table_row(line)) {
      // Collect table rows until the block ends
      table_buf.push_back(line);
    } else {
      // Non-table line: flush any pending table first
      if (!table_buf.empty()) {
        result += render_table(table_buf, color);
        table_buf.clear();
      }
      result += render_line(line, color);
    }
  }
  // Flush trailing table
  if (!table_buf.empty()) {
    result += render_table(table_buf, color);
  }
  return result;
}

/** Render a full markdown text block to ANSI.
 * Pluggable: replace this function to use a different renderer.
 * Falls back to raw text on any exception — never loses output. */
inline std::string render_markdown(const std::string& text, bool color) {
  if (!color) {
    return text;
  }
  try {
    std::string result = render_lines(text, color);
    if (!result.empty() && result.back() == '\n' && !text.empty() && text.back() != '\n') {
      result.pop_back();
    }
    return result;
  } catch (...) {
    // Graceful degradation: return raw text rather than crash
    return text;
  }
}

// --- Spinner ---

/** Default spinner messages — shown while waiting for LLM response */
inline std::vector<const char*> default_messages() { return {"thinking...", "processing...", "analyzing..."}; }

/** BOFH spinner messages — activate with --why-so-serious */
inline std::vector<const char*> bofh_messages() {
  return {
      "reticulating splines...",    "consulting the oracle...",   "warming up the hamsters...",   "bribing the CPU...",
      "asking nicely...",           "sacrificing a semicolon...", "blaming the intern...",        "googling the answer...",
      "rolling a d20...",           "compiling excuses...",       "negotiating with malloc...",   "feeding the neural goats...",
      "dividing by almost zero...", "untangling spaghetti...",    "summoning elder functions...", "polishing the bits...",
      "herding pointers...",        "defragmenting thoughts...",  "calibrating the flux...",      "rebooting common sense...",
  };
}

}  // namespace tui

/** Streaming markdown renderer — processes tokens as they arrive.
 * Maintains state across tokens to apply ANSI formatting:
 * - **bold**, *italic*, `code` (inline)
 * - ``` code blocks (dimmed)
 * - ## headings (bold+underline)
 * - bullet lists (• prefix)
 * - tables (buffered until block ends)
 * Falls back to raw text on any exception — never loses output. */
class StreamRenderer {
 public:
  /** Construct a streaming renderer.
   * @param out Output stream to write rendered tokens to.
   * @param color If true, apply ANSI styling; if false, pass through raw text. */
  explicit StreamRenderer(std::ostream& out, bool color) : out_(out), color_(color) {}

  /// Process a token (may contain partial lines, newlines, etc.)
  void write(const std::string& token) {
    try {
      for (char c : token) {
        buf_ += c;
        if (c == '\n') {
          flush_line();
        }
      }
    } catch (...) {
      // Graceful degradation: emit raw token
      out_ << token;
    }
  }

  /// Flush any remaining buffered content
  void finish() {
    try {
      // Flush any pending table rows
      flush_table_buf();
      if (!buf_.empty()) {
        // Add newline so flush_line can process block-level elements
        buf_ += '\n';
        flush_line();
      }
      if (in_code_block_) {
        out_ << "\033[0m";
      }
    } catch (...) {
      // Graceful degradation: dump raw buffer
      out_ << buf_;
      buf_.clear();
    }
  }

 private:
  void flush_line() {
    // buf_ includes the trailing \n
    std::string line = buf_;
    buf_.clear();
    std::string content = line.substr(0, line.size() - 1);

    // Hide annotation tags — they are processed separately after streaming
    if (content.find("<exec>") != std::string::npos || content.find("</exec>") != std::string::npos ||
        content.find("<write ") != std::string::npos || content.find("</write>") != std::string::npos ||
        content.find("<str_replace ") != std::string::npos || content.find("</str_replace>") != std::string::npos ||
        content.find("<read ") != std::string::npos || content.find("<search>") != std::string::npos ||
        content.find("</search>") != std::string::npos) {
      return;  // suppress tag lines from stream output
    }

    if (!color_) {
      flush_line_plain(content, line);
      return;
    }

    // Buffer table rows until the block ends
    if (!in_code_block_ && tui::is_table_row(content)) {
      table_buf_.push_back(content);
      return;
    }
    flush_table_buf();

    // Code fence toggle
    if (line.size() >= 4 && line.substr(0, 3) == "```") {
      in_code_block_ = !in_code_block_;
      out_ << (in_code_block_ ? "\033[2m" : "") << line << (in_code_block_ ? "" : "\033[0m");
      return;
    }
    if (in_code_block_) {
      out_ << line;
      return;
    }

    render_content_line(content);
  }

  /// Flush a line when color is off — still buffers tables for alignment
  void flush_line_plain(const std::string& content, const std::string& line) {
    if (tui::is_table_row(content)) {
      table_buf_.push_back(content);
      return;
    }
    flush_table_buf();
    out_ << line;
  }

  /// Flush any pending table rows
  void flush_table_buf() {
    if (!table_buf_.empty()) {
      out_ << tui::render_table(table_buf_, color_);
      table_buf_.clear();
    }
  }

  /// Render a non-code, non-table content line with markdown formatting
  void render_content_line(const std::string& content) {
    // Heading
    if (!content.empty() && content[0] == '#') {
      std::string h = tui::try_heading(content);
      if (!h.empty()) {
        out_ << h;
        return;
      }
    }
    // Horizontal rule
    if (tui::is_horizontal_rule(content)) {
      out_ << "\033[2m────────────────────\033[0m\n";
      return;
    }
    // Blockquote
    if (!content.empty() && content[0] == '>') {
      std::string bq = (content.size() > 1 && content[1] == ' ') ? content.substr(2) : content.substr(1);
      out_ << "\033[2m│\033[0m " << tui::render_inline(bq, color_) << "\n";
      return;
    }
    // List item
    std::string li = tui::try_list(content, color_);
    if (!li.empty()) {
      out_ << li;
      return;
    }
    // Regular line with inline formatting
    out_ << tui::render_inline(content, color_) << "\n";
  }

  std::ostream& out_;
  bool color_;
  std::string buf_;
  std::vector<std::string> table_buf_;  // buffered table rows for block rendering
  bool in_code_block_ = false;
};

/** RAII spinner — shows animated dots while waiting for LLM response.
 * Only active when constructed with active=true (interactive TTY).
 * Destructor stops the animation and clears the line automatically. */
class Spinner {
 public:
  /** Start spinner with custom messages. No-op if active=false. */
  Spinner(std::ostream& out, bool active, std::vector<const char*> msgs = tui::default_messages())
      : out_(out), running_(active), msgs_(std::move(msgs)) {
    if (running_) {
      thread_ = std::thread([this] { run(); });
    }
  }
  ~Spinner() { stop(); }
  /// Stop the spinner and clear its line (safe to call multiple times)
  void stop() {
    bool was_running = running_.exchange(false);
    if (thread_.joinable()) {
      thread_.join();
    }
    if (was_running) {
      out_ << "\r\033[K" << std::flush;
    }
  }
  Spinner(const Spinner&) = delete;
  Spinner& operator=(const Spinner&) = delete;

 private:
  void run() {
    const char* frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    int i = 0;
    int msg_idx = 0;
    while (running_) {
      out_ << "\r\033[2m" << frames[i % 10] << " " << msgs_[msg_idx % msgs_.size()] << "\033[0m\033[K" << std::flush;
      i++;
      if (i % 30 == 0) {
        msg_idx++;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
  }
  std::ostream& out_;
  std::atomic<bool> running_;
  std::vector<const char*> msgs_;
  std::thread thread_;
};

#endif
