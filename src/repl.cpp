/**
 * @file repl.cpp
 * @brief Interactive REPL loop with conversation memory and commands.
 * @see docs/adr/adr-012-interactive-repl.md
 * @see docs/adr/adr-014-tool-annotations.md
 */

#include "repl.h"

#include <csignal>
#include <fstream>

#include "annotation.h"
#include "command.h"
#include "exec.h"
#include "tui.h"

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
  bool markdown = true;           ///< Whether to render markdown in LLM output
  bool bofh = false;              ///< BOFH mode: sarcastic spinner
};

/** Get version string from VERSION file + git dirty status */
static std::string get_version() {
  std::string ver = "unknown";
  std::ifstream vf("VERSION");
  if (vf.is_open()) std::getline(vf, ver);
  // Check if working tree is dirty
  if (std::system("git diff --quiet HEAD 2>/dev/null") != 0) {
    ver += "-dirty";
  }
  return ver;
}

/** Show current options state — lists all toggleable runtime settings */
static void show_options(ReplState& s) {
  s.out << "Options (toggle with /set <option>):\n";
  s.out << "  markdown  " << (s.markdown ? "on" : "off") << "\n";
  s.out << "  color     " << (s.color ? "on" : "off") << "\n";
  s.out << "  bofh      " << (s.bofh ? "on" : "off") << "\n";
}

/** Toggle a named option, return true if recognized.
 * Supports: markdown, color, bofh */
static bool toggle_option(const std::string& name, ReplState& s) {
  if (name == "markdown") {
    s.markdown = !s.markdown;
  } else if (name == "color") {
    s.color = !s.color;
  } else if (name == "bofh") {
    s.bofh = !s.bofh;
  } else {
    return false;
  }
  s.out << "[" << name << " "
        << (name == "markdown" ? (s.markdown ? "on" : "off")
            : name == "color"  ? (s.color ? "on" : "off")
                               : (s.bofh ? "on" : "off"))
        << "]\n";
  return true;
}

// Handle a slash command (/help, /clear, /options, /set, /version, /unknown)
// Returns true always — commands don't exit the loop
static bool handle_command(const ParsedInput& input, ReplState& s) {
  if (input.command == "clear") {
    s.history.clear();
    s.out << "[history cleared]\n";
  } else if (input.command == "set" && !input.arg.empty()) {
    // Extract just the option name (ignore extra args like "on"/"off")
    auto space = input.arg.find(' ');
    std::string opt = (space != std::string::npos) ? input.arg.substr(0, space) : input.arg;
    if (!toggle_option(opt, s)) {
      s.out << "Unknown option: " << input.arg << ". Type /set to see available options.\n";
    }
  } else if (input.command == "set" || input.command == "options") {
    show_options(s);
  } else if (input.command == "version") {
    s.out << "llama-cli " << get_version() << "\n";
  } else if (input.command == "help") {
    s.out << "Commands:\n";
    s.out << "  !command      Run command, output to terminal\n";
    s.out << "  !!command     Run command, output as LLM context\n";
    s.out << "  /clear        Clear conversation history\n";
    s.out << "  /set          Show options\n";
    s.out << "  /set <opt>    Toggle option (markdown, color, bofh)\n";
    s.out << "  /version      Show version info\n";
    s.out << "  /help         Show this help\n";
    s.out << "  exit, quit    Exit the REPL\n";
  } else {
    s.out << "Unknown command: /" << input.command << ". Type /help for options.\n";
  }
  return true;
}

/** Prompt user for write confirmation (ADR-014)
 * Supports "s"/"show" to preview content before deciding
 * Returns true if user confirms with y/yes */
static bool confirm_write(const WriteAction& action, std::istream& in, std::ostream& out) {
  out << "Write to " << action.path << "? [y/n/s] " << std::flush;
  std::string answer;
  if (!std::getline(in, answer)) {
    return false;
  }
  if (answer == "s" || answer == "show") {
    out << action.content << "\n";
    out << "Write to " << action.path << "? [y/n] " << std::flush;
    if (!std::getline(in, answer)) {
      return false;
    }
  }
  return answer == "y" || answer == "yes";
}

/** Process a single write action with user confirmation
 * Writes file on "y"/"yes", prints [skipped] otherwise
 * Reports errors if file cannot be opened for writing */
static void process_write(const WriteAction& action, std::istream& in, std::ostream& out, bool color) {
  tui::write_proposal(out, color, action.path);
  if (confirm_write(action, in, out)) {
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

/** Find all <exec>command</exec> annotations in text
 * Iterates through text finding exec blocks between open/close tags
 * Returns vector of command strings to be confirmed and executed */
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

// Strip <exec> annotations from text, replacing with readable summary
// Used to display clean response text to the user
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

/** Handle LLM response: check for write and exec annotations
 * If no annotations, just print. Otherwise strip tags and process each.
 * Returns true if exec output was added to history (needs LLM follow-up) */
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

// Read one line of input using linenoise (interactive) or getline (tests)
// Returns false on EOF (signals loop exit)
static bool read_line(std::istream& in, std::ostream& /*out*/, std::string& line, bool color) {
  if (&in == &std::cin) {
    std::string prompt_str = color ? "\033[1;32m> \033[0m" : "> ";
    auto quit = linenoise::Readline(prompt_str.c_str(), line);
    if (quit) return false;
    linenoise::AddHistory(line.c_str());
    return true;
  }
  return static_cast<bool>(std::getline(in, line));
}

// Execute a command and optionally add output to history
// Used by ! (add_to_history=false) and !! (add_to_history=true)
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
 * The abandoned thread will finish in the background. */
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

// Dispatch a single REPL input: command, exec, prompt, or exit
// Returns false if the loop should exit, true to continue
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

  // Send prompt to LLM and handle response
  // SIGINT handler installed so Ctrl+C aborts the blocking HTTP call
  s.history.push_back({"user", line});
  std::string response;
  {
    g_interrupted = 0;
    auto prev = std::signal(SIGINT, sigint_handler);
    Spinner spin(s.out, s.color, s.bofh ? tui::bofh_messages() : tui::default_messages());
    response = interruptible_chat(s);
    std::signal(SIGINT, prev);  // restore previous handler
  }
  // If user pressed Ctrl+C, discard the unanswered prompt
  if (g_interrupted) {
    s.out << "\n[interrupted]\n";
    s.history.pop_back();  // remove the unanswered user message
    g_interrupted = 0;
    return true;
  }
  s.history.push_back({"assistant", response});
  bool needs_followup = handle_response(response, s);
  s.count++;

  // If exec output was added, send follow-up so LLM can react
  if (needs_followup) {
    std::string followup;
    {
      g_interrupted = 0;
      auto prev = std::signal(SIGINT, sigint_handler);
      Spinner spin(s.out, s.color, s.bofh ? tui::bofh_messages() : tui::default_messages());
      followup = interruptible_chat(s);
      std::signal(SIGINT, prev);
    }
    if (!g_interrupted) {
      s.out << tui::render_markdown(followup, s.color && s.markdown) << "\n";
      s.history.push_back({"assistant", followup});
    } else {
      s.out << "\n[interrupted]\n";
      g_interrupted = 0;
    }
  }
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
  ReplState state = {chat, cfg, history, in, out, 0, tui::use_color(cfg.no_color), true, cfg.bofh};

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
