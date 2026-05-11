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
#include "logging/logger.h"
#include "ollama/ollama.h"
#include "provider/provider_factory.h"
#include "provider/registry.h"
#include "repl/repl.h"
#include "session/session.h"
#include "sync/sync.h"
#include "tui/tui.h"

/// Load .kiro/agents/*.json: append agent prompt to system prompt,
/// read resource files into initial context string.
/// Only used in REPL mode — sync mode skips this for speed (ADR-066).
/// Respects system prompt budget: skips resources if prompt already exceeds
/// 4096 chars (~1024 tokens) to avoid overwhelming small models (ADR-087).
static std::string load_kiro_context(Config& cfg, bool color) {
  DIR* dir = opendir(".kiro/agents");
  if (!dir) {
    return "";
  }
  // Budget: max chars for system prompt before we stop adding resources
  constexpr size_t max_prompt_chars = 4096;

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
    // Skip resource loading if system prompt already exceeds budget (ADR-087)
    if (cfg.system_prompt.size() > max_prompt_chars) {
      if (cfg.trace) {
        tui::system_msg(std::cerr, color, "[trace: skipping resources — prompt budget exceeded]");
      }
      continue;
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
/// Handles read, exec, write, str_replace, add_line, delete_line annotations.
/// Returns context string to feed back to the model for follow-up turns.
/// Respects sandbox restrictions — blocks operations outside allowed path.
// pmccabe:skip-complexity — handles all annotation types in one pass
#include "sync/sync_annotations.h"

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
    LOG_FEATURE("files_flag");
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

  // Create provider early — used for both sync and REPL mode (ADR-020)
  auto provider = create_provider(cfg);

  // Auto-detect model for sync mode (skip for mock provider)
  if (cfg.model == "auto" && cfg.mode == Mode::Sync && cfg.provider != "mock") {
    // Prefer already-loaded model to avoid cold-start delay (ADR-112)
    // Only query Ollama /api/ps for running models — other providers don't support it
    std::string running;
    if (cfg.provider == "ollama") {
      running = get_running_model(cfg);
    }
    if (!running.empty()) {
      cfg.model = running;
      Config::instance().model = running;
    } else {
      auto models = provider->list_models();
      if (!models.empty()) {
        cfg.model = models[0];
        Config::instance().model = models[0];
      } else {
        std::string hint = (cfg.provider == "ollama") ? "Run: ollama pull qwen3:30b" : "No models available for provider: " + cfg.provider;
        tui::error(std::cerr, color, hint);
        return 1;
      }
    }
    // Recreate provider with the resolved model (ADR-112)
    provider = create_provider(Config::instance());
  }

  if (cfg.mode == Mode::Sync) {
    LOG_FEATURE("sync_mode");
    // DIP: Sync mode uses a lean system prompt unless capabilities are set.
    // Tool annotations are useless without capabilities, and the full prompt
    // adds ~500 tokens of overhead that slows down thinking models (ADR-066).
    // With --system-prompt override, the user's choice is respected.
    if (cfg.capabilities.empty() && !cfg.system_prompt_override) {
      cfg.system_prompt = "You are a helpful assistant. Be concise and direct.";
    }
    // Sync mode: one-shot, response to stdout (ADR-007)
    // Mock provider goes through the same flow — echoes prompt as response
    // so annotation processing and session work identically in tests.
    // Streaming: tokens are printed to stdout as they arrive.
    // The full response is returned for annotation processing.
    // Provider-based response generation (ADR-020).
    auto generate_response = [&provider, &cfg, color](const std::vector<Message>& msgs) -> std::string {
      if (cfg.provider == "mock") {
        LOG_FEATURE("mock_provider");
      }
      tui::system_msg(std::cerr, color, "Connecting to " + provider->host() + " with model " + cfg.model + "...");
      return provider->chat_stream(msgs, [](const std::string& token) {
        std::cout << token << std::flush;
        return true;
      });
    };

    // Load session history if --session is set (ADR-056)
    std::vector<Message> messages;
    if (!cfg.session_path.empty()) {
      LOG_FEATURE("session_load");
      messages = load_session(cfg.session_path);
    }
    // Add system prompt only if starting a new conversation
    if (messages.empty() && !cfg.system_prompt.empty()) {
      std::string sys = cfg.system_prompt;
      if (!cfg.system_prompt_override && is_small_model(cfg.model)) {
        sys = SMALL_SYSTEM_PROMPT;
      }
      messages.push_back({"system", sys});
    }
    messages.push_back({"user", cfg.prompt});
    std::string response = generate_response(messages);
    if (!response.empty()) {
      // Streaming already printed tokens to stdout — just add newline
      if (cfg.provider != "mock") {
        std::cout << "\n";
      } else {
        std::cout << response << "\n";
      }
      messages.push_back({"assistant", response});
      // Process annotations if capabilities are set (ADR-056)
      int max_rounds = 10;
      while (!cfg.capabilities.empty() && max_rounds > 0) {
        --max_rounds;
        std::string followup = process_sync_annotations(fix_malformed_tags(response), cfg);
        if (followup.empty()) {
          break;
        }
        messages.push_back({"user", followup});
        response = generate_response(messages);
        if (response.empty()) {
          break;
        }
        // Streaming already printed — just newline
        if (cfg.provider != "mock") {
          std::cout << "\n";
        } else {
          std::cout << response << "\n";
        }
        messages.push_back({"assistant", response});
      }
      // Save updated history if --session is set (ADR-056)
      if (!cfg.session_path.empty()) {
        save_session(cfg.session_path, messages);
      }
    }
  } else {
    // Interactive mode: REPL loop (ADR-012)
    LOG_FEATURE("repl_mode");

    // Startup scan: build unified model registry (ADR-081)
    ModelRegistry registry;
    if (cfg.provider != "mock") {
      tui::system_msg(std::cerr, color, "Scanning providers...");
      registry = scan_all_providers();
      // Show scan results
      std::map<std::string, int> prov_counts;
      for (const auto& m : registry.models) {
        prov_counts[m.provider + "@" + m.host]++;
      }
      for (const auto& [key, count] : prov_counts) {
        tui::system_msg(std::cerr, color, "  ✓ " + key + " — " + std::to_string(count) + " model" + (count > 1 ? "s" : ""));
      }
      tui::system_msg(std::cerr, color,
                      std::to_string(registry.models.size()) + " models across " + std::to_string(registry.host_count()) + " hosts.");
    }

    // Auto-detect model: pick fastest in sweetspot (11-28B), or fastest overall
    if (cfg.model == "auto" && cfg.provider != "mock") {
      const ModelEntry* pick = nullptr;
      // Prefer sweetspot (11-28B) on local hosts
      for (const auto& m : registry.models) {
        if (m.params_b >= 11.0 && m.params_b <= 28.0 && m.provider == "ollama") {
          if (!pick || m.tokens_per_sec > pick->tokens_per_sec) {
            pick = &m;
          }
        }
      }
      // Fallback: fastest available
      if (!pick && !registry.models.empty()) {
        pick = &registry.models[0];
      }
      if (pick) {
        cfg.model = pick->name;
        Config::instance().model = pick->name;
        // Switch to the right host
        auto colon = pick->host.find(':');
        if (colon != std::string::npos) {
          Config::instance().host = pick->host.substr(0, colon);
          Config::instance().port = pick->host.substr(colon + 1);
        }
        provider = create_provider(Config::instance());
        std::string speed = pick->tokens_per_sec > 0 ? " (" + std::to_string(static_cast<int>(pick->tokens_per_sec)) + " t/s)" : "";
        tui::system_msg(std::cerr, color, "Auto-selected: " + pick->name + "@" + pick->host + speed);
      } else {
        tui::error(std::cerr, color, "No models found. Run: ollama pull qwen3:30b");
        return 1;
      }
    }

    if (!cfg.no_banner) {
      tui::banner(std::cerr, color);
    }

    // Model fallback: if configured model is not available, warn and pick first available.
    // This prevents HTTP 404 errors during warmup when .env references a deleted model.
    // The user can fix permanently via /model or editing .env.
    if (cfg.provider != "mock") {
      auto available = provider->list_models();
      std::string& model = Config::instance().model;
      bool found = std::find(available.begin(), available.end(), model) != available.end();
      if (!found && !available.empty()) {
        // Warn and fallback — don't crash on a stale config
        tui::system_msg(std::cerr, color, "⚠ model '" + model + "' not found, falling back to " + available[0]);
        model = available[0];
      } else if (!found && available.empty()) {
        // No models at all — host might be unreachable or empty
        tui::system_msg(std::cerr, color, "⚠ no models available on " + cfg.host + ":" + cfg.port);
      }
    }

    // Optional model warmup (only if not already running, skip for mock provider)
    if (cfg.provider != "mock" && Config::instance().warmup && !provider->is_model_running(Config::instance().model)) {
      tui::system_msg(std::cerr, color, "Warming up " + Config::instance().model + "... (Ctrl+C to skip)");
      // Simple spinner loop
      const char* spinner = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏";
      int frame = 0;
      std::vector<Message> warmup_msg = {{"user", "hi"}};

      // We need to run this in a way that allows us to check progress
      // provider->chat_stream doesn't expose the http object for cancellation easily,
      // but we can at least show a spinner.
      provider->chat_stream(warmup_msg, [&](const std::string&) {
        if (g_interrupted) {
          return false;
        }
        std::cerr << "\r" << spinner[frame % 10] << " loading..." << std::flush;
        frame++;
        return true;
      });
      std::cerr << "\r" << "   " << std::flush;  // Clear spinner
    }

    tui::system_msg(std::cerr, color, "llama-cli — connected to " + cfg.host + ":" + cfg.port + " (" + Config::instance().model + ")");
    tui::system_msg(std::cerr, color,
                    "Type " + tui::bold("/host") + " " + tui::bold("/provider") + " " + tui::bold("/model") + " to switch, " +
                        tui::bold("/help") + " for commands.");
    if (cfg.provider == "mock") {
      tui::system_msg(std::cerr, color, "[MOCK MODE] All prompts will be echoed back.\n");
    }
    tui::system_msg(std::cerr, color, "Type your prompt. " + tui::bold("/quit") + " to exit.\n");
    // Load .kiro/agents/ context if present in cwd
    std::string kiro_ctx = load_kiro_context(cfg, color);
    if (!kiro_ctx.empty()) {
      cfg.system_prompt += "\n\n## Project context\n" + kiro_ctx;
    }
    auto generate = [&provider](const std::vector<Message>& msgs) -> std::string { return provider->chat(msgs); };
    auto stream = [&provider](const std::vector<Message>& msgs, StreamCallback on_token) { return provider->chat_stream(msgs, on_token); };
    // Provider's list_models wraps get_available_models with the right config
    auto models_fn = [&provider](const Config& /*cfg*/) { return provider->list_models(); };
    // Switch provider at runtime — recreates provider instance (ADR-089).
    // Callers (/model, /host, /use) set Config fields BEFORE calling this.
    // This only picks a default model if Config::instance().model is still
    // from a different provider (stale). This prevents the race condition where
    // /model sets a specific model, then switch_provider overwrites it with
    // the first model from the registry. The check ensures we only auto-pick
    // when the current model genuinely doesn't belong to the new provider.
    auto switch_prov = [&provider, &registry](const std::string& name) {
      Config::instance().provider = name;
      // Only auto-pick model if current model isn't on this provider
      bool model_valid = false;
      for (const auto& m : registry.models) {
        if (m.provider == name && m.name == Config::instance().model) {
          model_valid = true;
          break;
        }
      }
      if (!model_valid) {
        for (const auto& m : registry.models) {
          if (m.provider == name && m.available) {
            Config::instance().model = m.name;
            if (m.host != "cloud") {
              auto colon = m.host.find(':');
              if (colon != std::string::npos) {
                Config::instance().host = m.host.substr(0, colon);
                Config::instance().port = m.host.substr(colon + 1);
              }
            }
            break;
          }
        }
      }
      provider = create_provider(Config::instance());
    };
    run_repl(generate, cfg, std::cin, std::cout, models_fn, stream, detect_hardware, get_model_info, scan_ollama_hosts, switch_prov,
             &registry);
  }
  return 0;
}
