/**
 * @file markdown.cpp
 * @brief Markdown rendering implementations (extracted from tui.h).
 *
 * SRP: All markdown parsing and ANSI rendering lives here.
 *
 * @see docs/adr/adr-052-markdown-renderer.md
 * @see docs/adr/adr-066-solid-refactoring.md
 */

#include "tui/markdown.h"

#include <unistd.h>

#include <algorithm>

#include "tui/highlight.h"
#include "tui/mermaid/mermaid.h"
#include "tui/mermaid/renderer.h"
#include "tui/table.h"
#include "tui/tui.h"

namespace tui {

// --- Inline formatting helpers ---
// Each try_* function attempts to match a markdown delimiter at position i.
// On match: appends ANSI-styled text to out, advances i past the delimiter, returns true.
// On no match: returns false, i and out unchanged.

/// Check if a line is a horizontal rule (---, ***, ___).
/// Must be 3+ of the same char with optional spaces, nothing else.
bool is_horizontal_rule(const std::string& line) {
  if (line.size() < 3) {
    return false;
  }
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

/// Match ~~strikethrough~~ — ANSI ESC[9m (crossed out text).
bool try_strikethrough(const std::string& line, size_t& i, std::string& out) {
  if (i + 1 >= line.size() || line[i] != '~' || line[i + 1] != '~') {
    return false;
  }
  auto end = line.find("~~", i + 2);
  if (end == std::string::npos) {
    return false;
  }
  out += "\033[9m" + line.substr(i + 2, end - i - 2) + "\033[0m";
  i = end + 2;
  return true;
}

/// Match ***bold+italic*** — must check before ** to avoid partial match.
bool try_bold_italic(const std::string& line, size_t& i, std::string& out) {
  if (i + 2 >= line.size() || line[i] != '*' || line[i + 1] != '*' || line[i + 2] != '*') {
    return false;
  }
  auto end = line.find("***", i + 3);
  if (end == std::string::npos) {
    return false;
  }
  out += "\033[1;3m" + line.substr(i + 3, end - i - 3) + "\033[0m";
  i = end + 3;
  return true;
}

/// Match [text](url) — renders text underlined, url dimmed in parens.
bool try_link(const std::string& line, size_t& i, std::string& out) {
  if (line[i] != '[') {
    return false;
  }
  auto close_bracket = line.find(']', i + 1);
  if (close_bracket == std::string::npos) {
    return false;
  }
  if (close_bracket + 1 >= line.size() || line[close_bracket + 1] != '(') {
    return false;
  }
  auto close_paren = line.find(')', close_bracket + 2);
  if (close_paren == std::string::npos) {
    return false;
  }
  std::string text = line.substr(i + 1, close_bracket - i - 1);
  std::string url = line.substr(close_bracket + 2, close_paren - close_bracket - 2);
  out += "\033[4m" + text + "\033[0m \033[2m(" + url + ")\033[0m";
  i = close_paren + 1;
  return true;
}

/// Match **bold** — ANSI ESC[1m (bright/bold text).
/// Must check after *** to avoid consuming triple-star as double+single.
bool try_bold(const std::string& line, size_t& i, std::string& out) {
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

/// Match *italic* — single star, not adjacent to ** pair.
/// Avoids matching the second star of a ** bold delimiter.
bool try_italic(const std::string& line, size_t& i, std::string& out) {
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

/// Match `code` — single backtick, not triple (```).
/// Renders inline code in cyan (same color as code blocks for consistency).
bool try_code(const std::string& line, size_t& i, std::string& out) {
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

/// Render inline markdown: bold_italic > bold > italic > strikethrough > code > link.
/// Order matters: check triple-star before double-star, double before single.
/// Returns original string unchanged when color is disabled.
std::string render_inline(const std::string& line, bool color) {
  if (!color) {
    return line;
  }
  std::string out;
  size_t i = 0;
  while (i < line.size()) {
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

// --- Block-level rendering ---
// Headings, lists, blockquotes — each returns empty string if not matched.

/// Try to render a heading line (# ## ###).
/// Strips the # prefix and renders bold+underline.
std::string try_heading(const std::string& line) {
  if (line.empty() || line[0] != '#') {
    return "";
  }
  size_t lvl = line.find_first_not_of('#');
  if (lvl == std::string::npos || line[lvl] != ' ') {
    return "";
  }
  return "\033[1;4m" + line.substr(lvl + 1) + "\033[0m\n";
}

/// Try to render a list item (- or * or 1.), including indented sub-items.
/// Preserves nesting depth with additional indentation.
std::string try_list(const std::string& line, bool color) {
  size_t indent = 0;
  while (indent < line.size() && (line[indent] == ' ' || line[indent] == '\t')) {
    indent++;
  }
  std::string rest = line.substr(indent);
  std::string pad(2 + indent, ' ');
  if (rest.size() >= 2 && (rest[0] == '-' || rest[0] == '*') && rest[1] == ' ') {
    return pad + "• " + render_inline(rest.substr(2), color) + "\n";
  }
  if (!rest.empty() && rest[0] >= '1' && rest[0] <= '9') {
    auto dot = rest.find(". ");
    if (dot != std::string::npos && dot <= 3) {
      return pad + rest.substr(0, dot + 2) + render_inline(rest.substr(dot + 2), color) + "\n";
    }
  }
  return "";
}

// --- Table rendering ---
// Table functions (is_table_row, parse_table_cells, visible_length, render_table)
// are in tui/table.cpp (ADR-065 split).

// --- Full rendering ---
// render_markdown is the public entry point. It splits text into lines,
// handles code fences, tables, and dispatches to block/inline renderers.
// Falls back to raw text on any exception — never loses output.

/// Render a single markdown line: dispatches to heading, rule, quote, list, or inline.
/// Returns the rendered line with trailing newline.
std::string render_line(const std::string& line, bool color) {
  if (is_horizontal_rule(line)) {
    return color ? "\033[2m────────────────────\033[0m\n" : "--------------------\n";
  }
  std::string result = try_heading(line);
  if (!result.empty()) {
    return result;
  }
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

/// Render a line inside a code block with syntax highlighting.
/// Extracts language from opening fence, applies highlighter to content lines.
std::string render_code_block_line(const std::string& line, bool& in_code_block, std::string& code_lang) {
  if (line.rfind("```", 0) == 0) {
    if (!in_code_block) {
      // Opening fence — extract language tag
      code_lang = line.size() > 3 ? line.substr(3) : "";
      // Strip whitespace from language tag
      while (!code_lang.empty() && code_lang.back() == ' ') {
        code_lang.pop_back();
      }
    } else {
      code_lang.clear();
    }
    in_code_block = !in_code_block;
    return "\033[2m" + line + "\033[0m\n";
  }
  // Content line — apply syntax highlighting
  return active_highlighter().highlight_line(line, code_lang) + "\n";
}

/// Render multiline text: splits on newlines, handles code fences and tables.
std::string render_lines(const std::string& text, bool color) {
  std::string result;
  bool in_code_block = false;
  std::string code_lang;
  std::string mermaid_buf;
  std::vector<std::string> table_buf;
  size_t pos = 0;
  while (pos < text.size()) {
    auto nl = text.find('\n', pos);
    std::string line = (nl == std::string::npos) ? text.substr(pos) : text.substr(pos, nl - pos);
    pos = (nl == std::string::npos) ? text.size() : nl + 1;
    if (line.rfind("```", 0) == 0 || in_code_block) {
      if (!table_buf.empty()) {
        result += render_table(table_buf, color);
        table_buf.clear();
      }
      // Handle mermaid blocks: buffer content, render on close
      if (in_code_block && code_lang == "mermaid") {
        if (line.rfind("```", 0) == 0) {
          // Closing fence — render the diagram
          std::ostringstream mout;
          if (tui::diagram_registry().render(mermaid_buf, mout)) {
            result += mout.str();
          } else {
            // Fallback: show as normal code block
            result += "\033[2m```mermaid\033[0m\n";
            result += mermaid_buf;
            result += "\033[2m```\033[0m\n";
          }
          mermaid_buf.clear();
          in_code_block = false;
          code_lang.clear();
        } else {
          mermaid_buf += line + "\n";
        }
      } else if (!in_code_block && line.rfind("```", 0) == 0) {
        // Opening fence — extract language, check if mermaid
        code_lang = line.size() > 3 ? line.substr(3) : "";
        while (!code_lang.empty() && code_lang.back() == ' ') {
          code_lang.pop_back();
        }
        in_code_block = true;
        if (code_lang == "mermaid") {
          mermaid_buf.clear();
        } else {
          result += "\033[2m" + line + "\033[0m\n";
        }
      } else {
        result += render_code_block_line(line, in_code_block, code_lang);
      }
    } else if (is_table_row(line)) {
      table_buf.push_back(line);
    } else {
      if (!table_buf.empty()) {
        result += render_table(table_buf, color);
        table_buf.clear();
      }
      result += render_line(line, color);
    }
  }
  if (!table_buf.empty()) {
    result += render_table(table_buf, color);
  }
  return result;
}

/// Public entry point: render markdown to ANSI. Returns raw text on error.
std::string render_markdown(const std::string& text, bool color) {
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
    return text;
  }
}

}  // namespace tui
