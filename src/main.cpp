/**
 * @file main.cpp
 * @brief Entry point: loads config, dispatches to sync or interactive mode.
 * @see docs/adr/adr-005-execution-modes.md
 * @see docs/adr/adr-007-cli-interface.md
 */

#include <dirent.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "config/config.h"
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
// pmccabe:skip-complexity — TODO: refactor main dispatch logic
// NOLINTNEXTLINE(readability-function-size)
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

  if (cfg.mode == Mode::Sync) {
    // Sync mode: one-shot, response to stdout (ADR-007)
    if (cfg.provider == "mock") {
      std::cout << "mock response: " << cfg.prompt << "\n";
    } else {
      tui::system_msg(std::cerr, color, "Connecting to " + cfg.host + ":" + cfg.port + " with model " + cfg.model + "...");
      std::vector<Message> messages;
      if (!cfg.system_prompt.empty()) {
        messages.push_back({"system", cfg.system_prompt});
      }
      messages.push_back({"user", cfg.prompt});
      std::string response = ollama_chat_stream(cfg, messages, [](const std::string& token) {
        std::cout << token << std::flush;
        return true;
      });
      if (!response.empty()) {
        std::cout << "\n";
      }
    }
  } else {
    // Interactive mode: REPL loop (ADR-012)
    if (!cfg.no_banner) {
      tui::banner(std::cerr, color);
    }
    tui::system_msg(std::cerr, color, "llama-cli — connected to " + cfg.host + ":" + cfg.port + " (" + cfg.model + ")");
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
