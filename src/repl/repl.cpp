/**
 * @file repl.cpp
 * @brief Interactive REPL loop with conversation memory and commands.
 * @see docs/adr/adr-012-interactive-repl.md
 * @see docs/adr/adr-014-tool-annotations.md
 */

#include "repl/repl.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <fstream>
#include <sstream>
#include <vector>

#include "command/command.h"
#include "exec/exec.h"
#include "logging/logger.h"
#include "orchestrator/subagent.h"
#include "repl/repl_chat.h"
#include "repl/repl_commands.h"
#include "repl/repl_search.h"
#include "session/session.h"
#include "trace/trace.h"
#include "tui/tui.h"
#include "util/util.h"

#ifdef LINENOISE_HPP
// already included
#else
#include "linenoise.hpp"
#endif

/// @cond
#ifndef BUILD_TIMEZONE
#define BUILD_TIMEZONE "UTC"
#endif
/// @endcond

/// Global flag set by SIGINT handler to interrupt LLM calls
volatile sig_atomic_t g_interrupted = 0;

/** SIGINT handler — sets flag so spinner/chat can check and abort */
void sigint_handler(int /*sig*/) { g_interrupted = 1; }

// SRP: ReplState is defined in repl_commands.h (shared with repl_commands.cpp)
// SRP: Annotation handlers (process_write, process_read, confirm_exec, etc.)
//      are in repl_annotations.cpp (ADR-065 split).
// SRP: Table rendering (is_table_row, render_table, visible_length)
//      is in tui/table.cpp (ADR-065 split).

/// Read entire file into a string. Returns empty on failure.
/// Used for loading preferences and memory files (ADR-059).
static std::string read_file(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) {
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
// Wrap rendered AI text with the configured AI color.
// Re-applies AI color after any ANSI reset inside markdown rendering.
/// Build the REPL prompt label based on nick, provider, and model.
/// Build the REPL prompt label based on nick, provider, and model.
/// Model is shown for all providers with selectable models.
/// Only tgpt is excluded (single fixed model, no choice).
/// Format: "nick@provider:model> " or "nick@provider> " or "> "
/// Examples:
///   gius\@ollama:gemma4:26b>     (local model visible)
///   gius\@kiro-cli:claude-sonnet> (cloud model visible)
///   gius\@tgpt>                   (no model — tgpt has only one)
std::string build_prompt_label(const std::string& nick, const std::string& provider, const std::string& model) {
  // tgpt has no model selection — skip model in prompt.
  // All other providers (ollama, gemini, kiro-cli, amazon-q) show the model.
  bool show_model = (provider != "tgpt") && !model.empty();
  if (!nick.empty() && !provider.empty() && show_model) {
    return nick + "@" + provider + ":" + model + "> ";
  }
  if (!nick.empty() && !provider.empty()) {
    return nick + "@" + provider + "> ";
  }
  if (!nick.empty()) {
    return nick + "> ";
  }
  return "> ";
}

/// Read one line of input using linenoise (interactive) or getline (tests).
static bool read_line(std::istream& in, std::ostream& /*out*/, std::string& line, bool color, const std::string& /*prompt_ansi*/ = "",
                      const std::string& nick = "", const std::string& model = "", const std::string& provider = "") {
  if (&in != &std::cin) {
    return static_cast<bool>(std::getline(in, line));
  }
  if (!isatty(STDIN_FILENO)) {
    return static_cast<bool>(std::getline(in, line));
  }
  // Build prompt: "nick@provider:model> " or "nick@provider> " or "nick> " or "> "
  std::string label = build_prompt_label(nick, provider, model);
  std::string prompt_str = color ? tui::active_theme().prompt.ansi() + label + ThemeStyle::reset() : label;
  // NOLINTNEXTLINE(readability-identifier-naming)
  auto quit = linenoise::Readline(prompt_str.c_str(), line);
  if (quit) {
    return false;
  }
  linenoise::AddHistory(line.c_str());
  return true;
}

/// Execute a shell command, print output, optionally add to history.
static void run_exec(const std::string& cmd, bool add_to_history, ReplState& s) {
  if (s.cfg.trace) {
    stderr_trace->log("[TRACE] exec: %s\n", cmd.c_str());
  }
  auto t0 = std::chrono::steady_clock::now();
  auto r = cmd_exec(cmd, s.cfg.exec_timeout, s.cfg.max_output);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
  if (s.cfg.trace) {
    stderr_trace->log("[TRACE] exec: exit=%d %ldms output=%zu bytes\n", r.exit_code, ms, r.output.size());
  }
  LOG_EVENT("repl", add_to_history ? "exec_context" : "exec", cmd, r.output, static_cast<int>(ms), 0, 0);
  tui::cmd_output(s.out, s.color, r.output);
  if (r.exit_code != 0 && !r.timed_out) {
    tui::error(s.out, s.color, "[exit code: " + std::to_string(r.exit_code) + "]");
  }
  if (add_to_history) {
    s.history.push_back({"user", "[command: " + cmd + "]\n" + r.output});
  }
}

static bool dispatch(const std::string& line, ReplState& s) {
  auto input = parse_input(line);
  if (input.type == InputType::Exit) {
    return false;
  }
  if (input.type == InputType::Command) {
    dispatch_command(input.command, input.arg, s);
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
  // @mention routing: @q, @opencode, @explore, etc. (ADR-096 Phase 4)
  auto mention = parse_mention(line);
  if (mention.has_mention) {
    tui::system_msg(s.out, s.color, "[routing to @" + mention.agent + "]");
    std::string response = invoke_subagent(mention.agent, mention.prompt, s.history);
    s.out << "\n" << response << "\n";
    s.history.push_back({"user", mention.prompt});
    s.history.push_back({"assistant", response});
    return true;
  }
  send_prompt(line, s);
  return true;
}

/** Main REPL loop: read → parse → dispatch → respond
 * Returns number of LLM prompts processed (excludes ! and !! commands) */
/// List files/dirs matching a path prefix for tab completion
static void path_completions(const std::string& prefix, const std::string& before, std::vector<std::string>& completions) {
  // Split into directory and partial filename
  std::string dir_part, file_part;
  auto slash = prefix.rfind('/');
  if (slash != std::string::npos) {
    dir_part = prefix.substr(0, slash + 1);
    file_part = prefix.substr(slash + 1);
  } else {
    dir_part = ".";
    file_part = prefix;
  }

  // Expand ~ to HOME
  std::string resolved_dir = dir_part;
  if (!resolved_dir.empty() && resolved_dir[0] == '~') {
    const char* home = getenv("HOME");
    if (home) {
      resolved_dir = std::string(home) + resolved_dir.substr(1);
    }
  }

  DIR* d = opendir(resolved_dir.c_str());
  if (!d) {
    return;
  }
  const struct dirent* entry;
  while ((entry = readdir(d)) != nullptr) {
    std::string name(entry->d_name);
    if (name == "." || name == "..") {
      continue;
    }
    if (name.compare(0, file_part.size(), file_part) != 0) {
      continue;
    }
    std::string full = dir_part + name;
    // Append / for directories
    if (entry->d_type == DT_DIR) {
      full += "/";
    }
    completions.push_back(before + full);
  }
  closedir(d);
}

// Tab-completion for slash commands and file paths
static void slash_completion(const char* buf, std::vector<std::string>& completions) {
  static const std::vector<std::string> cmds = {"/c",     "/clear", "/color", "/copy", "/mem", "/model",   "/p",
                                                "/paste", "/pref",  "/rate",  "/scan", "/set", "/version", "/help"};
  std::string input(buf);
  if (input.empty()) {
    return;
  }

  // Slash commands
  if (input[0] == '/') {
    for (const auto& cmd : cmds) {
      if (cmd.compare(0, input.size(), input) == 0) {
        completions.push_back(cmd);
      }
    }
    return;
  }

  // Path completion for ! and !! commands
  std::string before, path_prefix;
  if (input.compare(0, 2, "!!") == 0) {
    before = "!!";
    path_prefix = input.substr(2);
  } else if (input[0] == '!') {
    before = "!";
    path_prefix = input.substr(1);
  } else {
    // Also complete paths that start with ./ ~/ or /
    auto last_space = input.rfind(' ');
    if (last_space != std::string::npos) {
      std::string word = input.substr(last_space + 1);
      if (!word.empty() && (word[0] == '.' || word[0] == '~' || word[0] == '/')) {
        before = input.substr(0, last_space + 1);
        path_prefix = word;
      }
    } else if (input[0] == '.' || input[0] == '~' || input[0] == '/') {
      path_prefix = input;
    }
  }

  if (!path_prefix.empty()) {
    path_completions(path_prefix, before, completions);
  }
}

/** Main REPL loop: read input, dispatch commands/prompts, return prompt count. */
int run_repl(ChatFn chat, const Config& cfg, std::istream& in, std::ostream& out, ModelsFn models_fn, StreamChatFn stream_chat,
             HardwareFn hw_fn, ModelInfoFn model_info_fn, ScanFn scan_fn, SwitchProviderFn switch_provider_fn, ModelRegistry* registry) {
  std::string line;
  std::vector<Message> history;
  if (!cfg.system_prompt.empty()) {
    std::string sys = cfg.system_prompt;
    // Use small prompt for models ≤7B (they can't handle complex tool instructions)
    if (!cfg.system_prompt_override) {
      std::string model_lower = cfg.model;
      // Quick heuristic: check for known small model patterns
      bool is_small = (model_lower.find(":1b") != std::string::npos || model_lower.find(":3b") != std::string::npos ||
                       model_lower.find(":4b") != std::string::npos || model_lower.find(":7b") != std::string::npos ||
                       model_lower.find(":1B") != std::string::npos || model_lower.find(":3B") != std::string::npos ||
                       model_lower.find(":4B") != std::string::npos || model_lower.find(":7B") != std::string::npos);
      if (is_small) {
        sys = SMALL_SYSTEM_PROMPT;
      }
    }
    // Append web search tool description when enabled (ADR-057)
    if (cfg.allow_web_search) {
      sys += Config::web_search_prompt;
    }
    // Load persistent preferences and memory into system prompt (ADR-059)
    std::string prefs = read_file(cfg.preferences_path);
    if (!prefs.empty()) {
      sys += "\n\nUser preferences:\n" + prefs;
    }
    std::string mem = read_file(cfg.memory_path);
    if (!mem.empty()) {
      sys += "\n\nUser context (remembered facts):\n" + mem;
    }
    history.push_back({"system", sys});
  }
  bool is_tty = tui::use_color(cfg.no_color);
  linenoise::SetCompletionCallback(slash_completion);
  // Enable multiline mode so long prompts wrap instead of scrolling horizontally
  linenoise::SetMultiLine(true);

  // Persist prompt history across sessions — up-arrow recalls previous prompts.
  // Dev mode: .tmp/history.txt, installed: ~/.llama-cli/history.txt
  std::string history_path;
  {
    struct stat st;
    if (stat("Makefile", &st) == 0) {
      history_path = ".tmp/history.txt";
    } else {
      const char* home = getenv("HOME");
      history_path = std::string(home ? home : ".") + "/.llama-cli/history.txt";
    }
  }
  linenoise::SetHistoryMaxLen(500);
  linenoise::LoadHistory(history_path.c_str());

  ReplState state = {chat,
                     stream_chat,
                     models_fn,
                     model_info_fn,
                     hw_fn,
                     scan_fn,
                     cfg,
                     history,
                     in,
                     out,
                     0,
                     is_tty,
                     is_tty,
                     true,
                     cfg.bofh,
                     cfg.warmup,
                     color_name_to_ansi(cfg.prompt_color),
                     color_name_to_ansi(cfg.ai_color),
                     false,
                     -1,
                     static_cast<ModelRegistry*>(nullptr),
                     {},
                     nullptr};

  // Log session start with version, commit, model, and host for traceability
  std::string version_info = std::string(LLAMA_CLI_VERSION) + " (" + GIT_COMMIT + ")";
  LOG_EVENT("repl", "session_start", cfg.model, version_info + " " + cfg.host + ":" + cfg.port, 0, 0, 0);

  // Wire provider switching callback (ADR-020)
  state.switch_provider = switch_provider_fn;
  state.registry = registry;

  // Auto-start SearXNG when web search is enabled (ADR-057)
  if (cfg.allow_web_search) {
    ensure_searxng(cfg, out);
  }

  while (read_line(in, out, line, state.color, state.prompt_color, Config::instance().nick, Config::instance().model,
                   Config::instance().provider)) {
    if (line.empty()) {
      continue;
    }
    if (!dispatch(line, state)) {
      break;
    }
  }
  // Log session end with total prompt count for usage analysis
  LOG_EVENT("repl", "session_end", std::to_string(state.count) + " prompts", "", 0, 0, 0);
  // Save prompt history so next session can recall with up-arrow
  linenoise::SaveHistory(history_path.c_str());
  return state.count;
}
