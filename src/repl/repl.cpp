/**
 * @file repl.cpp
 * @brief Interactive REPL loop with conversation memory and commands.
 * @see docs/adr/adr-012-interactive-repl.md
 * @see docs/adr/adr-014-tool-annotations.md
 */

#include "repl/repl.h"

#include <unistd.h>

#include <csignal>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>

#include "annotation/annotation.h"
#include "command/command.h"
#include "exec/exec.h"
#include "help.h"
#include "trace/trace.h"
#include "tui/tui.h"

#ifdef LINENOISE_HPP
// already included
#else
#include "linenoise.hpp"
#endif

#include "dtl/dtl.hpp"

/// @cond
#ifndef BUILD_TIMEZONE
#define BUILD_TIMEZONE "UTC"
#endif
/// @endcond

/// Global flag set by SIGINT handler to interrupt LLM calls
static volatile sig_atomic_t g_interrupted = 0;

/** SIGINT handler — sets flag so spinner/chat can check and abort */
static void sigint_handler(int /*sig*/) { g_interrupted = 1; }

/// REPL session state — groups related data to reduce parameter passing
struct ReplState {
  ChatFn& chat;                     ///< Chat function for LLM interaction
  const Config& cfg;                ///< Configuration (timeouts, etc.)
  std::vector<Message>& history;    ///< Conversation history
  std::istream& in;                 ///< Input stream
  std::ostream& out;                ///< Output stream
  int count = 0;                    ///< Number of prompts processed
  bool color = false;               ///< Whether to use ANSI colors (TTY detect)
  bool interactive = false;         ///< Whether running on a real TTY (for spinner)
  bool markdown = true;             ///< Whether to render markdown in LLM output
  bool bofh = false;                ///< BOFH mode: sarcastic spinner
  std::string prompt_color = "32";  ///< ANSI code for user prompt (green)
  std::string ai_color = "";        ///< ANSI code for AI response (none=default)
};

/** Get version string from compile-time definition. */
static std::string get_version() {
  std::string ver = LLAMA_CLI_VERSION;
  ver += " (built " __DATE__ " " __TIME__ " " BUILD_TIMEZONE ")";
  return ver;
}

static std::string ansi_to_name(const std::string& code);

/** Show current options state — lists all toggleable runtime settings.
 * Called by /set without arguments, similar to `env` or `set` in bash. */
static void show_options(ReplState& s) {
  s.out << "Options (toggle with /set <option>):\n";
  s.out << "  markdown  " << (s.markdown ? "on" : "off") << "\n";
  s.out << "  color     " << (s.color ? "on" : "off") << "\n";
  s.out << "  bofh      " << (s.bofh ? "on" : "off") << "\n";
  s.out << "  trace     " << (Config::instance().trace ? "on" : "off") << "\n";
  s.out << "Colors (/color prompt|ai <name>):\n";
  s.out << "  prompt    " << ansi_to_name(s.prompt_color) << "\n";
  s.out << "  ai        " << ansi_to_name(s.ai_color) << "\n";
}

/** Toggle a named option, return true if recognized.
 * Uses a lookup table to map option names to ReplState bool fields. */
static bool toggle_option(const std::string& name, ReplState& s) {
  using BoolField = bool ReplState::*;
  struct OptEntry {
    const char* name;
    BoolField field;
  };
  static const OptEntry opts[] = {
      {"markdown", &ReplState::markdown},
      {"color", &ReplState::color},
      {"bofh", &ReplState::bofh},
  };
  if (name == "trace") {
    Config::instance().trace = !Config::instance().trace;
    s.out << "[trace " << (Config::instance().trace ? "on" : "off") << "]\n";
    return true;
  }
  for (const auto& opt : opts) {
    if (name == opt.name) {
      s.*(opt.field) = !(s.*(opt.field));
      s.out << "[" << name << " " << (s.*(opt.field) ? "on" : "off") << "]\n";
      return true;
    }
  }
  return false;
}

// Handle /set command: show options or toggle one.
// Without args: show all options. With args: toggle named option.
/**
 * @brief Handles the "/set" command: toggles a named boolean REPL option.
 *
 * If `arg` is empty, prints the current options. Otherwise extracts the option
 * name up to the first space (exact match required) and toggles that option.
 * If the option name is unrecognized, prints an "Unknown option" message.
 *
 * @param arg Argument string following "/set" (option name optionally followed by additional text).
 * @param s REPL state whose option flags and output stream may be modified or used.
 */
static void handle_set(const std::string& arg, ReplState& s) {
  if (arg.empty()) {
    show_options(s);
    return;
  }
  auto space = arg.find(' ');
  std::string opt = (space != std::string::npos) ? arg.substr(0, space) : arg;
  if (!toggle_option(opt, s)) {
    s.out << "Unknown option: " << arg << ". Type /set to see available options.\n";
  }
}

// Map color name to ANSI code
static std::string color_name_to_ansi(const std::string& name) {
  if (name == "black") return "30";
  if (name == "red") return "31";
  if (name == "green") return "32";
  if (name == "yellow") return "33";
  if (name == "blue") return "34";
  if (name == "magenta") return "35";
  if (name == "cyan") return "36";
  if (name == "white") return "37";
  if (name == "bright-red") return "91";
  if (name == "bright-green") return "92";
  if (name == "bright-yellow") return "93";
  if (name == "bright-blue") return "94";
  if (name == "bright-magenta") return "95";
  if (name == "bright-cyan") return "96";
  if (name == "bright-white") return "97";
  if (name == "orange") return "38;5;208";
  if (name == "pink") return "38;5;213";
  if (name == "purple") return "38;5;129";
  if (name == "lime") return "38;5;118";
  if (name == "none" || name == "default") return "";
  return "";
}

// Map ANSI code back to color name for display
static std::string ansi_to_name(const std::string& code) {
  if (code == "30") return "black";
  if (code == "31") return "red";
  if (code == "32") return "green";
  if (code == "33") return "yellow";
  if (code == "34") return "blue";
  if (code == "35") return "magenta";
  if (code == "36") return "cyan";
  if (code == "37") return "white";
  if (code == "91") return "bright-red";
  if (code == "92") return "bright-green";
  if (code == "93") return "bright-yellow";
  if (code == "94") return "bright-blue";
  if (code == "95") return "bright-magenta";
  if (code == "96") return "bright-cyan";
  if (code == "97") return "bright-white";
  if (code == "38;5;208") return "orange";
  if (code == "38;5;213") return "pink";
  if (code == "38;5;129") return "purple";
  if (code == "38;5;118") return "lime";
  return "none";
}

// Save or update a key=value in .env file
static bool save_to_dotenv(const std::string& key, const std::string& value) {
  std::string path = ".env";
  std::vector<std::string> lines;
  bool found = false;
  std::ifstream in(path);
  if (in.is_open()) {
    std::string line;
    while (std::getline(in, line)) {
      std::string prefix = key + "=";
      if (line.compare(0, prefix.size(), prefix) == 0) {
        line = key + "=" + value;
        found = true;
      }
      lines.push_back(line);
    }
    in.close();
  }
  if (!found) {
    lines.push_back(key + "=" + value);
  }
  std::ofstream out(path);
  if (!out.is_open()) {
    return false;
  }
  for (const auto& l : lines) {
    out << l << "\n";
  }
  return true;
}

// Handle /color command: /color prompt <name>, /color ai <name>
static void handle_color(const std::string& arg, ReplState& s) {
  static const char* names =
      "black, red, green, yellow, blue, magenta, cyan, white,\n"
      "         bright-red, bright-green, bright-yellow, bright-blue,\n"
      "         bright-magenta, bright-cyan, bright-white,\n"
      "         orange, pink, purple, lime, none";
  if (arg.empty()) {
    s.out << "Usage: /color prompt <color>  or  /color ai <color>\n";
    s.out << "Colors: " << names << "\n";
    s.out << "Current: prompt=" << ansi_to_name(s.prompt_color) << ", ai=" << ansi_to_name(s.ai_color) << "\n";
    return;
  }
  auto space = arg.find(' ');
  if (space == std::string::npos) {
    s.out << "Usage: /color prompt <color>  or  /color ai <color>\n";
    return;
  }
  std::string target = arg.substr(0, space);
  std::string cname = arg.substr(space + 1);
  std::string code = color_name_to_ansi(cname);
  if (code.empty() && cname != "none" && cname != "default") {
    s.out << "Unknown color: " << cname << ". Options: " << names << "\n";
    return;
  }
  if (target == "prompt") {
    s.prompt_color = code;
    bool ok = save_to_dotenv("LLAMA_PROMPT_COLOR", cname);
    s.out << "[prompt color set to " << cname << (ok ? " (saved)" : " (save failed)") << "]\n";
  } else if (target == "ai") {
    s.ai_color = code;
    bool ok = save_to_dotenv("LLAMA_AI_COLOR", cname);
    s.out << "[ai color set to " << cname << (ok ? " (saved)" : " (save failed)") << "]\n";
  } else {
    s.out << "Unknown target: " << target << ". Use 'prompt' or 'ai'.\n";
  }
}

// Handle a slash command (/help, /clear, /set, /version, /unknown)
// Returns true always — slash commands never exit the REPL loop.
/**
 * @brief Interactive model selection menu with numbered list.
 *
 * Fetches available models from Ollama server, displays them with numbers,
 * and prompts user to select one by entering its number. Updates Config
 * with the selected model. Shows error message if no models available.
 *
 * @param s REPL state used for I/O and config access.
 */
static void handle_model_selection(ReplState& s) {
  // Fetch available models from server
  std::vector<std::string> models = get_available_models(s.cfg);

  if (models.empty()) {
    s.out << "No models available on " << s.cfg.host << ":" << s.cfg.port << "\n";
    return;
  }

  // Display numbered list of models
  s.out << "Available models:\n";
  for (size_t i = 0; i < models.size(); i++) {
    s.out << "  " << (i + 1) << ". " << models[i] << "\n";
  }

  // Prompt user to select by number
  s.out << "Select model (1-" << models.size() << "): ";
  s.out.flush();

  std::string input;
  if (!std::getline(s.in, input)) {
    s.out << "\n[cancelled]\n";
    return;
  }

  // Parse selection number
  int choice = 0;
  try {
    choice = std::stoi(input);
  } catch (...) {
    s.out << "[invalid input]\n";
    return;
  }

  // Validate range (1-indexed for user, 0-indexed for vector)
  if (choice < 1 || choice > static_cast<int>(models.size())) {
    s.out << "[out of range]\n";
    return;
  }

  // Update config with selected model
  std::string selected = models[choice - 1];
  Config::instance().model = selected;
  s.out << "[model set to " << selected << "]\n";
}

/**
 * @brief Handle a slash command and perform its associated REPL action.
 *
 * Processes recognized commands and writes output to the provided REPL streams
 * or mutates session state as appropriate. Supported commands:
 * - `clear`: clears the conversation history and notifies the user.
 * - `set` / `options`: toggles or shows configurable REPL options via handle_set.
 * - `version`: prints the running program version.
 * - `model`: interactive model selection menu.
 * - `help`: prints the REPL help text.
 * Unknown commands produce an informational "Unknown command" message.
 *
 * @param input Parsed slash command; `input.command` is the command name and
 *              `input.arg` contains any following argument text.
 * @param s REPL state used for I/O and session data (may be modified).
 * @return true Always returns `true` to indicate the REPL should continue.
 */
static bool handle_command(const ParsedInput& input, ReplState& s) {
  if (input.command == "clear") {
    s.history.clear();
    s.out << "[history cleared]\n";
  } else if (input.command == "set" || input.command == "options") {
    handle_set(input.arg, s);
  } else if (input.command == "color") {
    handle_color(input.arg, s);
  } else if (input.command == "model") {
    handle_model_selection(s);
  } else if (input.command == "version") {
    s.out << "llama-cli " << get_version() << "\n";
  } else if (input.command == "help" || input.command.empty()) {
    s.out << help::repl;
  } else {
    s.out << "Unknown command: /" << input.command << ". Type /help for options.\n";
  }
  return true;
}

/**
 * @brief Read the entire contents of a file into a string.
 *
 * @param path Path to the file to read.
 * @return std::string File contents. Returns an empty string if the file cannot be opened.
 */
static std::string read_file(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) {
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

/** Emit one diff line with optional ANSI color.
 * prefix is "- " (red) or "+ " (green); reset is applied after the line when color is on. */
static void emit_diff_line(std::ostream& out, const char* ansi, const char* prefix, const std::string& line, bool color) {
  if (color) {
    out << ansi << prefix << line << "\033[0m\n";
  } else {
    out << prefix << line << "\n";
  }
}

/**
 * @brief Print a git-style LCS diff between two text blobs using dtl.
 *
 * Uses Myers diff (via dtl) for accurate change detection — inserted/deleted
 * blocks are shown with +/- prefixes; unchanged lines with two spaces.
 * ANSI colors applied when color is true.
 */
static void show_diff(const std::string& old_text, const std::string& new_text, std::ostream& out, bool color) {
  // Split into lines
  auto split = [](const std::string& s) {
    std::vector<std::string> lines;
    std::istringstream ss(s);
    std::string line;
    while (std::getline(ss, line)) {
      lines.push_back(line);
    }
    return lines;
  };
  auto old_lines = split(old_text);
  auto new_lines = split(new_text);

  dtl::Diff<std::string> diff(old_lines, new_lines);
  diff.compose();

  for (const auto& elem : diff.getSes().getSequence()) {
    const auto& line = elem.first;
    switch (elem.second.type) {
      case dtl::SES_DELETE:
        emit_diff_line(out, "\033[1;31m", "- ", line, color);
        break;
      case dtl::SES_ADD:
        emit_diff_line(out, "\033[1;32m", "+ ", line, color);
        break;
      default:
        out << "  " << line << "\n";
        break;
    }
  }
}

/**
 * @brief Prompt the user to confirm writing a proposed file change.
 *
 * Always shows a diff (for existing files) or the full content (for new files)
 * before prompting. Accepts y/yes to confirm, n/no to decline, s/show to
 * re-display content.
 */
// todo: reduce complexity of confirm_write
// pmccabe:skip-complexity
static bool confirm_write(const WriteAction& action, std::istream& in, std::ostream& out, bool color) {
  std::ifstream check(action.path);
  bool file_exists = check.good();
  check.close();
  std::string existing = file_exists ? read_file(action.path) : "";

  // Always show diff / content before prompting
  if (file_exists) {
    show_diff(existing, action.content, out, color);
  } else {
    out << action.content << "\n";
  }

  std::string opts = "[y/n/s]";
  out << "Write to " << action.path << "? " << opts << " " << std::flush;
  std::string answer;
  while (std::getline(in, answer)) {
    if (answer == "y" || answer == "yes") {
      return true;
    }
    if (answer == "n" || answer == "no") {
      return false;
    }
    if (answer == "s" || answer == "show") {
      out << action.content << "\n";
    }
    out << "Write to " << action.path << "? " << opts << " " << std::flush;
  }
  return false;
}

/**
 * @brief Present a proposed file write to the user and perform the write only if confirmed.
 *
 * Presents a write proposal for action.path, prompts the user for confirmation, and if
 * confirmed preserves any existing content by writing it to `path.bak` (when non-empty)
 * before overwriting the target file with action.content (appends a trailing newline).
 * Writes status messages to out: "[wrote <path>]" on success, a tui::error message on
 * failure to open the target file, or "[skipped]" when the user declines.
 *
 * @param action The write action containing `path` and `content` to be written.
 * @param in Input stream used to read the user's confirmation responses.
 * @param out Output stream used for prompts and status messages.
 * @param color If true, enable ANSI-colored output where supported.
 */
static void process_write(const WriteAction& action, std::istream& in, std::ostream& out, bool color) {
  if (confirm_write(action, in, out, color)) {
    // Backup existing file before overwriting
    std::ifstream exists_check(action.path);
    if (exists_check.good()) {
      exists_check.close();
      std::string existing = read_file(action.path);
      std::ofstream bak(action.path + ".bak");
      if (bak.is_open()) {
        bak << existing;
      }
    }
    std::ofstream file(action.path);
    if (file.is_open()) {
      file << action.content << "\n";
      out << "[wrote " << action.path << "]\n";
    } else {
      tui::error(out, color, "Error: could not write to " + action.path);
    }
  } else {
    out << "[skipped]\n";
  }
}

/**
 * @brief Apply a <str_replace> action: show diff, prompt, then do targeted replacement.
 */
static void process_str_replace(const StrReplaceAction& action, std::istream& in, std::ostream& out, bool color) {
  std::ifstream check(action.path);
  if (!check.good()) {
    tui::error(out, color, "str_replace: file not found: " + action.path);
    return;
  }
  std::string existing = read_file(action.path);
  if (existing.find(action.old_str) == std::string::npos) {
    tui::error(out, color, "str_replace: old string not found in " + action.path);
    return;
  }

  // Compute updated file for preview
  std::string updated = existing;
  auto replace_pos = updated.find(action.old_str);
  updated.replace(replace_pos, action.old_str.size(), action.new_str);
  show_diff(existing, updated, out, color);

  out << "Apply str_replace to " << action.path << "? [y/n] " << std::flush;
  std::string answer;
  if (!std::getline(in, answer) || (answer != "y" && answer != "yes")) {
    out << "[skipped]\n";
    return;
  }

  // Backup and write
  {
    std::ofstream bak(action.path + ".bak");
    bak << existing;
  }
  std::ofstream f(action.path);
  if (f.is_open()) {
    f << updated;
    out << "[wrote " << action.path << "]\n";
  } else {
    tui::error(out, color, "Error: could not write to " + action.path);
  }
}

/**
 * @brief Execute a <read> action and return the result as a context string for the LLM.
 *
 * Reads the requested lines or searches for a term, returning the content
 * so it can be injected into the conversation history.
 */
// todo: reduce complexity of process_read
// pmccabe:skip-complexity
// NOLINTNEXTLINE(readability-function-size)
static std::string process_read(const ReadAction& action, std::ostream& out, bool color) {
  std::ifstream check(action.path);
  if (!check.good()) {
    tui::error(out, color, "read: file not found: " + action.path);
    return "";
  }
  std::string content = read_file(action.path);

  // Split into lines (1-based)
  std::vector<std::string> lines;
  std::istringstream ss(content);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }

  std::ostringstream result;
  result << "[file: " << action.path;

  if (!action.search.empty()) {
    // Search mode: return lines containing the term with context (±3 lines)
    result << " search=\"" << action.search << "\"]\n";
    for (size_t i = 0; i < lines.size(); ++i) {
      if (lines[i].find(action.search) != std::string::npos) {
        size_t from = (i >= 3) ? i - 3 : 0;
        size_t to = std::min(i + 3, lines.size() - 1);
        for (size_t j = from; j <= to; ++j) {
          result << (j + 1) << ": " << lines[j] << "\n";
        }
        result << "---\n";
      }
    }
  } else if (action.from_line > 0 && action.to_line > 0) {
    // Line range mode
    int from = std::max(1, action.from_line);
    int to = std::min((int)lines.size(), action.to_line);
    result << " lines=" << from << "-" << to << "]\n";
    for (int i = from; i <= to; ++i) {
      result << i << ": " << lines[i - 1] << "\n";
    }
  } else {
    // Full file
    result << "]\n" << content;
  }

  std::string r = result.str();
  return r;
}

/**
 * @brief Extracts command strings enclosed in <exec>...</exec> tags.
 *
 * Scans the input text left-to-right and collects the inner contents of each
 * complete `<exec>`...`</exec>` block found.
 *
 * @return std::vector<std::string> Vector of command strings in the order they
 * appear. Returns an empty vector if no complete exec blocks are found or if a
 * closing tag is missing for a detected opening tag.
 */
static std::vector<std::string> parse_exec_annotations(const std::string& text) {
  std::vector<std::string> cmds;
  const std::string open = "<exec>";
  const std::string close = "</exec>";
  size_t pos = 0;
  while (pos < text.size()) {
    auto start = text.find(open, pos);
    if (start == std::string::npos) {
      break;
    }
    auto end = text.find(close, start + open.size());
    if (end == std::string::npos) {
      break;
    }
    cmds.push_back(text.substr(start + open.size(), end - start - open.size()));
    pos = end + close.size();
  }
  return cmds;
}

// Strip <exec> annotations from text, replacing with readable summary.
// Used to display clean response text to the user without raw XML tags.
/**
 * @brief Replace every `<exec>...</exec>` block with a readable placeholder.
 *
 * Scans the input text for well-formed `<exec>...</exec>` tags and replaces each
 * occurrence with "[proposed: exec <cmd>]" where `<cmd>` is the enclosed content.
 * Tags that are not properly closed are left unchanged.
 *
 * @param text Input string that may contain `<exec>` annotation blocks.
 * @return std::string A new string with all complete `<exec>` blocks replaced by their placeholders.
 */
static std::string strip_exec_annotations(const std::string& text) {
  std::string result = text;
  const std::string open = "<exec>";
  const std::string close = "</exec>";
  while (true) {
    auto start = result.find(open);
    if (start == std::string::npos) {
      break;
    }
    auto end = result.find(close, start);
    if (end == std::string::npos) {
      break;
    }
    std::string cmd = result.substr(start + open.size(), end - start - open.size());
    result.replace(start, end + close.size() - start, "[proposed: exec " + cmd + "]");
  }
  return result;
}

/** Confirm and execute an LLM-proposed command (ADR-015)
 * Prompts user with y/n, executes on confirmation
 * Returns output if confirmed, empty string if declined */
static std::string confirm_exec(const std::string& cmd, const Config& cfg, std::istream& in, std::ostream& out) {
  out << "Run: " << cmd << "? [y/n] " << std::flush;
  std::string answer;
  if (!std::getline(in, answer)) {
    return "";
  }
  if (answer != "y" && answer != "yes") {
    out << "[skipped]\n";
    return "";
  }
  auto r = cmd_exec(cmd, cfg.exec_timeout, cfg.max_output);
  out << r.output;
  return r.output;
}

/**
 * @brief Process an LLM response for file-write and shell-exec annotations and handle them.
 *
 * If the response contains no write or exec annotations, the raw response is rendered
 * as markdown and printed. If annotations are present, the response is printed with
 * annotations removed, each write action is offered for confirmation and applied as
 * requested, and each proposed exec command is offered for confirmation and executed.
 *
 * @param response Full textual response from the LLM, possibly containing annotations.
 * @param s REPL state used for I/O, configuration, and conversation history.
 * @return true if any exec command produced output that was appended to the conversation history; false otherwise.
 */
// todo: reduce complexity of handle_response
// Wrap rendered AI text with the configured AI color.
// Re-applies AI color after any ANSI reset inside markdown rendering.
static std::string colorize_ai(const std::string& text, const ReplState& s) {
  if (!s.color || s.ai_color.empty()) return text;
  std::string color_code = "\033[" + s.ai_color + "m";
  std::string result = color_code;
  std::string reset = "\033[0m";
  size_t pos = 0;
  size_t found;
  while ((found = text.find(reset, pos)) != std::string::npos) {
    result += text.substr(pos, found - pos + reset.size()) + color_code;
    pos = found + reset.size();
  }
  result += text.substr(pos) + "\033[0m";
  return result;
}

// pmccabe:skip-complexity
// NOLINTNEXTLINE(readability-function-size)
static bool handle_response(const std::string& response, ReplState& s) {
  auto writes = parse_write_annotations(response);
  auto str_replaces = parse_str_replace_annotations(response);
  auto reads = parse_read_annotations(response);
  auto execs = parse_exec_annotations(response);

  if (writes.empty() && str_replaces.empty() && reads.empty() && execs.empty()) {
    s.out << colorize_ai(tui::render_markdown(response, s.color && s.markdown), s) << "\n";
    return false;
  }

  // Strip all annotations and display clean text
  s.out << colorize_ai(tui::render_markdown(strip_exec_annotations(strip_annotations(response)), s.color && s.markdown), s) << "\n";

  for (const auto& action : writes) {
    process_write(action, s.in, s.out, s.color);
  }
  for (const auto& action : str_replaces) {
    process_str_replace(action, s.in, s.out, s.color);
  }
  bool has_followup = false;
  std::set<std::string> seen_reads;
  for (const auto& action : reads) {
    std::string key = action.path + "|" + std::to_string(action.from_line) + "-" + std::to_string(action.to_line) + "|" + action.search;
    if (!seen_reads.insert(key).second) {
      continue;  // skip exact duplicate read
    }
    std::string ctx = process_read(action, s.out, s.color);
    if (!ctx.empty()) {
      s.history.push_back({"user", ctx});
      has_followup = true;
    }
  }
  for (const auto& cmd : execs) {
    std::string output = confirm_exec(cmd, s.cfg, s.in, s.out);
    if (!output.empty()) {
      s.history.push_back({"user", "[command: " + cmd + "]\n" + output});
      has_followup = true;
    }
  }
  return has_followup;
}

// Read one line of input using linenoise (interactive) or getline (tests).
// Linenoise is used when reading from stdin (real TTY) for history/arrows.
// Falls back to plain getline for non-interactive use (pipes, test harness).
/**
 * @brief Reads a single input line from the given stream, using linenoise when reading from stdin.
 *
 * When `in` is `std::cin` this emits a prompt (colorized when `color` is true), reads user input via
 * linenoise, and adds successful entries to the linenoise history. For non-stdin streams it reads a
 * line using `std::getline`.
 *
 * @param in Input stream to read from.
 * @param out Unused output stream parameter (kept for API symmetry).
 * @param line Output parameter that receives the read line.
 * @param color When true and reading from `std::cin`, the prompt is colorized.
 * @return bool `false` on EOF or user-initiated quit, `true` if a line was successfully read.
 */
static bool read_line(std::istream& in, std::ostream& /*out*/, std::string& line, bool color, const std::string& prompt_ansi = "32") {
  // Use std::getline for non-stdin streams (tests, pipes)
  if (&in != &std::cin) {
    return static_cast<bool>(std::getline(in, line));
  }
  // Use std::getline when stdin is not a TTY (piped input, --repl mode)
  if (!isatty(STDIN_FILENO)) {
    return static_cast<bool>(std::getline(in, line));
  }
  // Interactive mode: use linenoise with colored prompt
  std::string prompt_str = (color && !prompt_ansi.empty()) ? "\033[1;" + prompt_ansi + "m> \033[0m" : "> ";
  auto quit = linenoise::Readline(prompt_str.c_str(), line);
  if (quit) {
    return false;
  }
  linenoise::AddHistory(line.c_str());
  return true;
}

// Execute a command and optionally add output to history.
// ! (add_to_history=false): output goes to terminal only, LLM doesn't see it.
/**
 * @brief Execute a shell command, print its output, and optionally add the output to the REPL history.
 *
 * Executes the provided command using the session's execution settings, writes the command output
 * to the REPL output stream, and—if `add_to_history` is true—appends a user-formatted message
 * containing the command and its output to the session history. If the command exits with a
 * non-zero code and did not time out, an exit-code error indicator is printed.
 *
 * @param cmd The command to execute.
 * @param add_to_history When true, injects the command output into `s.history` as a user message
 *                       with the format: "[command: <cmd>]\n<output>".
 * @param s The REPL state providing execution configuration, input/output streams, and history.
 */
static void run_exec(const std::string& cmd, bool add_to_history, ReplState& s) {
  auto r = cmd_exec(cmd, s.cfg.exec_timeout, s.cfg.max_output);
  tui::cmd_output(s.out, s.color, r.output);
  if (r.exit_code != 0 && !r.timed_out) {
    tui::error(s.out, s.color, "[exit code: " + std::to_string(r.exit_code) + "]");
  }
  if (add_to_history) {
    s.history.push_back({"user", "[command: " + cmd + "]\n" + r.output});
  }
}

/** Run chat on a thread, interruptible by Ctrl+C.
 * Polls g_interrupted every 50ms. If set, detaches the HTTP thread
 * so the user gets their prompt back immediately.
 * Uses shared_ptr for result to avoid use-after-free on detach. */
static std::string interruptible_chat(ReplState& s) {
  auto result = std::make_shared<std::string>();
  auto done = std::make_shared<std::atomic<bool>>(false);
  std::thread t([&s, result, done] {
    *result = s.chat(s.history);
    *done = true;
  });
  while (!*done && !g_interrupted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (*done) {
    t.join();
    return *result;
  }
  t.detach();
  return "";
}

/** Call LLM with spinner, interruptible by Ctrl+C.
 * Installs SIGINT handler, starts spinner, calls chat on a thread.
 * Returns response text, or empty string if interrupted. */
static std::string chat_with_spinner(ReplState& s) {
  g_interrupted = 0;
  auto prev = std::signal(SIGINT, sigint_handler);
  Spinner spin(s.out, s.interactive, s.bofh ? tui::bofh_messages() : tui::default_messages());
  std::string result = interruptible_chat(s);
  std::signal(SIGINT, prev);
  return result;
}

// Send a prompt to the LLM and handle the response.
// Manages spinner, SIGINT, history, annotations, and follow-up calls.
// A follow-up call is triggered when the LLM proposes an exec and user
/**
 * @brief Sends a user prompt to the chat backend, manages history, and handles the assistant's response(s).
 *
 * Appends the user's prompt to the session history, invokes the chat (with a spinner),
 * prints and records the assistant response, and processes any write/exec annotations.
 * If processing indicates a follow-up is needed, requests a follow-up response and prints
 * and records it as well. Handles SIGINT interruptions by printing "[interrupted]" and
 * undoing or stopping history updates as appropriate.
 *
 * @param line The user input line to send as a prompt.
 * @param s REPL state containing chat, configuration, I/O streams, and conversation history.
 */
static void send_prompt(const std::string& line, ReplState& s) {
  // Trace output for debugging loop behavior (ADR-028)
  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] iteration=%d prompt=%.50s\n", s.count, line.c_str());
  }

  s.history.push_back({"user", line});
  std::string response = chat_with_spinner(s);
  if (g_interrupted) {
    s.out << "\n[interrupted]\n";
    s.history.pop_back();
    g_interrupted = 0;
    return;
  }
  s.history.push_back({"assistant", response});
  bool needs_followup = handle_response(response, s);
  s.count++;

  if (needs_followup) {
    std::string followup = chat_with_spinner(s);
    if (!g_interrupted) {
      s.out << colorize_ai(tui::render_markdown(followup, s.color && s.markdown), s) << "\n";
      s.history.push_back({"assistant", followup});
    } else {
      s.out << "\n[interrupted]\n";
      g_interrupted = 0;
    }
  }
}

// Dispatch a single REPL input: command, exec, prompt, or exit.
// Order matters: exit and commands are checked before exec and LLM prompts.
/**
 * @brief Parse a single REPL line and dispatch it to the appropriate handler.
 *
 * Parses the provided input line, executes the matched action (command, exec,
 * exec-with-context, or standard prompt), and updates REPL state as needed.
 *
 * @param line The raw input line entered by the user.
 * @param s Current REPL state used by handlers and updated by prompt handling.
 * @return bool `false` if the input signals the REPL should exit, `true` otherwise.
 */
static bool dispatch(const std::string& line, ReplState& s) {
  auto input = parse_input(line);
  if (input.type == InputType::Exit) {
    return false;
  }
  if (input.type == InputType::Command) {
    handle_command(input, s);
    return true;
  }
  if (input.type == InputType::Exec) {
    run_exec(input.arg, false, s);
    return true;
  }
  if (input.type == InputType::ExecContext) {
    run_exec(input.arg, true, s);
    return true;
  }
  send_prompt(line, s);
  return true;
}

/** Main REPL loop: read → parse → dispatch → respond
 * Returns number of LLM prompts processed (excludes ! and !! commands) */
// Tab-completion for slash commands
static void slash_completion(const char* buf, std::vector<std::string>& completions) {
  static const std::vector<std::string> cmds = {"/clear", "/color", "/model", "/set", "/version", "/help"};
  std::string input(buf);
  if (input.empty()) {
    return;
  }
  if (input[0] != '/') {
    return;
  }
  for (const auto& cmd : cmds) {
    if (cmd.compare(0, input.size(), input) == 0) {
      completions.push_back(cmd);
    }
  }
}

/** Main REPL loop: read input, dispatch commands/prompts, return prompt count. */
int run_repl(ChatFn chat, const Config& cfg, std::istream& in, std::ostream& out) {
  std::string line;
  std::vector<Message> history;
  if (!cfg.system_prompt.empty()) {
    history.push_back({"system", cfg.system_prompt});
  }
  bool is_tty = tui::use_color(cfg.no_color);
  linenoise::SetCompletionCallback(slash_completion);
  ReplState state = {chat,
                     cfg,
                     history,
                     in,
                     out,
                     0,
                     is_tty,
                     is_tty,
                     true,
                     cfg.bofh,
                     color_name_to_ansi(cfg.prompt_color),
                     color_name_to_ansi(cfg.ai_color)};

  while (read_line(in, out, line, state.color, state.prompt_color)) {
    if (line.empty()) {
      continue;
    }
    if (!dispatch(line, state)) {
      break;
    }
  }
  return state.count;
}