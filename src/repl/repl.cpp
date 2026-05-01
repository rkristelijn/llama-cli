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
#include <iomanip>
#include <map>
#include <memory>
#include <set>
#include <sstream>

#include "annotation/annotation.h"
#include "command/command.h"
#include "exec/exec.h"
#include "exec/hardware.h"
#include "help.h"
#include "json/json.h"
#include "logging/logger.h"
#include "net/scan.h"
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
volatile sig_atomic_t g_interrupted = 0;

/** SIGINT handler — sets flag so spinner/chat can check and abort */
static void sigint_handler(int /*sig*/) { g_interrupted = 1; }

/// REPL session state — groups related data to reduce parameter passing
struct ReplState {
  ChatFn& chat;                     ///< Injected chat function (real or mock)
  StreamChatFn stream_chat;         ///< Streaming chat function (nullable)
  ModelsFn& models_fn;              ///< Injected model fetcher (real or mock)
  ModelInfoFn model_info_fn;        ///< Injected model info fetcher (real or mock)
  HardwareFn hw_fn;                 ///< Injected hardware detector (real or mock)
  ScanFn scan_fn;                   ///< Injected network scanner (real or mock)
  const Config& cfg;                ///< Configuration (timeouts, etc.)
  std::vector<Message>& history;    ///< Conversation history
  std::istream& in;                 ///< Input stream
  std::ostream& out;                ///< Output stream
  int count = 0;                    ///< Number of prompts processed
  bool color = false;               ///< Whether to use ANSI colors (TTY detect)
  bool interactive = false;         ///< Whether running on a real TTY (for spinner)
  bool markdown = true;             ///< Whether to render markdown in LLM output
  bool bofh = false;                ///< BOFH mode: sarcastic spinner
  bool warmup = false;              ///< Whether to warm up model on switch
  std::string prompt_color = "32";  ///< ANSI code for user prompt (green)
  std::string ai_color = "";        ///< ANSI code for AI response (none=default)
  bool trust = false;               ///< Trust mode: auto-approve all write/exec/str_replace (reset on /clear)
  int last_assistant_idx = -1;      ///< Index of last assistant message for rating
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
  auto status = [&](bool val) {
    if (!s.color) return val ? "on" : "off";
    return val ? "\033[32mon\033[0m" : "\033[31moff\033[0m";
  };

  s.out << "Options (toggle with /set <option>):\n";
  s.out << "  markdown  " << std::left << std::setw(15) << status(s.markdown) << "Render AI responses with formatting and lists\n";
  s.out << "  color     " << std::left << std::setw(15) << status(s.color) << "Enable ANSI colors in terminal output\n";
  s.out << "  warmup    " << std::left << std::setw(15) << status(s.warmup)
        << "Pre-load model on startup/switch to avoid first-prompt delay\n";
  s.out << "  bofh      " << std::left << std::setw(15) << status(s.bofh) << "Enable 'Bastard Operator From Hell' sarcastic spinner\n";
  s.out << "  trace     " << std::left << std::setw(15) << status(Config::instance().trace)
        << "Show detailed HTTP traffic and timing logs\n";

  s.out << "\nColors (/color prompt|ai <name>):\n";
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
      {"warmup", &ReplState::warmup},
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

// NOLINTBEGIN(readability-braces-around-statements)
// Map color name to ANSI code — lookup table as if-chain for readability
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
// NOLINTEND(readability-braces-around-statements)

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
static void handle_model_selection(ReplState& s, const std::string& arg) {
  // Detect hardware for sweetspot calculation (injected)
  HardwareInfo hw = s.hw_fn();

  // Collect models from all known hosts (or just the current one)
  struct HostModel {
    std::string name;
    std::string host;
    std::string port;
  };
  std::vector<HostModel> all_models;
  std::map<std::string, ModelInfo> info_map;

  auto& hosts = Config::instance().hosts;
  if (hosts.empty()) {
    // Single host mode — use injected functions
    auto models_raw = s.models_fn(s.cfg);
    for (const auto& m : models_raw) {
      all_models.push_back({m, s.cfg.host, s.cfg.port});
    }
    auto infos = s.model_info_fn(s.cfg);
    for (const auto& info : infos) info_map[info.name] = info;
  } else {
    // Multi-host mode — query each discovered host
    for (const auto& hp : hosts) {
      Config tmp = s.cfg;
      auto colon = hp.find(':');
      tmp.host = (colon != std::string::npos) ? hp.substr(0, colon) : hp;
      tmp.port = (colon != std::string::npos) ? hp.substr(colon + 1) : "11434";
      auto models_raw = get_available_models(tmp);
      auto infos = get_model_info(tmp);
      for (const auto& info : infos) info_map[info.name + "@" + tmp.host] = info;
      for (const auto& m : models_raw) {
        all_models.push_back({m, tmp.host, tmp.port});
      }
    }
  }

  if (all_models.empty()) {
    s.out << "No models available.\n";
    return;
  }

  // Determine sort mode from argument (default to params)
  char sort_mode = 'p';
  if (!arg.empty()) {
    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(arg[0])));
    if (c == 'n' || c == 'p' || c == 's' || c == 'q') {
      sort_mode = c;
    }
  }

  // Helper to get numeric value from "27.8B" or "1.2GB" strings
  auto to_num = [](const std::string& str) -> double {
    try {
      std::string digits;
      for (char c : str) {
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') digits += c;
      }
      return digits.empty() ? 0 : std::stod(digits);
    } catch (...) {
      return 0;
    }
  };

  while (true) {
    // Sort based on chosen mode
    auto sorted = all_models;
    std::sort(sorted.begin(), sorted.end(), [&](const HostModel& a, const HostModel& b) {
      // Lookup key depends on multi-host mode
      std::string ka = hosts.empty() ? a.name : a.name + "@" + a.host;
      std::string kb = hosts.empty() ? b.name : b.name + "@" + b.host;
      const auto& ia = info_map[ka];
      const auto& ib = info_map[kb];

      switch (sort_mode) {
        case 'n':
          return a.name < b.name;
        case 'p': {
          double pa = to_num(ia.params);
          double pb = to_num(ib.params);
          if (pa != pb) return pa > pb;
          return a.name < b.name;
        }
        case 'q': {
          if (ia.quant != ib.quant) return ia.quant > ib.quant;
          return ia.size_gb > ib.size_gb;
        }
        case 's':
        default: {
          if (ia.size_gb != ib.size_gb) return ia.size_gb > ib.size_gb;
          return a.name < b.name;
        }
      }
    });

    // Find max name length for column alignment
    size_t max_name = 0;
    for (const auto& m : sorted) {
      if (m.name.size() > max_name) max_name = m.name.size();
    }
    bool multi = !hosts.empty();

    // Display hardware info
    s.out << "\n  🖥️  CPU:  " << hw.cpu << "\n";
    s.out << "  🎮 GPU:  " << hw.gpu << " (" << hw.vram_gb << "GB VRAM estimated)\n";
    s.out << "  🧠 RAM:  " << hw.ram_gb << "GB\n";

    // Display aligned table
    s.out << "\nAvailable models (sorted by "
          << (sort_mode == 'n' ? "name" : (sort_mode == 'p' ? "params" : (sort_mode == 'q' ? "quality" : "size"))) << "):\n\n";
    s.out << "     " << std::left << std::setw(static_cast<int>(max_name + 2)) << "NAME" << std::right << std::setw(10) << "PARAMS"
          << std::right << std::setw(10) << "SIZE" << "  " << "QUANT";
    if (multi) s.out << "  HOST";
    s.out << "\n";
    s.out << "     " << std::string(max_name + 2 + 10 + 10 + 9 + (multi ? 20 : 0), '-') << "\n";

    for (size_t i = 0; i < sorted.size(); i++) {
      std::string info_key = multi ? sorted[i].name + "@" + sorted[i].host : sorted[i].name;
      const auto& m_info = info_map[info_key];
      double params = to_num(m_info.params);

      // Sweetspot: 11B to 27B
      bool sweet = (params >= 11.0 && params <= 27.5);
      std::string dim = (sweet || !s.color) ? "" : "\033[2m";
      std::string reset = (sweet || !s.color) ? "" : "\033[0m";

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
      if (multi) s.out << "  " << sorted[i].host;
      s.out << reset << "\n";
    }

    s.out << "\n  * = active  |  GB = disk/VRAM\n";
    s.out << "  -------------------------------------------------------\n";
    s.out << "  PARAMS: Complexity (number of parameters in billions)\n";
    s.out << "  SIZE:   Model size (disk space and VRAM usage)\n";
    s.out << "  QUANT:  Quantization (compression quality: higher is better)\n\n";

    // Prompt user to select by number
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

    // Check for re-sort command
    char first = static_cast<char>(std::tolower(static_cast<unsigned char>(input[0])));
    if (input.size() == 1 && (first == 'n' || first == 'p' || first == 's' || first == 'q')) {
      sort_mode = first;
      continue;
    }

    // Parse selection number
    int choice = 0;
    try {
      choice = std::stoi(input);
    } catch (...) {
      s.out << "[invalid input: enter a number or n/p/s/q]\n";
      continue;
    }

    // Validate range (1-indexed for user, 0-indexed for vector)
    if (choice < 1 || choice > static_cast<int>(sorted.size())) {
      s.out << "[out of range]\n";
      continue;
    }

    // Update config with selected model and host
    auto& selected = sorted[choice - 1];
    Config::instance().model = selected.name;
    Config::instance().host = selected.host;
    Config::instance().port = selected.port;
    if (multi) {
      s.out << "[model set to " << selected.name << " on " << selected.host << ":" << selected.port << "]\n";
    } else {
      s.out << "[model set to " << selected.name << "]\n";
    }

    // Optional warmup after switch
    if (s.warmup) {
      s.out << "Warming up " << selected.name << "... (Ctrl+C to skip)\n";
      s.out.flush();
      // Use streaming chat for warmup so it can be interrupted
      std::vector<Message> warmup_msg = {{"user", "hi"}};
      ollama_chat_stream(s.cfg, warmup_msg, [](const std::string&) {
        return g_interrupted == 0;  // stop if interrupted
      });
      if (g_interrupted) {
        s.out << "[warmup skipped]\n";
        g_interrupted = 0;
      }
    }
    break;
  }
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
/// Ensure parent directory exists for a file path (ADR-059)
static void ensure_parent_dir(const std::string& path) {
  auto slash = path.rfind('/');
  if (slash != std::string::npos) {
    std::string dir = path.substr(0, slash);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    mkdir(dir.c_str(), 0755);
  }
}

/// Read a plain-text file, return empty string if missing
static std::string read_file_or_empty(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) {
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

/// Append a line to a file, creating parent dirs as needed (ADR-059)
static void append_line(const std::string& path, const std::string& line) {
  ensure_parent_dir(path);
  std::ofstream f(path, std::ios::app);
  if (f.is_open()) {
    f << "- " << line << "\n";
  }
}

/**
 * @brief Handle /scan — discover Ollama servers on the local network.
 * Scans the local subnet for port 11434, writes results to .env as OLLAMA_HOSTS.
 */
static void handle_scan(ReplState& s) {
  s.out << "Scanning " << get_local_subnet() << "0/24 for Ollama servers...\n";
  s.out.flush();
  auto hosts = s.scan_fn(11434);
  if (hosts.empty()) {
    s.out << "No Ollama servers found on the local network.\n";
    return;
  }
  s.out << "Found " << hosts.size() << " Ollama server(s):\n";
  for (const auto& h : hosts) {
    s.out << "  " << h << "\n";
  }
  // Write to .env (append or update OLLAMA_HOSTS line)
  std::string hosts_str;
  for (size_t i = 0; i < hosts.size(); i++) {
    if (i > 0) hosts_str += ",";
    hosts_str += hosts[i];
  }
  // Read existing .env, replace or append OLLAMA_HOSTS
  std::vector<std::string> lines;
  bool found = false;
  std::ifstream rf(".env");
  if (rf.is_open()) {
    std::string line;
    while (std::getline(rf, line)) {
      if (line.rfind("OLLAMA_HOSTS=", 0) == 0) {
        lines.push_back("OLLAMA_HOSTS=" + hosts_str);
        found = true;
      } else {
        lines.push_back(line);
      }
    }
    rf.close();
  }
  if (!found) {
    lines.push_back("OLLAMA_HOSTS=" + hosts_str);
  }
  std::ofstream wf(".env");
  if (wf) {
    for (const auto& l : lines) wf << l << "\n";
    s.out << "Saved to .env (OLLAMA_HOSTS=" << hosts_str << ")\n";
  }
  // Update runtime config
  Config::instance().hosts = hosts;
}

/**
 * @brief Handle /mem — view or modify persistent memory (ADR-059).
 *
 * Subcommands: (none) = show, add <fact> = append, clear = wipe.
 */
static void handle_mem(const std::string& arg, ReplState& s) {
  const std::string& path = s.cfg.memory_path;
  if (arg.empty()) {
    std::string content = read_file_or_empty(path);
    if (content.empty()) {
      s.out << "[no memories stored]\n";
    } else {
      s.out << content;
    }
    return;
  }
  auto space = arg.find(' ');
  std::string sub = (space != std::string::npos) ? arg.substr(0, space) : arg;
  std::string val = (space != std::string::npos) ? arg.substr(space + 1) : "";

  if (sub == "add" && !val.empty()) {
    append_line(path, val);
    s.out << "[remembered: " << val << "]\n";
    LOG_EVENT("repl", "mem_add", val, path, 0, 0, 0);
  } else if (sub == "clear") {
    std::ofstream f(path, std::ios::trunc);
    s.out << "[memory cleared]\n";
    LOG_EVENT("repl", "mem_clear", path, "", 0, 0, 0);
  } else {
    s.out << "Usage: /mem, /mem add <fact>, /mem clear\n";
  }
}

/**
 * @brief Handle /pref command: show, add, or clear preferences (ADR-059).
 *
 * Subcommands: (none) = show, add <pref> = append, clear = wipe.
 */
static void handle_pref(const std::string& arg, ReplState& s) {
  const std::string& path = s.cfg.preferences_path;
  if (arg.empty()) {
    std::string content = read_file_or_empty(path);
    if (content.empty()) {
      s.out << "[no preferences stored]\n";
    } else {
      s.out << content;
    }
    return;
  }
  auto space = arg.find(' ');
  std::string sub = (space != std::string::npos) ? arg.substr(0, space) : arg;
  std::string val = (space != std::string::npos) ? arg.substr(space + 1) : "";

  if (sub == "add" && !val.empty()) {
    append_line(path, val);
    s.out << "[preference saved: " << val << "]\n";
    LOG_EVENT("repl", "pref_add", val, path, 0, 0, 0);
  } else if (sub == "clear") {
    std::ofstream f(path, std::ios::trunc);
    s.out << "[preferences cleared]\n";
    LOG_EVENT("repl", "pref_clear", path, "", 0, 0, 0);
  } else {
    s.out << "Usage: /pref, /pref add <preference>, /pref clear\n";
  }
}

/**
 * @brief Handle /rate command: rate last or specific response.
 *
 * /rate last +    — rate last response positive
 * /rate last -    — rate last response negative
 * /rate last s    — save for review
 * /rate 5 +       — rate 5th assistant response positive
 * /rate list      — show saved responses
 */
static void handle_rate(const std::string& arg, ReplState& s) {
  if (arg == "list") {
    int count = 0;
    for (size_t i = 0; i < s.history.size(); ++i) {
      if (s.history[i].role == "assistant" && !s.history[i].rating.empty()) {
        ++count;
        std::string preview = s.history[i].content.substr(0, 60);
        s.out << count << ". [" << s.history[i].rating << "] " << preview << "...\n";
      }
    }
    if (count == 0) {
      s.out << "[no rated responses]\n";
    }
    return;
  }

  // Parse: /rate <index> <rating>
  std::string index_str, rating_str;
  auto space = arg.find(' ');
  if (space == std::string::npos) {
    s.out << "Usage: /rate last +/-, /rate <n> +/-, /rate list\n";
    return;
  }
  index_str = arg.substr(0, space);
  rating_str = arg.substr(space + 1);

  // Find target message
  int target_idx = -1;
  if (index_str == "last") {
    target_idx = s.last_assistant_idx;
  } else {
    try {
      int n = std::stoi(index_str);
      // Find nth assistant message
      int count = 0;
      for (size_t i = 0; i < s.history.size(); ++i) {
        if (s.history[i].role == "assistant") {
          ++count;
          if (count == n) {
            target_idx = static_cast<int>(i);
            break;
          }
        }
      }
    } catch (...) {
      s.out << "Invalid index. Use 'last' or a number.\n";
      return;
    }
  }

  if (target_idx < 0 || target_idx >= static_cast<int>(s.history.size())) {
    s.out << "Response not found.\n";
    return;
  }

  // Parse rating
  std::string rating;
  if (rating_str == "+" || rating_str == "y" || rating_str == "yes" || rating_str == "good") {
    rating = "positive";
  } else if (rating_str == "-" || rating_str == "n" || rating_str == "no" || rating_str == "bad") {
    rating = "negative";
  } else if (rating_str == "s" || rating_str == "save") {
    rating = "saved";
  } else {
    s.out << "Invalid rating. Use: + (good), - (bad), s (save)\n";
    return;
  }

  s.history[target_idx].rating = rating;
  s.out << "[rated: " << rating << "]\n";

  // Log the rating
  LOG_EVENT("repl", "rate_response", s.history[target_idx].content, rating, 0, 0, 0);
}

static bool handle_command(const ParsedInput& input, ReplState& s) {
  if (input.command == "clear") {
    s.history.clear();
    s.out << "[history cleared]\n";
  } else if (input.command == "set" || input.command == "options") {
    handle_set(input.arg, s);
  } else if (input.command == "color") {
    handle_color(input.arg, s);
  } else if (input.command == "model") {
    handle_model_selection(s, input.arg);
  } else if (input.command == "version") {
    s.out << "llama-cli " << get_version() << "\n";
  } else if (input.command == "mem") {
    handle_mem(input.arg, s);
  } else if (input.command == "pref") {
    handle_pref(input.arg, s);
  } else if (input.command == "rate") {
    handle_rate(input.arg, s);
  } else if (input.command == "copy" || input.command == "c") {
    // Copy last assistant response to clipboard
    if (s.last_assistant_idx < 0 || s.last_assistant_idx >= static_cast<int>(s.history.size())) {
      s.out << "[no response to copy]\n";
    } else {
      const std::string& content = s.history[s.last_assistant_idx].content;
      FILE* pipe = popen("pbcopy 2>/dev/null || xclip -selection clipboard 2>/dev/null", "w");  // error-handling:ok
      if (pipe) {
        fwrite(content.c_str(), 1, content.size(), pipe);
        pclose(pipe);
        s.out << "[copied " << content.size() << " chars to clipboard]\n";
      } else {
        s.out << "[clipboard not available — install pbcopy or xclip]\n";
      }
    }
  } else if (input.command == "paste" || input.command == "p") {
    // Paste clipboard content as context for the LLM
    FILE* pipe = popen("pbpaste 2>/dev/null || xclip -selection clipboard -o 2>/dev/null", "r");  // error-handling:ok
    if (pipe) {
      std::string content;
      char buf[4096];
      while (fgets(buf, sizeof(buf), pipe)) {
        content += buf;
      }
      pclose(pipe);
      if (content.empty()) {
        s.out << "[clipboard is empty]\n";
      } else {
        s.history.push_back({"user", "[clipboard]\n" + content});
        s.out << "[pasted " << content.size() << " chars as context]\n";
      }
    } else {
      s.out << "[clipboard not available — install pbpaste or xclip]\n";
    }
  } else if (input.command == "help" || input.command.empty()) {
    s.out << help::repl;
  } else if (input.command == "scan") {
    handle_scan(s);
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
 * @brief Print a git-style unified diff with 3 lines of context and @@ hunk headers.
 *
 * Uses Myers diff (via dtl) for accurate change detection. Only changed lines
 * and their surrounding context are shown — unchanged regions are skipped.
 */
// pmccabe:skip-complexity
// NOLINTNEXTLINE(readability-function-size)
static void show_diff(const std::string& old_text, const std::string& new_text, std::ostream& out, bool color) {
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

  // Build flat list with old/new line numbers
  struct Entry {
    dtl::edit_t type;
    std::string text;
    int old_ln;
    int new_ln;
  };
  std::vector<Entry> entries;
  int oln = 1, nln = 1;
  for (const auto& elem : diff.getSes().getSequence()) {
    Entry e;
    e.type = elem.second.type;
    e.text = elem.first;
    e.old_ln = oln;
    e.new_ln = nln;
    if (e.type == dtl::SES_DELETE) {
      ++oln;
    } else if (e.type == dtl::SES_ADD) {
      ++nln;
    } else {
      ++oln;
      ++nln;
    }
    entries.push_back(e);
  }

  // Mark which entries to show (changed lines + 3 lines context)
  const int ctx = 3;
  std::vector<bool> show(entries.size(), false);
  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].type != dtl::SES_COMMON) {
      size_t lo = (i >= static_cast<size_t>(ctx)) ? i - ctx : 0;
      size_t hi = std::min(i + ctx, entries.size() - 1);
      for (size_t j = lo; j <= hi; ++j) {
        show[j] = true;
      }
    }
  }

  // Emit hunks
  bool in_hunk = false;
  for (size_t i = 0; i < entries.size(); ++i) {
    if (!show[i]) {
      in_hunk = false;
      continue;
    }
    if (!in_hunk) {
      // Find hunk extent to compute header
      size_t end = i;
      while (end < entries.size() && show[end]) {
        ++end;
      }
      int o_start = entries[i].old_ln, o_count = 0;
      int n_start = entries[i].new_ln, n_count = 0;
      for (size_t j = i; j < end; ++j) {
        if (entries[j].type != dtl::SES_ADD) {
          ++o_count;
        }
        if (entries[j].type != dtl::SES_DELETE) {
          ++n_count;
        }
      }
      if (color) {
        out << "\033[36m";
      }
      out << "@@ -" << o_start << "," << o_count << " +" << n_start << "," << n_count << " @@";
      if (color) {
        out << "\033[0m";
      }
      out << "\n";
      in_hunk = true;
    }
    const auto& e = entries[i];
    if (e.type == dtl::SES_DELETE) {
      emit_diff_line(out, "\033[1;31m", "- ", e.text, color);
    } else if (e.type == dtl::SES_ADD) {
      emit_diff_line(out, "\033[1;32m", "+ ", e.text, color);
    } else {
      out << "  " << e.text << "\n";
    }
  }
}

/**
 * @brief Prompt the user to confirm writing a proposed file change.
 *
 * Always shows a diff (for existing files) or the full content (for new files)
 * before prompting. Accepts:
 *   y/yes = confirm write
 *   n/no  = decline
 *   s/show = re-display content
 *   t/trust = approve this and all remaining actions this session
 *   c/copy = copy content to clipboard (pbcopy/xclip)
 */
// todo: reduce complexity of confirm_write
// pmccabe:skip-complexity
static bool confirm_write(const WriteAction& action, std::istream& in, std::ostream& out, bool color, bool& trust) {
  if (trust) {
    return true;
  }
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

  if (Config::instance().auto_confirm_write) {
    // Diff/content already shown above — proceed directly to write.
    LOG_EVENT("repl", "file_write", action.path, "auto-confirmed", 0, 0, 0);
    out << "[auto-written: " << action.path << "]\n";  // User feedback

    // *** The actual file writing and backup logic from the original function
    // *** (around lines ~950 onwards) needs to be executed here to complete the operation.
    // For demonstration, we return true, simulating successful bypass.
    // The actual write logic needs to be integrated here.
    // For example, it would proceed to call write_file_with_backup or similar.
    // The original function's structure implies the write happens *after* confirmation.
    // For automatic confirmation, we'd need to execute that part here.
    // To be precise, the entire write logic needs to be structured to be callable here.
    return true;
  }
  // Original interactive prompt logic follows if auto_confirm_write is false
  std::string opts = "[y/n/s/t/c]";
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
    if (answer == "t" || answer == "trust") {
      trust = true;
      return true;
    }
    if (answer == "c" || answer == "copy") {
      // Copy content to clipboard (macOS: pbcopy, Linux: xclip)
      FILE* pipe = popen("pbcopy 2>/dev/null || xclip -selection clipboard 2>/dev/null", "w");
      if (pipe) {
        fwrite(action.content.c_str(), 1, action.content.size(), pipe);
        pclose(pipe);
        out << "[copied to clipboard]\n";
      }
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
static void process_write(const WriteAction& action, std::istream& in, std::ostream& out, bool color, bool& trust) {
  if (confirm_write(action, in, out, color, trust)) {
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
      // Log successful file write for audit trail
      LOG_EVENT("repl", "file_write", action.path, "ok", 0, 0, 0);
    } else {
      tui::error(out, color, "Error: could not write to " + action.path);
      LOG_EVENT("repl", "file_write", action.path, "error: could not write", 0, 0, 0);
    }
  } else {
    out << "[skipped]\n";
    // Log declined write so we can see rejection patterns
    LOG_EVENT("repl", "file_write_declined", action.path, "", 0, 0, 0);
  }
}

/**
 * @brief Apply a <str_replace> action: show diff, prompt, then do targeted replacement.
 */
static void process_str_replace(const StrReplaceAction& action, std::istream& in, std::ostream& out, bool color, bool& trust) {
  if (trust) {
    // Auto-approve in trust mode — skip confirmation
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
    std::string updated = existing;
    auto replace_pos = updated.find(action.old_str);
    updated.replace(replace_pos, action.old_str.size(), action.new_str);
    show_diff(existing, updated, out, color);
    // fall through to write
    std::ofstream bak(action.path + ".bak");
    if (bak.is_open()) {
      bak << existing;
    }
    std::ofstream file(action.path);
    if (file.is_open()) {
      file << updated;
      out << "[wrote " << action.path << "]\n";
      LOG_EVENT("repl", "str_replace", action.path, "", 0, 0, 0);
    } else {
      tui::error(out, color, "str_replace: could not write to " + action.path);
    }
    return;
  }
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

  out << "Apply str_replace to " << action.path << "? [y/n/t/c] " << std::flush;
  std::string answer;
  if (!std::getline(in, answer)) {
    out << "[skipped]\n";
    LOG_EVENT("repl", "str_replace_declined", action.path, "", 0, 0, 0);
    return;
  }
  if (answer == "t" || answer == "trust") {
    trust = true;
  } else if (answer == "c" || answer == "copy") {
    FILE* pipe = popen("pbcopy 2>/dev/null || xclip -selection clipboard 2>/dev/null", "w");
    if (pipe) {
      fwrite(action.new_str.c_str(), 1, action.new_str.size(), pipe);
      pclose(pipe);
      out << "[copied to clipboard]\n";
    }
    return;
  } else if (answer != "y" && answer != "yes") {
    out << "[skipped]\n";
    LOG_EVENT("repl", "str_replace_declined", action.path, "", 0, 0, 0);
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
    // Log successful str_replace for audit trail
    LOG_EVENT("repl", "str_replace", action.path, "ok", 0, 0, 0);
  } else {
    tui::error(out, color, "Error: could not write to " + action.path);
    LOG_EVENT("repl", "str_replace", action.path, "error: could not write", 0, 0, 0);
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
    LOG_EVENT("repl", "file_read", action.path, "error: not found", 0, 0, 0);
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
  // Log file read so we can track which files the LLM accesses
  LOG_EVENT("repl", "file_read", action.path, "", 0, 0, 0);
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
    result.replace(start, end + close.size() - start, "\033[1;37m[proposed: exec " + cmd + "]\033[0m");
  }
  // Strip any remaining annotation-like tags so raw XML never reaches the user
  for (const auto& tag : {"<exec>", "</exec>", "<write", "</write>", "<str_replace", "</str_replace>", "<read ", "<search>", "</search>"}) {
    size_t pos = 0;
    std::string needle(tag);
    while ((pos = result.find(needle, pos)) != std::string::npos) {
      auto end = result.find('>', pos);
      if (end == std::string::npos) {
        break;
      }
      result.erase(pos, end - pos + 1);
    }
  }
  return result;
}

/** Confirm and execute an LLM-proposed command (ADR-015).
 * Prompts user with y/n/t/c:
 *   y = execute, n = skip, t = trust (auto-approve rest of session),
 *   c = copy command to clipboard.
 * Returns output if executed, empty string if declined/copied. */
static std::string confirm_exec(const std::string& cmd, const Config& cfg, std::istream& in, std::ostream& out, bool& trust) {
  if (!trust) {
    out << "Run: \033[1;33m" << cmd << "\033[0m? [y/n/t/c] " << std::flush;
    std::string answer;
    if (!std::getline(in, answer)) {
      return "";
    }
    if (answer == "t" || answer == "trust") {
      trust = true;
    } else if (answer == "c" || answer == "copy") {
      FILE* pipe = popen("pbcopy 2>/dev/null || xclip -selection clipboard 2>/dev/null", "w");
      if (pipe) {
        fwrite(cmd.c_str(), 1, cmd.size(), pipe);
        pclose(pipe);
        out << "[copied to clipboard]\n";
      }
      return "";
    } else if (answer != "y" && answer != "yes") {
      LOG_EVENT("repl", "exec_declined", cmd, "", 0, 0, 0);
      out << "[skipped]\n";
      return "";
    }
  }
  auto t0 = std::chrono::steady_clock::now();
  auto r = cmd_exec(cmd, cfg.exec_timeout, cfg.max_output);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] exec (LLM): %s exit=%d %ldms\n", cmd.c_str(), r.exit_code, ms);
  }

  // Log confirmed LLM-proposed exec with duration
  LOG_EVENT("repl", "exec_confirmed", cmd, r.output, static_cast<int>(ms), 0, 0);
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
  if (!s.color || s.ai_color.empty()) {
    return text;
  }
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

/**
 * @brief Ensure SearXNG is running when web search is enabled (ADR-057).
 *
 * Checks Docker availability, starts the daemon if needed (macOS: open -a Docker),
 * then ensures the searxng container is running. Pulls the image on first use.
 */
static void ensure_searxng(const Config& cfg, std::ostream& out) {
  bool trace = Config::instance().trace;
  // Quick check: is SearXNG already responding?
  if (trace) {
    stderr_trace->log("[TRACE] web-search: probing %s\n", cfg.search_url.c_str());
  }
  auto probe = cmd_exec("curl -sf -o /dev/null -m 2 '" + cfg.search_url + "'", 3, 100);
  if (probe.exit_code == 0) {
    if (trace) {
      stderr_trace->log("[TRACE] web-search: SearXNG already running\n");
    }
    return;  // already running
  }

  // Check if docker CLI is available
  if (trace) {
    stderr_trace->log("[TRACE] web-search: checking docker CLI\n");
  }
  auto docker_check = cmd_exec("docker --version", 3, 200);
  if (docker_check.exit_code != 0) {
    out << "\033[33m[web-search] docker not found — install Docker to enable web search\033[0m\n";
    return;
  }

  // Check if Docker daemon is running
  if (trace) {
    stderr_trace->log("[TRACE] web-search: checking Docker daemon\n");
  }
  auto daemon_check = cmd_exec("docker info", 5, 500);
  if (daemon_check.exit_code != 0) {
    out << "\033[33m[web-search] starting Docker...\033[0m\n";
    if (trace) {
      stderr_trace->log("[TRACE] web-search: starting Docker daemon\n");
    }
#ifdef __APPLE__
    cmd_exec("open -a Docker", 5, 100);
#else
    cmd_exec("sudo systemctl start docker", 10, 200);
#endif
    // Wait for daemon to be ready (up to 30s)
    for (int i = 0; i < 15; ++i) {
      auto ready = cmd_exec("docker info", 3, 500);
      if (ready.exit_code == 0) {
        if (trace) {
          stderr_trace->log("[TRACE] web-search: Docker daemon ready\n");
        }
        break;
      }
      cmd_exec("sleep 2", 3, 10);
    }
  }

  // Check if searxng container exists and is running
  if (trace) {
    stderr_trace->log("[TRACE] web-search: checking searxng container\n");
  }
  auto running = cmd_exec("docker ps -q -f name=searxng", 5, 200);
  if (!running.output.empty()) {
    if (trace) {
      stderr_trace->log("[TRACE] web-search: container already running\n");
    }
    return;  // container running, SearXNG may still be starting up
  }

  // Check if container exists but is stopped
  auto exists = cmd_exec("docker ps -aq -f name=searxng", 5, 200);
  if (!exists.output.empty()) {
    out << "\033[33m[web-search] starting SearXNG container...\033[0m\n";
    if (trace) {
      stderr_trace->log("[TRACE] web-search: docker start searxng\n");
    }
    cmd_exec("docker start searxng", 10, 200);
  } else {
    out << "\033[33m[web-search] pulling and starting SearXNG...\033[0m\n";
    if (trace) {
      stderr_trace->log("[TRACE] web-search: docker compose up searxng\n");
    }
    cmd_exec("docker compose -f .config/docker-compose.yml up -d", 60, 5000);
  }

  // Wait for SearXNG to respond (up to 15s)
  for (int i = 0; i < 15; ++i) {
    auto ready = cmd_exec("curl -sf -o /dev/null -m 1 '" + cfg.search_url + "'", 2, 100);
    if (ready.exit_code == 0) {
      out << "\033[32m[web-search] SearXNG ready\033[0m\n";
      if (trace) {
        stderr_trace->log("[TRACE] web-search: SearXNG responding\n");
      }
      return;
    }
    cmd_exec("sleep 1", 2, 10);
  }
  out << "\033[33m[web-search] SearXNG not responding — search may fail\033[0m\n";
}

/**
 * @brief URL-encode a string for use in query parameters.
 * Spaces become '+', alphanumerics and -_.~ pass through, rest is %XX.
 */
static std::string url_encode(const std::string& input) {
  std::string out;
  for (char c : input) {
    if (c == ' ') {
      out += '+';
    } else if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
      out += buf;
    }
  }
  return out;
}

/**
 * @brief Search the web via a SearXNG-compatible JSON API (ADR-057).
 *
 * Calls {search_url}/search?q=...&format=json and parses the results array.
 * Any service returning SearXNG-compatible JSON works as a drop-in backend.
 */
static std::string web_search(const std::string& query, const Config& cfg) {
  bool trace = Config::instance().trace;
  // Build SearXNG JSON API URL
  std::string url = cfg.search_url + "/search?q=" + url_encode(query) + "&format=json&language=" + url_encode(cfg.search_lang);
  if (!cfg.search_location.empty()) {
    url += "&location=" + url_encode(cfg.search_location);
  }

  if (trace) {
    stderr_trace->log("[TRACE] web-search: GET %s\n", url.c_str());
  }
  std::string cmd = "curl -sfL '" + url + "'";
  auto result = cmd_exec(cmd, 10, 200000);
  if (result.exit_code != 0 || result.output.empty()) {
    if (trace) {
      stderr_trace->log("[TRACE] web-search: curl failed exit=%d output=%s\n", result.exit_code, result.output.c_str());
    }
    return "[web search failed]";
  }
  if (trace) {
    stderr_trace->log("[TRACE] web-search: response %zu bytes\n", result.output.size());
  }

  // Parse results array — walk through JSON objects in "results":[...]
  const std::string& body = result.output;
  auto arr_key = body.find("\"results\"");
  if (arr_key == std::string::npos) {
    return "[web search: no results for '" + query + "']";
  }
  // Find opening bracket of the array
  auto arr_start = body.find('[', arr_key);
  if (arr_start == std::string::npos) {
    return "[web search: no results for '" + query + "']";
  }

  std::string out;
  int count = 0;
  size_t pos = arr_start + 1;
  // Extract up to 5 result objects from the array
  while (count < 5 && pos < body.size()) {
    auto obj_start = body.find('{', pos);
    if (obj_start == std::string::npos) {
      break;
    }
    // Use the same brace-tracking logic as json_extract_object
    std::string obj = json_extract_object_at(body, obj_start);
    if (obj.empty()) {
      break;
    }
    pos = obj_start + obj.size();

    std::string title = json_extract_string(obj, "title");
    std::string content = json_extract_string(obj, "content");
    std::string link = json_extract_string(obj, "url");
    if (!content.empty()) {
      out += "- " + title + ": " + content;
      if (!link.empty()) {
        out += " (" + link + ")";
      }
      out += "\n";
      ++count;
    }
  }
  if (out.empty()) {
    return "[web search: no results for '" + query + "']";
  }
  if (trace) {
    stderr_trace->log("[TRACE] web-search: %d results for '%s'\n", count, query.c_str());
  }
  return "[web search results for '" + query + "']\n" + out;
}

// pmccabe:skip-complexity
// NOLINTNEXTLINE(readability-function-size)
static bool handle_response(const std::string& response, ReplState& s) {
  auto writes = parse_write_annotations(response);
  auto str_replaces = parse_str_replace_annotations(response);
  auto reads = parse_read_annotations(response);
  auto execs = parse_exec_annotations(response);
  auto searches = parse_search_annotations(response);

  bool has_annotations = !writes.empty() || !str_replaces.empty() || !reads.empty() || !execs.empty() || !searches.empty();
  if (!has_annotations) {
    if (!s.stream_chat) {
      s.out << colorize_ai(tui::render_markdown(response, s.color && s.markdown), s) << "\n";
    }
    return false;
  }

  // Strip all annotations and display clean text
  if (!s.stream_chat) {
    s.out << colorize_ai(tui::render_markdown(strip_exec_annotations(strip_annotations(response)), s.color && s.markdown), s) << "\n";
  }

  for (const auto& action : writes) {
    process_write(action, s.in, s.out, s.color, s.trust);
  }
  for (const auto& action : str_replaces) {
    process_str_replace(action, s.in, s.out, s.color, s.trust);
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
    std::string output = confirm_exec(cmd, s.cfg, s.in, s.out, s.trust);
    if (!output.empty()) {
      s.history.push_back({"user", "[command: " + cmd + "]\n" + output});
      has_followup = true;
    }
  }
  // Web search: only when enabled in config (ADR-057, privacy-first)
  if (s.cfg.allow_web_search) {
    for (const auto& action : searches) {
      std::string result = web_search(action.query, s.cfg);
      s.out << "\033[1;37m[searching: " << action.query << "]\033[0m\n";
      LOG_EVENT("repl", "web_search", action.query, result, 0, 0, 0);
      s.history.push_back({"user", result});
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
  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] exec: %s\n", cmd.c_str());
  }
  auto t0 = std::chrono::steady_clock::now();
  auto r = cmd_exec(cmd, s.cfg.exec_timeout, s.cfg.max_output);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] exec: exit=%d %ldms output=%zu bytes\n", r.exit_code, ms, r.output.size());
  }

  // Log user-initiated exec (! or !!) with duration for performance analysis
  LOG_EVENT("repl", add_to_history ? "exec_context" : "exec", cmd, r.output, static_cast<int>(ms), 0, 0);
  tui::cmd_output(s.out, s.color, r.output);
  if (r.exit_code != 0 && !r.timed_out) {
    tui::error(s.out, s.color, "[exit code: " + std::to_string(r.exit_code) + "]");
  }
  if (add_to_history) {
    s.history.push_back({"user", "[command: " + cmd + "]\n" + r.output});
  }
}

/** Run chat on a thread, interruptible by Ctrl+C.
 * Polls g_interrupted every 50ms. If interrupted, detaches the HTTP thread
 * so the user gets their prompt back immediately.
 *
 * Thread safety: the thread receives a shared_ptr copy of the conversation
 * history, so detach is safe — no dangling reference to s.history.
 * Without this copy, Ctrl+C followed by /model or a new prompt caused a
 * segfault (use-after-free on s.history). */
static std::string interruptible_chat(ReplState& s) {
  auto result = std::make_shared<std::string>();
  auto done = std::make_shared<std::atomic<bool>>(false);

  if (s.stream_chat) {
    // Streaming: spinner runs until first token, then live output
    auto first_token = std::make_shared<std::atomic<bool>>(false);
    auto renderer = std::make_shared<StreamRenderer>(s.out, s.color && s.markdown);
    Spinner spin(s.out, s.interactive, s.bofh ? tui::bofh_messages() : tui::default_messages());
    // Copy history so the thread owns its data — safe to detach after interrupt
    auto history_copy = std::make_shared<std::vector<Message>>(s.history);
    std::thread t([&s, result, done, first_token, &spin, renderer, history_copy] {
      *result = s.stream_chat(*history_copy, [first_token, &spin, renderer](const std::string& token) {
        if (g_interrupted) {
          return false;
        }
        if (!first_token->exchange(true)) {
          spin.stop();
        }
        renderer->write(token);
        return true;
      });
      renderer->finish();
      *done = true;
    });
    while (!*done && !g_interrupted) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    spin.stop();
    if (*done) {
      t.join();
    } else {
      // Safe to detach: thread owns history_copy, result, done, renderer
      t.detach();
    }
    // cppcheck-suppress knownConditionTrueFalse
    if (!g_interrupted && !result->empty()) {
      s.out << "\n";
    }
    return g_interrupted ? "" : *result;
  }

  // Buffered mode (mock/fallback): use spinner
  auto history_copy2 = std::make_shared<std::vector<Message>>(s.history);
  std::thread t([&s, result, done, history_copy2] {
    *result = s.chat(*history_copy2);
    *done = true;
  });
  while (!*done && !g_interrupted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (*done) {
    t.join();
  } else {
    t.detach();
  }
  return g_interrupted ? "" : *result;
}

/** Call LLM with spinner, interruptible by Ctrl+C.
 * Installs SIGINT handler, starts spinner, calls chat on a thread.
 * Returns response text, or empty string if interrupted. */
static std::string chat_with_spinner(ReplState& s) {
  g_interrupted = 0;
  auto prev = std::signal(SIGINT, sigint_handler);
  // Streaming mode: spinner managed inside interruptible_chat
  // Buffered mode: spinner shows activity while waiting
  Spinner spin(s.out, s.interactive && !s.stream_chat, s.bofh ? tui::bofh_messages() : tui::default_messages());
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
 * Loops follow-ups: as long as the model produces annotations (exec, read, write),
 * the output is fed back and the model gets another turn. This prevents the user
 * from having to type "ok, en nu?" after every exec.
 *
 * After iteration 2, injects a reminder system message to prevent model drift
 * (hallucination, flattery). See ADR-054.
 *
 * @param line The user input line to send as a prompt.
 * @param s REPL state containing chat, configuration, I/O streams, and conversation history.
 */
static constexpr const char* reminder_nudge =
    "Reminder: be concise, only state facts about code you have read in this "
    "session, no scores without criteria.";

static void send_prompt(const std::string& line, ReplState& s) {
  // Trace output for debugging loop behavior (ADR-028)
  if (Config::instance().trace) {
    stderr_trace->log("[TRACE] iteration=%d prompt=%.50s\n", s.count, line.c_str());
  }

  // Inject a short reminder after iteration 2 to prevent model drift (ADR-054)
  bool inserted_reminder = false;
  if (s.count >= 2) {
    s.history.push_back({"system", reminder_nudge});
    inserted_reminder = true;
  }
  s.history.push_back({"user", line});

  // Time the LLM call so user sees how long it took
  auto t0 = std::chrono::steady_clock::now();
  std::string response = chat_with_spinner(s);
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

  if (g_interrupted) {
    s.out << "\n[interrupted]\n";
    s.history.pop_back();
    if (inserted_reminder) {
      s.history.pop_back();
    }
    g_interrupted = 0;
    return;
  }
  // Auto-repair malformed closing tags before parsing (e.g. <exec>...</bash>)
  response = fix_malformed_tags(response);

  s.history.push_back({"assistant", response});
  s.last_assistant_idx = static_cast<int>(s.history.size()) - 1;
  bool needs_followup = handle_response(response, s);
  s.count++;

  // Show response stats: duration, model, response length
  if (s.color) {
    s.out << "\033[2m";  // dim
  }
  if (elapsed >= 1000) {
    s.out << "[" << (elapsed / 1000) << "." << ((elapsed % 1000) / 100) << "s";
  } else {
    s.out << "[" << elapsed << "ms";
  }
  s.out << " · " << Config::instance().model;
  s.out << " · " << response.size() << " chars]";
  if (s.color) {
    s.out << "\033[0m";
  }
  s.out << "\n";

  // Keep following up while the model produces annotations (exec, read, etc.)
  // Bounded to prevent runaway turns burning tokens/time.
  constexpr int k_max_followups = 8;
  int followup_count = 0;
  while (needs_followup && followup_count < k_max_followups) {
    std::string followup = chat_with_spinner(s);
    if (g_interrupted) {
      s.out << "\n[interrupted]\n";
      g_interrupted = 0;
      break;
    }
    s.history.push_back({"assistant", followup});
    s.last_assistant_idx = static_cast<int>(s.history.size()) - 1;
    needs_followup = handle_response(followup, s);
    ++followup_count;
  }
  if (needs_followup) {
    s.out << "[follow-up limit reached]\n";
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
  static const std::vector<std::string> cmds = {"/c",     "/clear", "/color", "/copy", "/mem",   "/model", "/p",
                                                "/paste", "/pref",  "/rate",  "/scan", "/set",   "/version", "/help"};
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
             HardwareFn hw_fn, ModelInfoFn model_info_fn, ScanFn scan_fn) {
  std::string line;
  std::vector<Message> history;
  if (!cfg.system_prompt.empty()) {
    std::string sys = cfg.system_prompt;
    // Append web search tool description when enabled (ADR-057)
    if (cfg.allow_web_search) {
      sys += Config::web_search_prompt;
    }
    // Load persistent preferences and memory into system prompt (ADR-059)
    std::string prefs = read_file_or_empty(cfg.preferences_path);
    if (!prefs.empty()) {
      sys += "\n\nUser preferences:\n" + prefs;
    }
    std::string mem = read_file_or_empty(cfg.memory_path);
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
                     color_name_to_ansi(cfg.ai_color)};

  // Log session start with version, commit, model, and host for traceability
  std::string version_info = std::string(LLAMA_CLI_VERSION) + " (" + GIT_COMMIT + ")";
  LOG_EVENT("repl", "session_start", cfg.model, version_info + " " + cfg.host + ":" + cfg.port, 0, 0, 0);

  // Auto-start SearXNG when web search is enabled (ADR-057)
  if (cfg.allow_web_search) {
    ensure_searxng(cfg, out);
  }

  while (read_line(in, out, line, state.color, state.prompt_color)) {
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
