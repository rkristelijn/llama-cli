/**
 * @file repl.cpp
 * @brief Interactive REPL loop with conversation memory and commands.
 * @see docs/adr/adr-012-interactive-repl.md
 * @see docs/adr/adr-014-tool-annotations.md
 */

#include "repl/repl.h"

#include <csignal>
#include <fstream>
#include <sstream>

#include "annotation/annotation.h"
#include "command/command.h"
#include "exec/exec.h"
#include "help.h"
#include "tui/tui.h"

#ifdef LINENOISE_HPP
// already included
#else
#include "linenoise.hpp"
#endif

/// Global flag set by SIGINT handler to interrupt LLM calls
static volatile sig_atomic_t g_interrupted = 0;

/** SIGINT handler — sets flag so spinner/chat can check and abort */
static void sigint_handler(int /*sig*/) { g_interrupted = 1; }

/// REPL session state — groups related data to reduce parameter passing
struct ReplState {
  ChatFn& chat;                   ///< Chat function for LLM interaction
  const Config& cfg;              ///< Configuration (timeouts, etc.)
  std::vector<Message>& history;  ///< Conversation history
  std::istream& in;               ///< Input stream
  std::ostream& out;              ///< Output stream
  int count = 0;                  ///< Number of prompts processed
  bool color = false;             ///< Whether to use ANSI colors (TTY detect)
  bool interactive = false;       ///< Whether running on a real TTY (for spinner)
  bool markdown = true;           ///< Whether to render markdown in LLM output
  bool bofh = false;              ///< BOFH mode: sarcastic spinner
};

/** Get version string from VERSION file + git dirty status.
 * Returns "unknown" if VERSION file is missing.
 * Appends "-dirty" if there are uncommitted changes. */
static std::string get_version() {
  std::string ver = "unknown";
  std::ifstream vf("VERSION");
  if (vf.is_open()) {
    std::getline(vf, ver);
  }
  // Check if working tree is dirty
  if (std::system("git diff --quiet HEAD 2>/dev/null") != 0) {
    ver += "-dirty";
  }
  return ver;
}

/** Show current options state — lists all toggleable runtime settings.
 * Called by /set without arguments, similar to `env` or `set` in bash. */
static void show_options(ReplState& s) {
  s.out << "Options (toggle with /set <option>):\n";
  s.out << "  markdown  " << (s.markdown ? "on" : "off") << "\n";
  s.out << "  color     " << (s.color ? "on" : "off") << "\n";
  s.out << "  bofh      " << (s.bofh ? "on" : "off") << "\n";
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

// Handle a slash command (/help, /clear, /set, /version, /unknown)
// Returns true always — slash commands never exit the REPL loop.
/**
 * @brief Handle a slash command and perform its associated REPL action.
 *
 * Processes recognized commands and writes output to the provided REPL streams
 * or mutates session state as appropriate. Supported commands:
 * - `clear`: clears the conversation history and notifies the user.
 * - `set` / `options`: toggles or shows configurable REPL options via handle_set.
 * - `version`: prints the running program version.
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
  } else if (input.command == "version") {
    s.out << "llama-cli " << get_version() << "\n";
  } else if (input.command == "help") {
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
static void emit_diff_line(std::ostream& out, const char* ansi, const char* prefix,
                           const std::string& line, bool color) {
  if (color) {
    out << ansi << prefix << line << "\033[0m\n";
  } else {
    out << prefix << line << "\n";
  }
}

/**
 * @brief Print a simple line-by-line diff between two text blobs.
 *
 * Compares lines from `old_text` and `new_text` sequentially and emits each
 * line prefixed to indicate its status: unchanged lines are prefixed with
 * two spaces ("  "), deletions with "- ", and additions with "+ ".
 *
 * When `color` is true, deletion prefixes and added prefixes are wrapped in
 * ANSI red and green escape sequences respectively.
 *
 * @param old_text Original text to compare (treated as lines separated by '\n').
 * @param new_text New text to compare (treated as lines separated by '\n').
 * @param out Output stream to which diff lines are written.
 * @param color If true, use ANSI color codes for added/removed line prefixes.
 */
static void show_diff(const std::string& old_text, const std::string& new_text, std::ostream& out, bool color) {
  std::istringstream old_s(old_text);
  std::istringstream new_s(new_text);
  std::string old_line;
  std::string new_line;
  bool has_old = true;
  bool has_new = true;
  while (has_old || has_new) {
    has_old = static_cast<bool>(std::getline(old_s, old_line));
    has_new = static_cast<bool>(std::getline(new_s, new_line));
    if (has_old && has_new && old_line == new_line) {
      out << "  " << old_line << "\n";
    } else {
      if (has_old) {
        emit_diff_line(out, "\033[1;31m", "- ", old_line, color);
      }
      if (has_new) {
        emit_diff_line(out, "\033[1;32m", "+ ", new_line, color);
      }
    }
  }
}

/**
 * @brief Prompt the user to confirm writing a proposed file change.
 *
 * Prompts "Write to <path>? [options]" where options are "[y/n/s/d]" for existing
 * files and "[y/n/s]" for new files. Accepts the following responses on stdin:
 * - `y` or `yes`: confirm and return true.
 * - `n` or `no`: decline and return false.
 * - `s` or `show`: print the proposed file content and re-prompt.
 * - `d` or `diff`: when the target file exists, print a line-by-line diff
 *   between existing and proposed content (uses ANSI colors if `color` is true),
 *   then re-prompt.
 *
 * If input reaches EOF or no confirmation is given, the function returns false.
 *
 * @param action WriteAction containing `path` and `content` for the proposed write.
 * @param in Input stream to read user responses from.
 * @param out Output stream to write prompts, content, and diffs to.
 * @param color When true, diffs are printed with ANSI color codes.
 * @return true if the user confirmed the write with `y` or `yes`, `false` otherwise.
 */
static bool confirm_write(const WriteAction& action, std::istream& in, std::ostream& out, bool color) {
  std::string existing = read_file(action.path);
  bool file_exists = !existing.empty();
  std::string opts = file_exists ? "[y/n/s/d]" : "[y/n/s]";

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
    } else if ((answer == "d" || answer == "diff") && file_exists) {
      show_diff(existing, action.content, out, color);
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
  tui::write_proposal(out, color, action.path);
  if (confirm_write(action, in, out, color)) {
    // Backup existing file before overwriting
    std::string existing = read_file(action.path);
    if (!existing.empty()) {
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
static bool handle_response(const std::string& response, ReplState& s) {
  auto writes = parse_write_annotations(response);
  auto execs = parse_exec_annotations(response);

  if (writes.empty() && execs.empty()) {
    s.out << tui::render_markdown(response, s.color && s.markdown) << "\n";
    return false;
  }

  // Strip all annotations and display clean text
  s.out << tui::render_markdown(strip_exec_annotations(strip_annotations(response)), s.color && s.markdown) << "\n";

  for (const auto& action : writes) {
    process_write(action, s.in, s.out, s.color);
  }
  bool has_exec_output = false;
  for (const auto& cmd : execs) {
    std::string output = confirm_exec(cmd, s.cfg, s.in, s.out);
    if (!output.empty()) {
      s.history.push_back({"user", "[command: " + cmd + "]\n" + output});
      has_exec_output = true;
    }
  }
  return has_exec_output;
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
static bool read_line(std::istream& in, std::ostream& /*out*/, std::string& line, bool color) {
  if (&in == &std::cin) {
    std::string prompt_str = color ? "\033[1;32m> \033[0m" : "> ";
    auto quit = linenoise::Readline(prompt_str.c_str(), line);
    if (quit) {
      return false;
    }
    linenoise::AddHistory(line.c_str());
    return true;
  }
  return static_cast<bool>(std::getline(in, line));
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
 * The abandoned thread will finish in the background.
 * Returns empty string if interrupted, response text otherwise. */
static std::string interruptible_chat(ReplState& s) {
  std::string result;
  std::atomic<bool> done{false};
  std::thread t([&] {
    result = s.chat(s.history);
    done = true;
  });
  while (!done && !g_interrupted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (done) {
    t.join();
    return result;
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
      s.out << tui::render_markdown(followup, s.color && s.markdown) << "\n";
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
int run_repl(ChatFn chat, const Config& cfg, std::istream& in, std::ostream& out) {
  std::string line;
  std::vector<Message> history;
  if (!cfg.system_prompt.empty()) {
    history.push_back({"system", cfg.system_prompt});
  }
  bool is_tty = tui::use_color(cfg.no_color);
  ReplState state = {chat, cfg, history, in, out, 0, is_tty, is_tty, true, cfg.bofh};

  while (read_line(in, out, line, state.color)) {
    if (line.empty()) {
      continue;
    }
    if (!dispatch(line, state)) {
      break;
    }
  }
  return state.count;
}