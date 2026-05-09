/**
 * @file tui.h
 * @brief Terminal UI helpers — ANSI colors, markdown rendering, spinner.
 *
 * SRP: This header keeps only trivial color/output helpers.
 * Markdown rendering is in tui/markdown.h, spinner in tui/spinner.h.
 *
 * @see docs/adr/adr-016-tui-design.md
 * @see docs/adr/adr-066-solid-refactoring.md
 */
#ifndef TUI_H
#define TUI_H

#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// OCP: new rendering capabilities are added via new files, not by editing this header.
#include "tui/markdown.h"
#include "tui/spinner.h"
#include "tui/table.h"
#include "tui/theme.h"

namespace tui {

/// Global active theme — set at startup, changeable via /theme
inline Theme& active_theme() {
  static Theme t = theme_dark();
  return t;
}

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
    out << active_theme().banner.ansi() << art << ThemeStyle::reset() << "\n";
  } else {
    out << art << "\n";
  }
}

/** Print a dim system message */
inline void system_msg(std::ostream& out, bool color, const std::string& msg) {
  out << (color ? active_theme().system.ansi() : "") << msg << (color ? ThemeStyle::reset() : "") << "\n";
}

/// Format a word as bold+white within a system message (stands out from dim text).
/// Uses \033[1;97m (bold + bright white) and \033[22;39m (bold off + default fg).
/// This avoids \033[0m which would reset the entire system style (dim, color).
/// Used in startup messages to highlight commands like /host, /model, /quit.
inline std::string bold(const std::string& text) {
  return "\033[1;97m" + text + "\033[22;39m";  // bold+bright white on, then bold off + default fg
}

/** Print a bold red error message */
inline void error(std::ostream& out, bool color, const std::string& msg) {
  out << "\n" << (color ? active_theme().error.ansi() : "") << msg << (color ? ThemeStyle::reset() : "") << "\n";
}

/** Print cyan command output */
inline void cmd_output(std::ostream& out, bool color, const std::string& msg) {
  out << (color ? active_theme().info.ansi() : "") << msg << (color ? ThemeStyle::reset() : "") << "\n";
}

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

/** Load spinner messages from a plugin file (one message per line).
 *  Search: .config/spinners/<name>.txt, ~/.llama-cli/spinners/<name>.txt */
inline std::vector<std::string> load_spinner_plugin(const std::string& name) {
  std::vector<std::string> result;
  const char* home = std::getenv("HOME");
  std::string paths[] = {".config/spinners/" + name + ".txt", std::string(home ? home : ".") + "/.llama-cli/spinners/" + name + ".txt"};
  for (const auto& path : paths) {
    std::ifstream f(path);
    if (f.is_open()) {
      std::string line;
      while (std::getline(f, line)) {
        if (!line.empty() && line[0] != '#') result.push_back(line);
      }
      if (!result.empty()) return result;
    }
  }
  return {};
}

/** Get spinner messages for a personality. Plugin file overrides built-in. */
inline std::vector<const char*> personality_messages(const std::string& name) {
  static std::vector<std::string> data;
  static std::vector<const char*> ptrs;
  data = load_spinner_plugin(name);
  if (!data.empty()) {
    ptrs.clear();
    for (const auto& s : data) ptrs.push_back(s.c_str());
    return ptrs;
  }
  if (name == "bofh") return bofh_messages();
  return default_messages();
}

}  // namespace tui

#endif  // TUI_H
