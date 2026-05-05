/**
 * @file repl_model.cpp
 * @brief Interactive model selection UI for the REPL.
 *
 * SRP: Model listing, sorting, hardware display, and selection prompt.
 * Extracted from repl_commands.cpp to reduce file size (ADR-065).
 *
 * @see docs/adr/adr-049-model-selection-command.md
 */

#include "repl/repl_model.h"

#include <unistd.h>

#include <algorithm>
#include <csignal>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "config/config.h"
#include "logging/logger.h"
#include "ollama/ollama.h"
#include "tui/tui.h"

#ifdef LINENOISE_HPP
// already included
#else
#include "linenoise.hpp"
#endif

/// Global interrupt flag (defined in repl.cpp)
extern volatile sig_atomic_t g_interrupted;

/// Handle /model command: interactive model selection menu.
/// Queries all known hosts for available models, displays a sorted table
/// with hardware info and sweetspot highlighting, and prompts user to pick.
/// Supports re-sorting by name/params/size/quality within the menu.
/// Handle /model command: list models, switch active model, or show current.
/// Aggregates models from all registered providers (ADR-081).
// pmccabe:skip-complexity
// NOLINTNEXTLINE(readability-function-size)
void handle_model_selection(ReplState& s, const std::string& arg) {
  LOG_FEATURE("model_selection");

  // ADR-081: Use unified registry when available
  if (s.registry && !s.registry->models.empty()) {
    const auto& models = s.registry->models;
    s.out << "\n";
    s.out << "  #  Model                          Provider       Host                   Speed\n";
    s.out << "  ── ────────────────────────────── ────────────── ────────────────────── ─────\n";
    for (size_t i = 0; i < models.size(); i++) {
      const auto& m = models[i];
      std::string marker = (m.name == Config::instance().model && m.host.find(Config::instance().host) != std::string::npos) ? "*" : " ";
      std::string speed = m.tokens_per_sec > 0 ? std::to_string(static_cast<int>(m.tokens_per_sec)) + " t/s" : "  ?  ";
      // Dim non-sweetspot models
      bool sweet = (m.params_b >= 11.0 && m.params_b <= 28.0);
      std::string dim = (!sweet && s.color) ? tui::active_theme().system.ansi() : "";
      std::string reset = (!sweet && s.color) ? Style::reset() : "";
      char buf[120];
      snprintf(buf, sizeof(buf), "%s %2zu %-30s %-14s %-22s %s", marker.c_str(), i + 1, m.name.c_str(), m.provider.c_str(), m.host.c_str(),
               speed.c_str());
      s.out << dim << buf << reset << "\n";
    }
    s.out << "\n  Select [1-" << models.size() << "] or /model <name>: ";
    s.out.flush();

    // Read selection — use linenoise in interactive mode for proper terminal handling
    std::string input;
#ifdef LINENOISE_HPP
    if (&s.in == &std::cin) {
      auto quit = linenoise::Readline("", input);
      if (quit || input.empty()) {
        return;
      }
    } else
#endif
    {
      if (!std::getline(s.in, input) || input.empty()) {
        return;
      }
    }
    // Strip trailing \r (raw terminal mode can leave it)
    if (input.back() == '\r') {
      input.pop_back();
    }
    if (input.empty()) {
      return;
    }
    // Try numeric selection
    try {
      int idx = std::stoi(input) - 1;
      if (idx >= 0 && idx < static_cast<int>(models.size())) {
        const auto& pick = models[idx];
        // Switch to selected model's host and provider
        auto colon = pick.host.find(':');
        if (colon != std::string::npos && pick.provider == "ollama") {
          Config::instance().host = pick.host.substr(0, colon);
          Config::instance().port = pick.host.substr(colon + 1);
        }
        Config::instance().model = pick.name;
        Config::instance().provider = pick.provider;
        if (s.switch_provider) {
          s.switch_provider(pick.provider);
        }
        s.out << "[model: " << pick.name << " on " << pick.provider << "@" << pick.host << "]\n";
        return;
      }
    } catch (...) {
    }
    // Try name match
    for (const auto& m : models) {
      if (m.name == input) {
        auto colon = m.host.find(':');
        if (colon != std::string::npos && m.provider == "ollama") {
          Config::instance().host = m.host.substr(0, colon);
          Config::instance().port = m.host.substr(colon + 1);
        }
        Config::instance().model = m.name;
        Config::instance().provider = m.provider;
        if (s.switch_provider) {
          s.switch_provider(m.provider);
        }
        s.out << "[model: " << m.name << " on " << m.provider << "@" << m.host << "]\n";
        return;
      }
    }
    s.out << "Invalid selection: " << input << "\n";
    return;
  }

  // Fallback: legacy model selection (no registry)
  // Detect hardware for sweetspot calculation (injected via hw_fn)
  HardwareInfo hw = s.hw_fn();

  // Collect models from all known hosts (or just the current one)
  // Multi-host support enables distributed Ollama setups (ADR-033)
  struct HostModel {
    std::string name;
    std::string host;
    std::string port;
  };
  std::vector<HostModel> all_models;
  std::map<std::string, ModelInfo> info_map;

  const auto& hosts = Config::instance().hosts;
  if (hosts.empty()) {
    auto models_raw = s.models_fn(s.cfg);
    for (const auto& m : models_raw) {
      all_models.push_back({m, s.cfg.host, s.cfg.port});
    }
    auto infos = s.model_info_fn(s.cfg);
    for (const auto& info : infos) {
      info_map[info.name] = info;
    }
  } else {
    // Multi-host: query each host separately
    for (const auto& hp : hosts) {
      Config tmp = s.cfg;
      auto colon = hp.find(':');
      tmp.host = (colon != std::string::npos) ? hp.substr(0, colon) : hp;
      tmp.port = (colon != std::string::npos) ? hp.substr(colon + 1) : "11434";
      auto models_raw = get_available_models(tmp);
      auto infos = get_model_info(tmp);
      for (const auto& info : infos) {
        info_map[info.name + "@" + tmp.host] = info;
      }
      for (const auto& m : models_raw) {
        all_models.push_back({m, tmp.host, tmp.port});
      }
    }
  }

  if (all_models.empty()) {
    s.out << "No models available.\n";
    return;
  }

  // Determine sort mode from argument (default: params)
  char sort_mode = 'p';
  if (!arg.empty()) {
    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(arg[0])));
    if (c == 'n' || c == 'p' || c == 's' || c == 'q') {
      sort_mode = c;
    }
  }

  // Helper to extract numeric value from "27.8B" or "1.2GB" strings
  auto to_num = [](const std::string& str) -> double {
    try {
      std::string digits;
      for (char c : str) {
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
          digits += c;
        }
      }
      return digits.empty() ? 0 : std::stod(digits);
    } catch (...) {
      return 0;
    }
  };

  // Interactive loop: display table, accept sort command or selection number
  while (true) {
    auto sorted = all_models;
    std::sort(sorted.begin(), sorted.end(), [&](const HostModel& a, const HostModel& b) {
      std::string ka = hosts.empty() ? a.name : a.name + "@" + a.host;
      std::string kb = hosts.empty() ? b.name : b.name + "@" + b.host;
      const auto& ia = info_map[ka];
      const auto& ib = info_map[kb];

      switch (sort_mode) {
        case 'n':  // Sort alphabetically by model name
          return a.name < b.name;
        case 'p': {  // Sort by parameter count (largest first)
          double pa = to_num(ia.params);
          double pb = to_num(ib.params);
          if (pa != pb) {
            return pa > pb;
          }
          return a.name < b.name;
        }
        case 'q': {  // Sort by quantization level (highest quality first)
          if (ia.quant != ib.quant) {
            return ia.quant > ib.quant;
          }
          return ia.size_gb > ib.size_gb;
        }
        case 's':  // Sort by file size on disk (largest first)
        default: {
          if (ia.size_gb != ib.size_gb) {
            return ia.size_gb > ib.size_gb;
          }
          return a.name < b.name;
        }
      }
    });

    size_t max_name = 0;
    for (const auto& m : sorted) {
      if (m.name.size() > max_name) {
        max_name = m.name.size();
      }
    }
    bool multi = !hosts.empty();

    // Display hardware info
    s.out << "\n  🖥️  CPU:  " << hw.cpu << "\n";
    s.out << "  🎮 GPU:  " << hw.gpu << " (" << hw.vram_gb << "GB VRAM estimated)\n";
    s.out << "  🧠 RAM:  " << hw.ram_gb << "GB\n";

    s.out << "\nAvailable models (sorted by "
          << (sort_mode == 'n' ? "name" : (sort_mode == 'p' ? "params" : (sort_mode == 'q' ? "quality" : "size"))) << "):\n\n";
    s.out << "     " << std::left << std::setw(static_cast<int>(max_name + 2)) << "NAME" << std::right << std::setw(10) << "PARAMS"
          << std::right << std::setw(10) << "SIZE" << "  " << "QUANT";
    if (multi) {
      s.out << "  HOST";
    }
    s.out << "\n";
    s.out << "     " << std::string(max_name + 2 + 10 + 10 + 9 + (multi ? 20 : 0), '-') << "\n";

    // Render each model row with sweetspot highlighting
    for (size_t i = 0; i < sorted.size(); i++) {
      std::string info_key = multi ? sorted[i].name + "@" + sorted[i].host : sorted[i].name;
      const auto& m_info = info_map[info_key];
      double params = to_num(m_info.params);

      // Sweetspot: 11B-27.5B models are highlighted (optimal for local hardware)
      bool sweet = (params >= 11.0 && params <= 27.5);
      std::string dim = (sweet || !s.color) ? "" : tui::active_theme().system.ansi();
      std::string reset = (sweet || !s.color) ? "" : Style::reset();

      std::string marker = (sorted[i].name == Config::instance().model) ? " *" : "  ";
      s.out << marker << std::setw(2) << std::right << (i + 1) << ". " << dim;
      s.out << std::left << std::setw(static_cast<int>(max_name + 2)) << sorted[i].name;

      auto it = info_map.find(info_key);
      if (it != info_map.end()) {
        const auto& m_data = it->second;
        s.out << std::right << std::setw(10) << m_data.params;
        if (m_data.size_gb >= 0.1) {
          std::ostringstream ss;
          ss << std::fixed << std::setprecision(1) << m_data.size_gb << "GB";
          s.out << std::setw(10) << ss.str();
        } else {
          s.out << std::setw(10) << "-";
        }
        s.out << "  " << m_data.quant;
      }
      if (multi) {
        s.out << "  " << sorted[i].host;
      }
      s.out << reset << "\n";
    }

    s.out << "\n  * = active  |  GB = disk/VRAM\n";
    s.out << "  -------------------------------------------------------\n";
    s.out << "  PARAMS: Complexity (number of parameters in billions)\n";
    s.out << "  SIZE:   Model size (disk space and VRAM usage)\n";
    s.out << "  QUANT:  Quantization (compression quality: higher is better)\n\n";

    // Prompt user to select by number or re-sort
    std::string input;
    std::string prompt = "Select 1-" + std::to_string(sorted.size()) + " (or n/p/s/q to sort): ";

    if (&s.in == &std::cin && isatty(STDIN_FILENO)) {
      auto quit = linenoise::Readline(prompt.c_str(), input);
      if (quit) {
        s.out << "[cancelled]\n";
        return;
      }
    } else {
      s.out << prompt;
      s.out.flush();
      if (!std::getline(s.in, input)) {
        s.out << "\n[cancelled]\n";
        return;
      }
    }

    if (input.empty()) {
      return;
    }

    // Check for sort command (single letter: n/p/s/q)
    char first = static_cast<char>(std::tolower(static_cast<unsigned char>(input[0])));
    if (input.size() == 1 && (first == 'n' || first == 'p' || first == 's' || first == 'q')) {
      sort_mode = first;
      continue;
    }

    // Parse numeric selection
    int choice = 0;
    try {
      choice = std::stoi(input);
    } catch (...) {
      s.out << "[invalid input: enter a number or n/p/s/q]\n";
      continue;
    }

    if (choice < 1 || choice > static_cast<int>(sorted.size())) {
      s.out << "[out of range]\n";
      continue;
    }

    // Apply selection: update global config
    const auto& selected = sorted[choice - 1];
    Config::instance().model = selected.name;
    Config::instance().host = selected.host;
    Config::instance().port = selected.port;
    if (multi) {
      s.out << "[model set to " << selected.name << " on " << selected.host << ":" << selected.port << "]\n";
    } else {
      s.out << "[model set to " << selected.name << "]\n";
    }

    // Optional warmup after switch to avoid first-prompt delay
    if (s.warmup) {
      s.out << "Warming up " << selected.name << "... (Ctrl+C to skip)\n";
      s.out.flush();
      std::vector<Message> warmup_msg = {{"user", "hi"}};
      ollama_chat_stream(Config::instance(), warmup_msg, [](const std::string&) { return g_interrupted == 0; });
      if (g_interrupted) {
        s.out << "[warmup skipped]\n";
        g_interrupted = 0;
      }
    }
    break;
  }
}
