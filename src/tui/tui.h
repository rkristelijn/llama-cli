/**
 * @file tui.h
 * @brief Terminal UI helpers — ANSI colors, markdown rendering, spinner.
 * @see docs/adr/adr-016-tui-design.md
 */
#ifndef TUI_H
#define TUI_H

#include <unistd.h>

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

/** Render inline markdown: **bold**, *italic*, `code`.
 * Walks the string char-by-char, matching paired delimiters.
 * Returns the original string unchanged when color is disabled. */
inline std::string render_inline(const std::string& line, bool color) {
  if (!color) {
    return line;
  }
  std::string out;
  size_t i = 0;
  while (i < line.size()) {
    if (try_bold(line, i, out)) {
      continue;
    }
    if (try_italic(line, i, out)) {
      continue;
    }
    if (try_code(line, i, out)) {
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

/** Try to render a list item (- or * or 1.). Returns empty if not a list. */
inline std::string try_list(const std::string& line, bool color) {
  // Unordered: - item or * item
  if (line.size() >= 2 && (line[0] == '-' || line[0] == '*') && line[1] == ' ') {
    return "  • " + render_inline(line.substr(2), color) + "\n";
  }
  // Ordered: 1. item
  if (!line.empty() && line[0] >= '1' && line[0] <= '9') {
    auto dot = line.find(". ");
    if (dot != std::string::npos && dot <= 3) {
      return "  " + line.substr(0, dot + 2) + render_inline(line.substr(dot + 2), color) + "\n";
    }
  }
  return "";
}

/** Render a single markdown line (not inside a code block) to ANSI */
inline std::string render_line(const std::string& line, bool color) {
  std::string result = try_heading(line);
  if (!result.empty()) {
    return result;
  }
  result = try_list(line, color);
  if (!result.empty()) {
    return result;
  }
  return render_inline(line, color) + "\n";
}

/**
 * Render a single line that is either inside a code block or at a code-fence boundary.
 *
 * If `line` starts with a triple-backtick fence (```), toggles `in_code_block` to enter or
 * exit a code block before returning the rendered output.
 *
 * @param line The input line to render.
 * @param in_code_block Reference to the current code-block state; toggled when a fence is encountered.
 * @returns The input line wrapped with cyan ANSI styling and terminated with a newline.
 */
inline std::string render_code_block_line(const std::string& line, bool& in_code_block) {
  if (line.rfind("```", 0) == 0) {
    in_code_block = !in_code_block;
    return "\033[36m" + line + "\033[0m\n";
  }
  return "\033[36m" + line + "\033[0m\n";
}

/** Render a full markdown text block to ANSI.
 * Handles: # headings, **bold**, *italic*, `code`, ```code blocks```, -
 * lists, 1. lists. Pluggable: replace this function to use a different
 * renderer. */
/**
 * Render multiline Markdown-like text into an ANSI-styled string.
 *
 * Splits the input on newline boundaries and renders each line using block
 * and inline renderers; lines that start or are inside a triple-backtick code
 * fence are rendered as code block lines.
 *
 * @param text Input text that may contain multiple lines and Markdown-like constructs.
 * @param color If `true`, apply ANSI styling to rendered output; if `false`, return content without styling.
 * @returns The concatenated rendered output with per-line rendering and preserved line breaks.
 */
inline std::string render_lines(const std::string& text, bool color) {
  std::string result;
  bool in_code_block = false;
  size_t pos = 0;
  while (pos < text.size()) {
    auto nl = text.find('\n', pos);
    std::string line = (nl == std::string::npos) ? text.substr(pos) : text.substr(pos, nl - pos);
    pos = (nl == std::string::npos) ? text.size() : nl + 1;
    if (line.rfind("```", 0) == 0 || in_code_block) {
      result += render_code_block_line(line, in_code_block);
    } else {
      result += render_line(line, color);
    }
  }
  return result;
}

/** Render a full markdown text block to ANSI.
 * Pluggable: replace this function to use a different renderer. */
inline std::string render_markdown(const std::string& text, bool color) {
  if (!color) {
    return text;
  }
  std::string result = render_lines(text, color);
  if (!result.empty() && result.back() == '\n' && !text.empty() && text.back() != '\n') {
    result.pop_back();
  }
  return result;
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

/** RAII spinner — shows animation while LLM is processing, stops on
 * destruction. Only active when active=true (interactive TTY). Construct before
 * a blocking call, destructor cleans up automatically. */
class Spinner {
 public:
  /** Start spinner with custom messages. No-op if active=false. */
  Spinner(std::ostream& out, bool active, std::vector<const char*> msgs = tui::default_messages())
      : out_(out), running_(active), msgs_(std::move(msgs)) {
    if (running_) {
      thread_ = std::thread([this] { run(); });
    }
  }
  ~Spinner() {
    running_ = false;
    if (thread_.joinable()) {
      thread_.join();
    }
    out_ << "\r\033[K" << std::flush;
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
