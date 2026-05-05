/**
 * @file repl_commands.cpp
 * @brief Slash command handlers for the REPL (extracted from repl.cpp).
 *
 * SRP: This file handles all /slash commands. repl.cpp only handles the loop.
 *
 * @see docs/adr/adr-066-solid-refactoring.md
 */

#include "repl/repl_commands.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>

#include "agent/agent.h"
#include "help.h"
#include "logging/logger.h"
#include "net/scan.h"
#include "planner/planner.h"
#include "provider/provider_factory.h"
#include "repl/repl_model.h"
#include "tui/tui.h"
#include "util/util.h"

#ifdef LINENOISE_HPP
// already included
#else
#include "linenoise.hpp"
#endif

/// @cond
#ifndef LLAMA_CLI_VERSION
#define LLAMA_CLI_VERSION "0.0.0-dev"
#endif
#ifndef BUILD_TIMEZONE
#define BUILD_TIMEZONE "UTC"
#endif
/// @endcond

// --- Helpers (file-local) ---
// These utilities support the command handlers but are not exposed.
// save_to_dotenv: persists runtime changes (color, hosts) across sessions.
// ensure_parent_dir/append_line: file I/O for /mem and /pref (ADR-059).
// read_file_or_empty: safe file read that returns "" on missing files.

/// Get version string from compile-time definition.
static std::string get_version() {
  std::string ver = LLAMA_CLI_VERSION;
  ver += " (built " __DATE__ " " __TIME__ " " BUILD_TIMEZONE ")";
  return ver;
}

/// Show current options state — lists all toggleable runtime settings.
static void show_options(ReplState& s) {
  auto status = [&](bool val) {
    if (!s.color) {
      return val ? "on" : "off";
    }
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

/// Toggle a named option, return true if recognized.
/// Uses a lookup table mapping option names to ReplState bool fields.
/// Trace is special-cased because it lives in the global Config singleton
/// (needs to be visible to ollama.cpp for HTTP tracing).
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

/// Save or update a key=value in .env file.
/// Reads existing lines, replaces matching key or appends new one.
/// Used by /color and /scan to persist settings across sessions.
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

/// Ensure parent directory exists for a file path (ADR-059)
static void ensure_parent_dir(const std::string& path) {
  auto slash = path.rfind('/');
  if (slash != std::string::npos) {
    std::string dir = path.substr(0, slash);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    mkdir(dir.c_str(), 0750);
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

// --- Command handlers ---
// Each handler implements one /slash command.
// They take ReplState& for I/O and session access.
// Return values: void (output written to s.out).

/// Handle /set command: show options or toggle one.
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

/// Handle /color command: /color prompt <name>, /color ai <name>
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

/// Handle /scan — discover Ollama servers on the local network.
/// Scans the local subnet for port 11434, writes results to .env as OLLAMA_HOSTS.
/// Updates runtime Config::instance().hosts so /model shows all discovered servers.
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
    if (i > 0) {
      hosts_str += ",";
    }
    hosts_str += hosts[i];
  }
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
  Config::instance().hosts = hosts;
}

/// Handle /mem — view or modify persistent memory (ADR-059).
/// Memory is a plain-text file with bullet points, loaded into system prompt.
/// Subcommands: (none) = show, add <fact> = append, clear = wipe.
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

/// Handle /pref command: show, add, or clear preferences (ADR-059).
/// Preferences are injected into the system prompt at session start.
/// Subcommands: (none) = show, add <pref> = append, clear = wipe.
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

/// Handle /rate command: rate last or specific response.
/// Ratings are stored in-memory on the Message struct and logged.
/// /rate list shows all rated responses with a preview.
static void handle_rate(const std::string& arg, ReplState& s) {
  // /rate list — show all rated responses with preview
  // /rate <value> — rate last response, /rate <n> <value> — rate specific response
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

  std::string index_str, rating_str;
  auto space = arg.find(' ');
  if (space == std::string::npos) {
    s.out << "Usage: /rate last +/-, /rate <n> +/-, /rate list\n";
    return;
  }
  index_str = arg.substr(0, space);
  rating_str = arg.substr(space + 1);

  int target_idx = -1;
  if (index_str == "last") {
    target_idx = s.last_assistant_idx;
  } else {
    try {
      int n = std::stoi(index_str);
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
  LOG_EVENT("repl", "rate_response", s.history[target_idx].content, rating, 0, 0, 0);
}

// --- Public dispatch function ---
// OCP: new commands are added as new handler functions, then a single
// else-if branch here. The dispatch table pattern keeps it extensible.

/// OCP: dispatch routes to handlers. New commands = new handler functions.
/// Returns true if the command was recognized, false for unknown commands.
/// Each branch delegates to a focused handler function (SRP).
bool dispatch_command(const std::string& command, const std::string& arg, ReplState& s) {
  if (command == "clear") {
    s.history.clear();
    s.out << "[history cleared]\n";
  } else if (command == "chat") {
    // /chat save <name>, /chat load <name>, /chat list, /chat delete <name>
    std::string sub, name;
    auto space = arg.find(' ');
    if (space != std::string::npos) {
      sub = arg.substr(0, space);
      name = arg.substr(space + 1);
    } else {
      sub = arg;
    }
    // Chat save directory
    std::string chat_dir;
    struct stat st;
    if (stat("Makefile", &st) == 0) {
      chat_dir = ".tmp/chats/";
    } else {
      const char* home = std::getenv("HOME");
      chat_dir = std::string(home ? home : ".") + "/.llama-cli/chats/";
    }
    mkdir(chat_dir.c_str(), 0750);

    if (sub == "save" && !name.empty()) {
      std::ofstream f(chat_dir + name + ".json");
      if (f) {
        f << "[\n";
        for (size_t i = 0; i < s.history.size(); i++) {
          f << "  {\"role\":\"" << s.history[i].role << "\",\"content\":\"";
          // Escape content for JSON
          for (char c : s.history[i].content) {
            if (c == '"') {
              f << "\\\"";
            } else if (c == '\\') {
              f << "\\\\";
            } else if (c == '\n') {
              f << "\\n";
            } else {
              f << c;
            }
          }
          f << "\"}";
          if (i + 1 < s.history.size()) {
            f << ",";
          }
          f << "\n";
        }
        f << "]\n";
        s.out << "[chat saved: " << name << " (" << s.history.size() << " messages)]\n";
      } else {
        s.out << "[error: cannot write to " << chat_dir << name << ".json]\n";
      }
    } else if (sub == "load" && !name.empty()) {
      std::ifstream f(chat_dir + name + ".json");
      if (f) {
        std::string json((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        // Simple JSON array parser for our known format
        s.history.clear();
        size_t pos = 0;
        while ((pos = json.find("\"role\":\"", pos)) != std::string::npos) {
          pos += 8;
          size_t end = json.find("\"", pos);
          std::string role = json.substr(pos, end - pos);
          pos = json.find("\"content\":\"", pos);
          if (pos == std::string::npos) {
            break;
          }
          pos += 11;
          // Parse escaped content
          std::string content;
          while (pos < json.size() && !(json[pos] == '"' && json[pos - 1] != '\\')) {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
              if (json[pos + 1] == 'n') {
                content += '\n';
                pos += 2;
              } else if (json[pos + 1] == '"') {
                content += '"';
                pos += 2;
              } else if (json[pos + 1] == '\\') {
                content += '\\';
                pos += 2;
              } else {
                content += json[pos++];
              }
            } else {
              content += json[pos++];
            }
          }
          s.history.push_back({role, content});
        }
        s.out << "[chat loaded: " << name << " (" << s.history.size() << " messages)]\n";
      } else {
        s.out << "[chat not found: " << name << "]\n";
      }
    } else if (sub == "list") {
      DIR* dir = opendir(chat_dir.c_str());
      if (dir) {
        const struct dirent* entry = nullptr;
        int count = 0;
        while ((entry = readdir(dir)) != nullptr) {
          std::string fname(entry->d_name);
          if (fname.size() > 5 && fname.substr(fname.size() - 5) == ".json") {
            s.out << "  " << fname.substr(0, fname.size() - 5) << "\n";
            count++;
          }
        }
        closedir(dir);
        if (count == 0) {
          s.out << "  (no saved chats)\n";
        }
      }
    } else if (sub == "delete" && !name.empty()) {
      std::string path = chat_dir + name + ".json";
      if (std::remove(path.c_str()) == 0) {
        s.out << "[chat deleted: " << name << "]\n";
      } else {
        s.out << "[chat not found: " << name << "]\n";
      }
    } else {
      s.out << "Usage: /chat save <name>, /chat load <name>, /chat list, /chat delete <name>\n";
    }
  } else if (command == "set" || command == "options") {
    handle_set(arg, s);
  } else if (command == "color") {
    handle_color(arg, s);
  } else if (command == "model") {
    handle_model_selection(s, arg);
  } else if (command == "version") {
    s.out << "llama-cli " << get_version() << "\n";
  } else if (command == "mem") {
    handle_mem(arg, s);
  } else if (command == "pref") {
    handle_pref(arg, s);
  } else if (command == "rate") {
    handle_rate(arg, s);
  } else if (command == "copy" || command == "c") {
    if (s.last_assistant_idx < 0 || s.last_assistant_idx >= static_cast<int>(s.history.size())) {
      s.out << "[no response to copy]\n";
    } else {
      const std::string& content = s.history[s.last_assistant_idx].content;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
      FILE* pipe = popen("pbcopy 2>/dev/null || xclip -selection clipboard 2>/dev/null", "w");
      if (pipe) {
        fwrite(content.c_str(), 1, content.size(), pipe);
        pclose(pipe);
        s.out << "[copied " << content.size() << " chars to clipboard]\n";
      } else {
        s.out << "[clipboard not available — install pbcopy or xclip]\n";
      }
    }
  } else if (command == "paste" || command == "p") {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    FILE* pipe = popen("pbpaste 2>/dev/null || xclip -selection clipboard -o 2>/dev/null", "r");
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
  } else if (command == "help" || command.empty()) {
    s.out << help::repl;
  } else if (command == "scan") {
    handle_scan(s);
  } else if (command == "provider") {
    if (arg.empty()) {
      s.out << "[current provider: " << Config::instance().provider << "]\n";
      s.out << "Available: ollama, tgpt, gemini\n";
      s.out << "Usage: /provider <name>\n";
    } else if (arg == "ollama" || arg == "tgpt" || arg == "gemini") {
      if (s.switch_provider) {
        s.switch_provider(arg);
        s.out << "[switched to provider: " << arg << "]\n";
      } else {
        s.out << "[provider switching not available]\n";
      }
    } else {
      s.out << "Unknown provider: " << arg << ". Available: ollama, tgpt, gemini\n";
    }
  } else if (command == "auto") {
    s.auto_route = !s.auto_route;
    s.out << "[auto routing: " << (s.auto_route ? "on" : "off") << "]\n";
    if (s.auto_route) {
      s.out << "Prompts will be routed by complexity: simple→3B, medium→14B, complex→27B+\n";
    }
  } else if (command == "image") {
    if (arg.empty()) {
      s.out << "Usage: /image <path> — attach image for next prompt (vision models)\n";
    } else {
      std::ifstream f(arg, std::ios::binary);
      if (!f) {
        s.out << "[error: cannot read " << arg << "]\n";
      } else {
        // Read file and base64 encode
        std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded;
        int val = 0;
        int bits = -6;
        for (unsigned char c : data) {
          val = (val << 8) + c;
          bits += 8;
          while (bits >= 0) {
            encoded += b64[(val >> bits) & 0x3F];
            bits -= 6;
          }
        }
        if (bits > -6) {
          encoded += b64[((val << 8) >> (bits + 8)) & 0x3F];
        }
        while (encoded.size() % 4) {
          encoded += '=';
        }
        // Store image in a pending message that will be attached to next user prompt
        s.history.push_back({"user", "[image: " + arg + "]", "", {encoded}});
        s.out << "[image attached: " << arg << " (" << data.size() / 1024 << "KB) — ask about it now]\n";
      }
    }
  } else if (command == "agent") {
    static auto personas = load_personas();
    if (arg.empty() || arg == "list") {
      s.out << "Available agents:\n";
      for (const auto& p : personas) {
        s.out << "  " << p.name << " — " << p.description << "\n";
      }
      s.out << "Usage: /agent <name> or /agent off\n";
    } else if (arg == "off" || arg == "reset") {
      // Restore original system prompt (first message in history)
      if (!s.history.empty() && s.history[0].role == "system") {
        s.history[0].content = s.cfg.system_prompt;
      }
      s.out << "[agent disabled — default persona restored]\n";
    } else {
      const AgentPersona* p = find_persona(personas, arg);
      if (p) {
        // Override system prompt with persona
        if (!s.history.empty() && s.history[0].role == "system") {
          s.history[0].content = p->system_prompt;
        } else {
          s.history.insert(s.history.begin(), {"system", p->system_prompt});
        }
        s.out << "[agent: " << p->name << " — " << p->description << "]\n";
      } else {
        s.out << "Unknown agent: " << arg << ". Type /agent list to see options.\n";
      }
    }
  } else if (command == "nick") {
    if (arg.empty()) {
      s.out << "Usage: /nick <name> — set your display name\n";
      s.out << "Current: " << Config::instance().nick << "\n";
    } else {
      Config::instance().nick = arg;
      s.out << "[nick set to: " << arg << "]\n";
    }
  } else if (command == "usage") {
    // Show token usage from event log
    int total_prompt = 0;
    int total_completion = 0;
    int calls = 0;
    for (const auto& msg : s.history) {
      if (msg.role == "assistant") {
        calls++;
        total_completion += static_cast<int>(msg.content.size()) / 4;  // rough estimate
      } else if (msg.role == "user") {
        total_prompt += static_cast<int>(msg.content.size()) / 4;
      }
    }
    s.out << "Session usage (estimated):\n";
    s.out << "  Messages: " << s.history.size() << "\n";
    s.out << "  Prompt tokens: ~" << total_prompt << "\n";
    s.out << "  Completion tokens: ~" << total_completion << "\n";
    s.out << "  LLM calls: " << calls << "\n";
    s.out << "  Provider: " << Config::instance().provider << "\n";
    s.out << "  Model: " << Config::instance().model << "\n";
    s.out << "  Host: " << Config::instance().host << ":" << Config::instance().port << "\n";
  } else if (command == "theme") {
    if (arg.empty()) {
      s.out << "Usage: /theme <name> — set color theme\n";
      s.out << "Available: dark, light, mono, hacker\n";
      s.out << "Current prompt_color: " << s.prompt_color << ", ai_color: " << s.ai_color << "\n";
    } else if (arg == "dark") {
      s.prompt_color = "32";
      s.ai_color = "";
      s.out << "[theme: dark (green prompt, default AI)]\n";
    } else if (arg == "light") {
      s.prompt_color = "34";
      s.ai_color = "90";
      s.out << "[theme: light (blue prompt, gray AI)]\n";
    } else if (arg == "mono") {
      s.prompt_color = "";
      s.ai_color = "";
      s.color = false;
      s.out << "[theme: mono (no colors)]\n";
    } else if (arg == "hacker") {
      s.prompt_color = "92";
      s.ai_color = "32";
      s.out << "[theme: hacker (bright green)]\n";
    } else {
      s.out << "Unknown theme: " << arg << ". Available: dark, light, mono, hacker\n";
    }
  } else if (command == "compress") {
    // Compress history: keep system prompt + generate summary of conversation
    if (s.history.size() <= 2) {
      s.out << "[nothing to compress — conversation too short]\n";
    } else {
      // Build summary from conversation
      std::string summary = "Previous conversation summary: ";
      int msg_count = 0;
      for (const auto& msg : s.history) {
        if (msg.role == "system") {
          continue;
        }
        if (msg.role == "user") {
          summary += "User asked about: " + msg.content.substr(0, 60);
          if (msg.content.size() > 60) {
            summary += "...";
          }
          summary += ". ";
          msg_count++;
        }
      }
      // Keep system prompt, replace everything else with summary
      std::vector<Message> compressed;
      if (!s.history.empty() && s.history[0].role == "system") {
        compressed.push_back(s.history[0]);
      }
      compressed.push_back({"assistant", summary});
      int removed = static_cast<int>(s.history.size()) - static_cast<int>(compressed.size());
      s.history = compressed;
      s.out << "[compressed: " << removed << " messages → summary (" << msg_count << " topics retained)]\n";
    }
  } else {
    s.out << "Unknown command: /" << command << ". Type /help for options.\n";
  }
  return true;
}
