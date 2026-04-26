/**
 * @file main.cpp
 * @brief Entry point: loads config, dispatches to sync or interactive mode.
 * @see docs/adr/adr-005-execution-modes.md
 * @see docs/adr/adr-007-cli-interface.md
 */

#include <dirent.h>
#include <unistd.h>

#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "annotation/annotation.h"
#include "config/config.h"
#include "exec/exec.h"
#include "help.h"
#include "json/json.h"
#include "ollama/ollama.h"
#include "repl/repl.h"
#include "tui/tui.h"

/// Extract "file://path" entries from a JSON resources array.
/// Minimal parser — just finds "file://" strings between quotes.
static std::vector<std::string> extract_resources(const std::string& json) {
  std::vector<std::string> paths;
  const std::string prefix = "\"file://";
  size_t pos = 0;
  while ((pos = json.find(prefix, pos)) != std::string::npos) {
    size_t start = pos + prefix.size();
    size_t end = json.find('"', start);
    if (end != std::string::npos) {
      paths.push_back(json.substr(start, end - start));
    }
    pos = (end != std::string::npos) ? end + 1 : start;
  }
  return paths;
}

/// Load .kiro/agents/*.json: append agent prompt to system prompt,
/// read resource files into initial context string.
static std::string load_kiro_context(Config& cfg, bool color) {
  DIR* dir = opendir(".kiro/agents");
  if (!dir) {
    return "";
  }
  std::string context;
  const struct dirent* entry = nullptr;
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  while ((entry = readdir(dir)) != nullptr) {
    std::string name(entry->d_name);
    if (name.size() < 6 || name.substr(name.size() - 5) != ".json") {
      continue;
    }
    std::ifstream f(".kiro/agents/" + name);
    if (!f) {
      continue;
    }
    std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    // Append agent prompt to system prompt
    std::string prompt = json_extract_string(json, "prompt");
    if (!prompt.empty()) {
      cfg.system_prompt += "\n\n" + prompt;
    }
    // Read resource files into context
    for (const auto& path : extract_resources(json)) {
      std::ifstream rf(path);
      if (!rf) {
        continue;
      }
      context += "--- " + path + " ---\n";
      context += std::string((std::istreambuf_iterator<char>(rf)), std::istreambuf_iterator<char>());
      context += "\n";
    }
    tui::system_msg(std::cerr, color, "[loaded .kiro/agents/" + name + "]\n");
  }
  closedir(dir);
  return context;
}

/**
 * @brief Program entry point that loads configuration and runs either a one-shot sync request or an interactive REPL.
 *
 * If the `--help` argument is present, prints the CLI help and exits immediately. Otherwise the function
 * loads configuration (defaults → environment → CLI), determines UI color, and dispatches to either
 * sync mode (produce a single response) or interactive mode (REPL).
 *
 * @return int Exit code: `0` on normal completion.
 */
/// Check if a capability is enabled in the comma-separated capabilities string
static bool has_cap(const std::string& caps, const std::string& cap) {
  // Search for cap as a whole word within comma-separated list
  size_t pos = 0;
  while ((pos = caps.find(cap, pos)) != std::string::npos) {
    bool at_start = (pos == 0 || caps[pos - 1] == ',');
    bool at_end = (pos + cap.size() >= caps.size() || caps[pos + cap.size()] == ',');
    if (at_start && at_end) return true;
    pos += cap.size();
  }
  return false;
}

/// Commands safe to auto-execute with the "read" capability.
/// These cannot modify files, network, or system state.
static const std::vector<std::string> READ_ONLY_COMMANDS = {"cat",  "ls",   "find", "grep",     "head",    "tail",    "wc",   "stat",
                                                            "tree", "du",   "df",   "file",     "diff",    "sort",    "uniq", "awk",
                                                            "sed",  "less", "more", "realpath", "dirname", "basename"};

/// Check if a command is read-only (first word in allowlist, no redirects)
static bool is_read_only(const std::string& cmd) {
  // Block redirects and destructive operators
  if (cmd.find('>') != std::string::npos) return false;
  // Extract first word from each pipe segment
  std::istringstream segments(cmd);
  std::string segment;
  while (std::getline(segments, segment, '|')) {
    // Trim leading whitespace
    size_t start = segment.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    // Extract first word (the command)
    size_t end = segment.find_first_of(" \t", start);
    std::string word = segment.substr(start, end - start);
    bool found = false;
    for (const auto& safe : READ_ONLY_COMMANDS) {
      if (word == safe) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  return true;
}

/// Parse <exec>cmd</exec> tags from LLM response text
static std::vector<std::string> parse_exec_tags(const std::string& text) {
  std::vector<std::string> cmds;
  const std::string open = "<exec>";
  const std::string close = "</exec>";
  size_t pos = 0;
  while ((pos = text.find(open, pos)) != std::string::npos) {
    size_t start = pos + open.size();
    size_t end = text.find(close, start);
    if (end == std::string::npos) break;
    cmds.push_back(text.substr(start, end - start));
    pos = end + close.size();
  }
  return cmds;
}

/// Check if a file path is within the sandbox directory (ADR-056).
/// Resolves both paths to absolute form to catch ".." traversal.
/// For non-existent files (writes), checks the parent directory.
static bool path_allowed(const std::string& path, const std::string& sandbox) {
  // Resolve sandbox to absolute path
  char sandbox_real[PATH_MAX];
  if (!realpath(sandbox.c_str(), sandbox_real)) return false;
  std::string sandbox_prefix(sandbox_real);
  sandbox_prefix += "/";

  // For existing files, resolve directly
  char path_real[PATH_MAX];
  if (realpath(path.c_str(), path_real)) {
    std::string resolved(path_real);
    // Allow exact match (sandbox dir itself) or prefix match
    return resolved == sandbox_real || resolved.rfind(sandbox_prefix, 0) == 0;
  }

  // For new files, resolve the parent directory
  size_t slash = path.rfind('/');
  std::string parent = (slash != std::string::npos) ? path.substr(0, slash) : ".";
  if (!realpath(parent.c_str(), path_real)) return false;
  std::string resolved(path_real);
  return resolved == sandbox_real || resolved.rfind(sandbox_prefix, 0) == 0;
}

/// Process annotations in sync mode response based on capabilities (ADR-056).
/// Returns context to feed back to the model (read/exec output).
// pmccabe:skip-complexity — handles all annotation types in one pass
static std::string process_sync_annotations(const std::string& response, const Config& cfg) {
  std::string caps = cfg.capabilities;
  std::string followup;

  // Process <read> annotations (requires "read" capability)
  if (has_cap(caps, "read")) {
    auto reads = parse_read_annotations(response);
    for (const auto& action : reads) {
      if (!path_allowed(action.path, cfg.sandbox)) {
        std::cerr << "[blocked: read outside sandbox: " << action.path << "]\n";
        continue;
      }
      std::ifstream f(action.path);
      if (!f) {
        std::cerr << "[read: file not found: " << action.path << "]\n";
        continue;
      }
      std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
      followup += "[file: " + action.path + "]\n" + content + "\n";
      std::cerr << "[read " << action.path << "]\n";
    }
  }

  // Process <exec> annotations (requires "read" for safe cmds, "exec" for all)
  auto execs = parse_exec_tags(response);
  for (const auto& cmd : execs) {
    bool allowed = has_cap(caps, "exec") || (has_cap(caps, "read") && is_read_only(cmd));
    if (!allowed) {
      std::cerr << "[blocked: " << cmd << "]\n";
      continue;
    }
    auto r = cmd_exec(cmd, cfg.exec_timeout, cfg.max_output);
    followup += "[command: " + cmd + "]\n" + r.output + "\n";
    std::cerr << "[exec " << cmd << "]\n";
  }

  // Process <write> and <str_replace> annotations (requires "write" capability)
  if (has_cap(caps, "write")) {
    auto writes = parse_write_annotations(response);
    for (const auto& action : writes) {
      if (!path_allowed(action.path, cfg.sandbox)) {
        std::cerr << "[blocked: write outside sandbox: " << action.path << "]\n";
        continue;
      }
      std::ofstream f(action.path);
      if (f) {
        f << action.content;
        std::cerr << "[wrote " << action.path << "]\n";
      }
    }
    auto replaces = parse_str_replace_annotations(response);
    for (const auto& action : replaces) {
      if (!path_allowed(action.path, cfg.sandbox)) {
        std::cerr << "[blocked: write outside sandbox: " << action.path << "]\n";
        continue;
      }
      std::ifstream rf(action.path);
      if (!rf) continue;
      std::string content((std::istreambuf_iterator<char>(rf)), std::istreambuf_iterator<char>());
      rf.close();
      size_t pos = content.find(action.old_str);
      if (pos == std::string::npos) continue;
      content.replace(pos, action.old_str.size(), action.new_str);
      std::ofstream wf(action.path);
      if (wf) {
        wf << content;
        std::cerr << "[str_replace " << action.path << "]\n";
      }
    }
  }

  return followup;
}

/// Escape a string for JSON output (quotes, backslashes, control chars)
static std::string escape_json(const std::string& s) {
  std::string out;
  for (char c : s) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += c;
    }
  }
  return out;
}

/// Unescape JSON string escape sequences (\\n, \\t, \\", \\\\, etc.)
static std::string unescape_json(const std::string& s) {
  std::string out;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      switch (s[++i]) {
        case '"':
          out += '"';
          break;
        case '\\':
          out += '\\';
          break;
        case 'n':
          out += '\n';
          break;
        case 'r':
          out += '\r';
          break;
        case 't':
          out += '\t';
          break;
        default:
          out += '\\';
          out += s[i];
      }
    } else {
      out += s[i];
    }
  }
  return out;
}

/// Load conversation history from a session JSON file (ADR-056).
/// Returns empty vector if file doesn't exist yet.
static std::vector<Message> load_session(const std::string& path) {
  std::vector<Message> msgs;
  std::ifstream f(path);
  if (!f) {
    return msgs;  // new session — file will be created on save
  }
  std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  // Minimal parser: find each {"role":"...","content":"..."} object
  size_t pos = 0;
  while ((pos = json.find("\"role\"", pos)) != std::string::npos) {
    std::string role = json_extract_string(json.substr(pos - 1), "role");
    std::string content = json_extract_string(json.substr(pos - 1), "content");
    if (!role.empty()) {
      msgs.push_back({role, unescape_json(content)});
    }
    pos += 6;  // advance past "role"
  }
  return msgs;
}

/// Save conversation history to a session JSON file (ADR-056)
static void save_session(const std::string& path, const std::vector<Message>& msgs) {
  std::ofstream f(path);
  f << "[\n";
  for (size_t i = 0; i < msgs.size(); i++) {
    f << "  {\"role\":\"" << msgs[i].role << "\",\"content\":\"" << escape_json(msgs[i].content) << "\"}";
    if (i + 1 < msgs.size()) f << ",";
    f << "\n";
  }
  f << "]\n";
}

// pmccabe:skip-complexity — TODO: refactor main dispatch logic
// NOLINTNEXTLINE(readability-function-size)
/** @brief Entry point: loads config, dispatches to sync or interactive mode. */
int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--help") {
      std::cout << help::cli();
      return 0;
    }
    if (std::string(argv[i]) == "--version") {
      std::cout << "llama-cli " << LLAMA_CLI_VERSION << " (built " << __DATE__ << " " << __TIME__ << " " << BUILD_TIMEZONE << ")\n";
      return 0;
    }
    if (std::string(argv[i]) == "--default-env") {
      print_default_env();
      return 0;
    }
  }

  // Load config: defaults -> env vars -> CLI args (ADR-004)
  Config cfg = load_config(argc, const_cast<const char* const*>(argv));
  bool color = tui::use_color(cfg.no_color);

  // Build context from --files or stdin pipe (ADR-030)
  std::string context;
  if (!cfg.files.empty()) {
    for (const auto& path : cfg.files) {
      std::ifstream f(path);
      if (!f) {
        std::cerr << "error: cannot read " << path << "\n";
        return 1;
      }
      context += "--- " + path + " ---\n";
      context += std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
      context += "\n";
    }
  } else if (!isatty(STDIN_FILENO) && !cfg.force_repl) {
    context = std::string((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
  }
  if (!context.empty()) {
    cfg.prompt = cfg.prompt.empty() ? context : cfg.prompt + "\n\n" + context;
    cfg.mode = Mode::Sync;
  }

  // Auto-detect model for sync mode
  if (cfg.model == "auto" && cfg.mode == Mode::Sync) {
    auto models = get_available_models(cfg);
    if (!models.empty()) {
      cfg.model = models[0];
      Config::instance().model = models[0];
    } else {
      tui::error(std::cerr, color, "No models found. Run: ollama pull qwen3:30b");
      return 1;
    }
  }

  if (cfg.mode == Mode::Sync) {
    // Sync mode: one-shot, response to stdout (ADR-007)
    // Mock provider goes through the same flow — echoes prompt as response
    // so annotation processing and session work identically in tests.
    auto generate_response = [&cfg, color](const std::vector<Message>& msgs) -> std::string {
      if (cfg.provider == "mock") {
        return "mock response: " + msgs.back().content;
      }
      tui::system_msg(std::cerr, color, "Connecting to " + cfg.host + ":" + cfg.port + " with model " + cfg.model + "...");
      return ollama_chat_stream(cfg, msgs, [](const std::string& token) {
        std::cout << token << std::flush;
        return true;
      });
    };

    // Load session history if --session is set (ADR-056)
    std::vector<Message> messages = cfg.session_path.empty() ? std::vector<Message>{} : load_session(cfg.session_path);
    // Add system prompt only if starting a new conversation
    if (messages.empty() && !cfg.system_prompt.empty()) {
      messages.push_back({"system", cfg.system_prompt});
    }
    messages.push_back({"user", cfg.prompt});
    std::string response = generate_response(messages);
    if (!response.empty()) {
      std::cout << response << "\n";
      messages.push_back({"assistant", response});
      // Process annotations if capabilities are set (ADR-056)
      int max_rounds = 10;
      while (!cfg.capabilities.empty() && max_rounds-- > 0) {
        std::string followup = process_sync_annotations(fix_malformed_tags(response), cfg);
        if (followup.empty()) break;
        messages.push_back({"user", followup});
        response = generate_response(messages);
        if (response.empty()) break;
        std::cout << response << "\n";
        messages.push_back({"assistant", response});
      }
      // Save updated history if --session is set (ADR-056)
      if (!cfg.session_path.empty()) {
        save_session(cfg.session_path, messages);
      }
    }
  } else {
    // Interactive mode: REPL loop (ADR-012)
    // Auto-detect model from server when set to "auto"
    if (cfg.model == "auto") {
      auto models = get_available_models(cfg);
      if (!models.empty()) {
        cfg.model = models[0];
        Config::instance().model = models[0];
      } else {
        tui::error(std::cerr, color, "No models found on " + cfg.host + ":" + cfg.port + ". Run: ollama pull qwen3:30b");
        return 1;
      }
    }
    // Warm up the model — Ollama loads it into memory on first request.
    // Without this, the first user prompt often fails with an HTTP error.
    {
      tui::system_msg(std::cerr, color, "Loading " + cfg.model + "...");
      auto warmup_result = ollama_generate(cfg, "hi");
      if (warmup_result.empty()) {
        tui::error(std::cerr, color, "Warning: model warmup failed — first prompt may be slow");
      }
    }
    if (!cfg.no_banner) {
      tui::banner(std::cerr, color);
    }
    tui::system_msg(std::cerr, color, "llama-cli — connected to " + cfg.host + ":" + cfg.port + " (" + cfg.model + ")");
    tui::system_msg(std::cerr, color, "Type /model to switch, /help for commands.");
    if (cfg.provider == "mock") {
      tui::system_msg(std::cerr, color, "[MOCK MODE] All prompts will be echoed back.\n");
    }
    tui::system_msg(std::cerr, color, "Type your prompt. 'exit' or Ctrl+D to quit.\n");
    // Load .kiro/agents/ context if present in cwd
    std::string kiro_ctx = load_kiro_context(cfg, color);
    if (!kiro_ctx.empty()) {
      cfg.system_prompt += "\n\n## Project context\n" + kiro_ctx;
    }
    auto generate = [&cfg](const std::vector<Message>& msgs) {
      if (cfg.provider == "mock") {
        return "mock response: " + msgs.back().content;
      }
      return ollama_chat(Config::instance(), msgs);
    };
    auto stream = [&cfg](const std::vector<Message>& msgs, StreamCallback on_token) {
      if (cfg.provider == "mock") {
        // Simulate streaming by sending the response through the callback
        std::string response = "mock response: " + msgs.back().content;
        on_token(response);
        return response;
      }
      return ollama_chat_stream(Config::instance(), msgs, on_token);
    };
    run_repl(generate, cfg, std::cin, std::cout, get_available_models, stream);
  }
  return 0;
}
