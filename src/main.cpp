/**
 * @file main.cpp
 * @brief Entry point: loads config, dispatches to sync or interactive mode.
 * @see docs/adr/adr-005-execution-modes.md
 * @see docs/adr/adr-007-cli-interface.md
 */

#include <dirent.h>
#include <unistd.h>

#include <csignal>

extern volatile sig_atomic_t g_interrupted;

#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>

#include "annotation/annotation.h"
#include "config/config.h"
#include "exec/exec.h"
#include "help.h"
#include "json/json.h"
#include "ollama/ollama.h"
#include "repl/repl.h"
#include "session/session.h"
#include "sync/sync.h"
#include "tui/tui.h"

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
        followup += "[error: write blocked — outside sandbox: " + action.path + "]\n";
        std::cerr << "[blocked: write outside sandbox: " << action.path << "]\n";
        continue;
      }
      std::ofstream f(action.path);
      if (f) {
        f << action.content;
        std::cerr << "[wrote " << action.path << "]\n";
      } else {
        followup += "[error: could not write " + action.path + "]\n";
      }
    }
    auto replaces = parse_str_replace_annotations(response);
    for (const auto& action : replaces) {
      if (!path_allowed(action.path, cfg.sandbox)) {
        followup += "[error: write blocked — outside sandbox: " + action.path + "]\n";
        std::cerr << "[blocked: write outside sandbox: " << action.path << "]\n";
        continue;
      }
      std::ifstream rf(action.path);
      if (!rf) {
        followup += "[error: file not found: " + action.path + "]\n";
        continue;
      }
      std::string content((std::istreambuf_iterator<char>(rf)), std::istreambuf_iterator<char>());
      rf.close();
      size_t pos = content.find(action.old_str);
      if (pos == std::string::npos) {
        followup += "[error: old text not found in " + action.path + "]\n";
        continue;
      }
      content.replace(pos, action.old_str.size(), action.new_str);
      std::ofstream wf(action.path);
      if (wf) {
        wf << content;
        std::cerr << "[str_replace " << action.path << "]\n";
      }
    }

    // Process <add_line> annotations — insert a line at a specific position
    auto adds = parse_add_line_annotations(response);
    for (const auto& action : adds) {
      if (!path_allowed(action.path, cfg.sandbox)) {
        followup += "[error: write blocked — outside sandbox: " + action.path + "]\n";
        std::cerr << "[blocked: write outside sandbox: " << action.path << "]\n";
        continue;
      }
      std::ifstream rf(action.path);
      if (!rf) {
        followup += "[error: file not found: " + action.path + "]\n";
        continue;
      }
      std::vector<std::string> lines;
      std::string line;
      while (std::getline(rf, line)) lines.push_back(line);
      rf.close();
      int idx = action.line_number - 1;
      if (idx < 0) idx = 0;
      if (idx > static_cast<int>(lines.size())) idx = static_cast<int>(lines.size());
      lines.insert(lines.begin() + idx, action.content);
      std::ofstream wf(action.path);
      if (wf) {
        for (const auto& l : lines) wf << l << "\n";
        std::cerr << "[add_line " << action.path << " at " << action.line_number << "]\n";
      }
    }

    // Process <delete_line> annotations — remove a line matching content
    auto deletes = parse_delete_line_annotations(response);
    for (const auto& action : deletes) {
      if (!path_allowed(action.path, cfg.sandbox)) {
        followup += "[error: write blocked — outside sandbox: " + action.path + "]\n";
        std::cerr << "[blocked: write outside sandbox: " << action.path << "]\n";
        continue;
      }
      std::ifstream rf(action.path);
      if (!rf) {
        followup += "[error: file not found: " + action.path + "]\n";
        continue;
      }
      std::vector<std::string> lines;
      std::string line;
      while (std::getline(rf, line)) lines.push_back(line);
      rf.close();
      bool found = false;
      for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (*it == action.content) {
          lines.erase(it);
          found = true;
          break;
        }
      }
      if (found) {
        std::ofstream wf(action.path);
        if (wf) {
          for (const auto& l : lines) wf << l << "\n";
          std::cerr << "[delete_line " << action.path << "]\n";
        }
      } else {
        followup += "[error: line not found in " + action.path + ": " + action.content + "]\n";
      }
    }
  }

  return followup;
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

  // Auto-detect model for sync mode (skip for mock provider)
  if (cfg.model == "auto" && cfg.mode == Mode::Sync && cfg.provider != "mock") {
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
        const char* mock_response_env = std::getenv("LLAMA_CLI_MOCK_RESPONSE");
        if (mock_response_env) {
          return mock_response_env;
        }
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
        if (followup.empty()) {
          break;
        }
        messages.push_back({"user", followup});
        response = generate_response(messages);
        if (response.empty()) {
          break;
        }
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
    if (cfg.model == "auto" && cfg.provider != "mock") {
      auto models = get_available_models(cfg);
      auto infos = get_model_info(cfg);
      std::map<std::string, ModelInfo> info_map;
      for (const auto& info : infos) info_map[info.name] = info;

      // Filter and sort potential models: Sweetspot (11B-28B) first, then others.
      struct Candidate {
        std::string name;
        double params;
        bool sweet;
      };
      std::vector<Candidate> candidates;

      for (const auto& m : models) {
        double p = 0;
        try {
          p = std::stod(info_map[m].params);
        } catch (...) {
        }
        candidates.push_back({m, p, (p >= 11.0 && p <= 28.0)});
      }

      std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        // Prioritize models in sweetspot range (11B-28B)
        if (a.sweet != b.sweet) return a.sweet > b.sweet;
        // Then sort by params ascending (pick the smallest/lightest in the sweetspot)
        return a.params < b.params;
      });

      if (!candidates.empty()) {
        cfg.model = candidates[0].name;
        Config::instance().model = candidates[0].name;
        HardwareInfo hw = detect_hardware();
        tui::system_msg(std::cerr, color,
                        "Detected hardware: " + hw.cpu + " | " + std::to_string(hw.ram_gb) + "GB RAM | " + std::to_string(hw.vram_gb) +
                            "GB VRAM estimated.");
        tui::system_msg(std::cerr, color, "Auto-selected " + cfg.model + " as the optimal sweetspot model for your hardware.");
      } else {
        tui::error(std::cerr, color, "No models found. Run: ollama pull qwen3.6:27b");
        return 1;
      }
    }

    if (!cfg.no_banner) {
      tui::banner(std::cerr, color);
    }

    // Optional model warmup (only if not already running)
    if (Config::instance().warmup && !is_model_running(cfg, Config::instance().model)) {
      tui::system_msg(std::cerr, color, "Warming up " + Config::instance().model + "... (Ctrl+C to skip)");
      // Simple spinner loop
      const char* spinner = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏";
      int frame = 0;
      std::vector<Message> warmup_msg = {{"user", "hi"}};

      // We need to run this in a way that allows us to check progress
      // ollama_chat_stream doesn't expose the http object for cancellation easily,
      // but we can at least show a spinner.
      ollama_chat_stream(cfg, warmup_msg, [&](const std::string&) {
        if (g_interrupted) return false;
        std::cerr << "\r" << spinner[frame % 10] << " loading..." << std::flush;
        frame++;
        return true;
      });
      std::cerr << "\r" << "   " << std::flush;  // Clear spinner
    }

    tui::system_msg(std::cerr, color, "llama-cli — connected to " + cfg.host + ":" + cfg.port + " (" + Config::instance().model + ")");
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
    auto generate = [&cfg](const std::vector<Message>& msgs) -> std::string {
      if (cfg.provider == "mock") {
        const char* mock_response_env = std::getenv("LLAMA_CLI_MOCK_RESPONSE");
        if (mock_response_env) {
          return std::string(mock_response_env);
        }
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
