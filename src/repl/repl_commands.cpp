/**
 * @file repl_commands.cpp
 * @brief Slash command handlers for the REPL (extracted from repl.cpp).
 *
 * SRP: This file handles all /slash commands. repl.cpp only handles the loop.
 *
 * Command categories:
 *   - History: /clear, /chat save|load|list|delete
 *   - Config: /set, /color, /theme
 *   - Model: /model, /provider, /auto
 *   - Session: /nick, /usage, /compress
 *   - Memory: /mem, /pref, /rate
 *   - Tools: /copy, /paste, /image, /scan
 *   - Info: /help, /version
 *
 * @see docs/adr/adr-066-solid-refactoring.md
 */

#include "repl/repl_commands.h"

#include <dirent.h>
#include <sys/ioctl.h>
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
#include "orchestrator/agent_config.h"
#include "orchestrator/prompt_template.h"
#include "orchestrator/task.h"
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
/// Get version string: "0.20.0 (ba58807 dirty)" or "0.20.0 (ba58807)"
static std::string get_version() {
  std::string ver = LLAMA_CLI_VERSION;
  ver += " (" GIT_COMMIT GIT_DIRTY ")";
  return ver;
}

/// Show current options state — lists all toggleable runtime settings.
static void show_options(ReplState& s) {
  auto status = [&](bool val) {
    if (!s.color) {
      return std::string(val ? "on" : "off");
    }
    return val ? tui::active_theme().info.ansi() + "on" + ThemeStyle::reset()
               : tui::active_theme().error.ansi() + "off" + ThemeStyle::reset();
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
/// Main command dispatcher — routes /slash commands to handlers.
/// Each branch delegates to a focused handler function (SRP).
bool dispatch_command(const std::string& command, const std::string& arg, ReplState& s) {
  // --- History management ---
  if (command == "clear") {
    LOG_FEATURE("cmd_clear");
    s.history.clear();
    s.out << "[history cleared]\n";
  } else if (command == "chat") {
    LOG_FEATURE("cmd_chat");
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
    // --- Configuration commands ---
  } else if (command == "set" || command == "options") {
    LOG_FEATURE("cmd_set");
    handle_set(arg, s);
  } else if (command == "color") {
    LOG_FEATURE("cmd_color");
    handle_color(arg, s);
    // --- Model and provider management ---
  } else if (command == "model") {
    LOG_FEATURE("cmd_model");
    handle_model_selection(s, arg);
  } else if (command == "version") {
    LOG_FEATURE("cmd_version");
    s.out << "llama-cli " << get_version() << "\n";
  } else if (command == "mem") {
    LOG_FEATURE("cmd_mem");
    handle_mem(arg, s);
  } else if (command == "pref") {
    LOG_FEATURE("cmd_pref");
    handle_pref(arg, s);
  } else if (command == "rate") {
    LOG_FEATURE("cmd_rate");
    handle_rate(arg, s);
    // --- Clipboard: copy last response or paste from system clipboard ---
  } else if (command == "copy" || command == "c") {
    LOG_FEATURE("cmd_copy");
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
    LOG_FEATURE("cmd_paste");
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
    LOG_FEATURE("cmd_help");
    // Paginate help output based on terminal height (same pattern as /model)
    std::string text = help::repl;
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
      lines.push_back(line);
    }
    int term_rows = 24;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
      term_rows = ws.ws_row;
    }
    int page_size = term_rows - 3;
    if (page_size < 5) page_size = 5;
    for (size_t i = 0; i < lines.size(); i++) {
      s.out << lines[i] << "\n";
      if (static_cast<int>(lines.size()) > page_size && (i + 1) % static_cast<size_t>(page_size) == 0 && i + 1 < lines.size()) {
        s.out << "  -- more (" << (i + 1) << "/" << lines.size() << ") press Enter --" << std::flush;
        std::string dummy;
        if (&s.in == &std::cin) {
          linenoise::Readline("", dummy);
        } else {
          std::getline(s.in, dummy);
        }
      }
    }
  } else if (command == "scan") {
    LOG_FEATURE("cmd_scan");
    handle_scan(s);
    // --- Provider switching and discovery ---
  } else if (command == "provider") {
    if (arg.empty()) {
      LOG_FEATURE("provider_list");
      s.out << "[current: " << Config::instance().provider << "]\n";
      // Show numbered list from registry (ADR-081)
      if (s.registry && !s.registry->models.empty()) {
        auto provs = s.registry->providers();
        for (size_t i = 0; i < provs.size(); i++) {
          int count = 0;
          for (const auto& m : s.registry->models) {
            if (m.provider == provs[i]) {
              count++;
            }
          }
          std::string marker = (provs[i] == Config::instance().provider) ? "*" : " ";
          s.out << marker << " " << (i + 1) << ". " << provs[i] << " (" << count << " model" << (count > 1 ? "s" : "") << ")\n";
        }
        s.out << "Select [1-" << provs.size() << "] or /provider <name>: ";
        s.out.flush();
        std::string input;
        if (std::getline(s.in, input) && !input.empty()) {
          try {
            int idx = std::stoi(input) - 1;
            if (idx >= 0 && idx < static_cast<int>(provs.size())) {
              input = provs[idx];
            }
          } catch (...) {
          }
          if (s.switch_provider) {
            s.switch_provider(input);
            s.out << "[switched to " << input << ": " << Config::instance().model << "@" << Config::instance().host << "]\n";
          }
        }
      } else {
        s.out << "Available: ollama, tgpt, gemini, kiro-cli\n";
        s.out << "Usage: /provider <name>\n";
      }
    } else {
      if (s.switch_provider) {
        s.switch_provider(arg);
        s.out << "[switched to " << arg << ": " << Config::instance().model << "@" << Config::instance().host << "]\n";
      } else {
        s.out << "[provider switching not available]\n";
      }
    }
    // --- Host selection: hierarchical navigation (ADR-089) ---
    // /host lists unique hosts from the registry with model counts.
    // Selecting a host switches provider+model to the first available on that host.
    // This enables the Host → Provider → Model drill-down workflow.
  } else if (command == "host") {
    LOG_FEATURE("cmd_host");
    if (!s.registry || s.registry->models.empty()) {
      s.out << "[no registry available — run /scan first]\n";
    } else if (!arg.empty()) {
      // Direct host switch: /host <name>
      bool found = false;
      for (const auto& m : s.registry->models) {
        if (m.host == arg || m.host.find(arg) == 0) {
          if (m.host != "cloud") {
            auto colon = m.host.find(':');
            if (colon != std::string::npos) {
              Config::instance().host = m.host.substr(0, colon);
              Config::instance().port = m.host.substr(colon + 1);
            }
          }
          Config::instance().provider = m.provider;
          Config::instance().model = m.name;
          if (s.switch_provider) {
            s.switch_provider(m.provider);
          }
          s.out << "[host: " << resolve_display_name(m.host) << " → " << m.provider << ":" << m.name << "]\n";
          found = true;
          break;
        }
      }
      if (!found) {
        s.out << "Unknown host: " << arg << "\n";
      }
    } else {
      // List unique hosts with model counts
      std::vector<std::string> hosts;
      std::map<std::string, int> host_counts;
      std::map<std::string, std::string> host_providers;
      for (const auto& m : s.registry->models) {
        if (host_counts.find(m.host) == host_counts.end()) {
          hosts.push_back(m.host);
        }
        host_counts[m.host]++;
        if (host_providers[m.host].empty()) {
          host_providers[m.host] = m.provider;
        } else if (host_providers[m.host].find(m.provider) == std::string::npos) {
          host_providers[m.host] += ", " + m.provider;
        }
      }
      std::string current_host = Config::instance().host + ":" + Config::instance().port;
      s.out << "[current: " << resolve_display_name(current_host) << "]\n";
      for (size_t i = 0; i < hosts.size(); i++) {
        std::string marker = (hosts[i] == current_host || hosts[i].find(Config::instance().host) == 0) ? "*" : " ";
        std::string display = resolve_display_name(hosts[i]);
        s.out << marker << " " << (i + 1) << ". " << display << "  (" << host_providers[hosts[i]] << ", " << host_counts[hosts[i]]
              << " model" << (host_counts[hosts[i]] > 1 ? "s" : "") << ")\n";
      }
      s.out << "Select [1-" << hosts.size() << "] or /host <name>: ";
      s.out.flush();
      std::string input;
      if (std::getline(s.in, input) && !input.empty()) {
        std::string selected_host;
        try {
          int idx = std::stoi(input) - 1;
          if (idx >= 0 && idx < static_cast<int>(hosts.size())) {
            selected_host = hosts[idx];
          }
        } catch (...) {
          selected_host = input;
        }
        if (!selected_host.empty()) {
          // Switch to first model on this host
          for (const auto& m : s.registry->models) {
            if (m.host == selected_host) {
              if (m.host != "cloud") {
                auto colon = m.host.find(':');
                if (colon != std::string::npos) {
                  Config::instance().host = m.host.substr(0, colon);
                  Config::instance().port = m.host.substr(colon + 1);
                }
              }
              Config::instance().provider = m.provider;
              Config::instance().model = m.name;
              if (s.switch_provider) {
                s.switch_provider(m.provider);
              }
              s.out << "[host: " << resolve_display_name(m.host) << " → " << m.provider << ":" << m.name << "]\n";
              break;
            }
          }
        }
      }
    }
    // --- Quick-switch: /use [@host:]provider:model (ADR-089) ---
    // Power-user command for one-liner switching without interactive menus.
    // Supports partial matches: /use claude matches first model containing "claude".
    // The @ prefix denotes a host filter, colon separates provider:model.
  } else if (command == "use") {
    LOG_FEATURE("cmd_use");
    if (arg.empty()) {
      s.out << "Usage: /use <model>                    — switch model\n";
      s.out << "       /use <provider>:<model>         — switch provider + model\n";
      s.out << "       /use @<host>                    — switch host\n";
      s.out << "       /use @<host>:<provider>:<model> — switch everything\n";
    } else if (!s.registry || s.registry->models.empty()) {
      s.out << "[no registry available]\n";
    } else {
      std::string host_part, prov_part, model_part;
      std::string input = arg;
      // Parse @host prefix
      if (!input.empty() && input[0] == '@') {
        input = input.substr(1);
        auto colon = input.find(':');
        if (colon != std::string::npos) {
          host_part = input.substr(0, colon);
          input = input.substr(colon + 1);
        } else {
          host_part = input;
          input.clear();
        }
      }
      // Parse provider:model or just model
      if (!input.empty()) {
        auto colon = input.find(':');
        if (colon != std::string::npos) {
          prov_part = input.substr(0, colon);
          model_part = input.substr(colon + 1);
        } else {
          model_part = input;
        }
      }
      // Find matching entry in registry
      const ModelEntry* match = nullptr;
      for (const auto& m : s.registry->models) {
        bool host_ok = host_part.empty() || m.host.find(host_part) != std::string::npos;
        bool prov_ok = prov_part.empty() || m.provider == prov_part;
        bool model_ok = model_part.empty() || m.name == model_part || m.name.find(model_part) == 0;
        if (host_ok && prov_ok && model_ok && m.available) {
          match = &m;
          break;
        }
      }
      if (match) {
        if (match->host != "cloud") {
          auto colon = match->host.find(':');
          if (colon != std::string::npos) {
            Config::instance().host = match->host.substr(0, colon);
            Config::instance().port = match->host.substr(colon + 1);
          }
        }
        Config::instance().provider = match->provider;
        Config::instance().model = match->name;
        if (s.switch_provider) {
          s.switch_provider(match->provider);
        }
        s.out << "[switched: " << match->name << " on " << match->provider << "@" << match->host << "]\n";
      } else {
        s.out << "No matching model found for: " << arg << "\n";
        s.out << "Try /model to see available options.\n";
      }
    }
    // --- Async delegation: run subtask in background (replaces ADR-079 auto-routing) ---
  } else if (command == "auto") {
    LOG_FEATURE("cmd_auto");
    s.out << "[auto routing removed — LLM can delegate via <delegate> tags, check /tasks]\n";
    // --- Image attachment for vision models ---
  } else if (command == "image") {
    LOG_FEATURE("cmd_image");
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
    // --- Agent persona switching (loads system prompt + temperature) ---
  } else if (command == "agent") {
    LOG_FEATURE("cmd_agent");
    static auto personas = load_personas();
    static auto agent_configs = load_agent_configs();
    if (arg.empty() || arg == "list") {
      s.out << "Available agents:\n";
      for (const auto& a : agent_configs) {
        std::string mode_str = (a.mode == AgentMode::Primary) ? "primary" : "subagent";
        s.out << "  " << a.name << " — " << a.description << " [" << mode_str << "]\n";
      }
      if (!personas.empty()) {
        s.out << "Personas:\n";
        for (const auto& p : personas) {
          s.out << "  " << p.name << " — " << p.description << "\n";
        }
      }
      s.out << "Active: " << active_agent_name() << "\n";
    } else if (arg == "off" || arg == "reset") {
      if (!s.history.empty() && s.history[0].role == "system") {
        s.history[0].content = s.cfg.system_prompt;
      }
      set_active_agent("build");
      s.out << "[agent: build — default restored]\n";
    } else {
      const auto* ac = find_agent_config(agent_configs, arg);
      if (ac) {
        set_active_agent(ac->name);
        std::string prompt = load_prompt(ac->prompt_file.empty() ? ac->name : ac->prompt_file.substr(0, ac->prompt_file.find('.')));
        if (!prompt.empty()) {
          if (!s.history.empty() && s.history[0].role == "system") {
            s.history[0].content = prompt;
          } else {
            s.history.insert(s.history.begin(), {"system", prompt});
          }
        }
        s.out << "[agent: " << ac->name << " — " << ac->description << "]\n";
      } else {
        const AgentPersona* p = find_persona(personas, arg);
        if (p) {
          set_active_agent("build");
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
    }
    // --- User identity and session info ---
  } else if (command == "nick") {
    LOG_FEATURE("cmd_nick");
    if (arg.empty()) {
      s.out << "Usage: /nick <name> — set your display name\n";
      s.out << "Current: " << Config::instance().nick << "\n";
    } else {
      Config::instance().nick = arg;
      s.out << "[nick set to: " << arg << "]\n";
    }
  } else if (command == "usage") {
    LOG_FEATURE("cmd_usage");
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
    // --- Background tasks: check async delegation results (ADR-099) ---
  } else if (command == "tasks") {
    LOG_FEATURE("cmd_tasks");
    if (s.pending_tasks.empty()) {
      s.out << "[no background tasks]\n";
    } else {
      int done = 0;
      int running = 0;
      for (auto it = s.pending_tasks.begin(); it != s.pending_tasks.end();) {
        auto status = it->result.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
          // Task completed — inject result into conversation
          try {
            std::string result = it->result.get();
            s.out << tui::active_theme().info.ansi() << "[done: " << it->model << "] " << ThemeStyle::reset();
            s.out << it->prompt.substr(0, 60) << "\n";
            s.out << tui::render_markdown(result, s.color && s.markdown) << "\n";
            s.history.emplace_back(Message{"user", "[background task result from " + it->model + "]\n" + result});
          } catch (const std::exception& e) {
            s.out << tui::active_theme().error.ansi() << "[failed: " << it->model << "] " << ThemeStyle::reset();
            s.out << e.what() << "\n";
          }
          it = s.pending_tasks.erase(it);
          done++;
        } else {
          s.out << tui::active_theme().warning.ansi() << "[running: " << it->model << "] " << ThemeStyle::reset();
          s.out << it->prompt.substr(0, 60) << "...\n";
          ++it;
          running++;
        }
      }
      if (done > 0) {
        s.out << done << " task(s) completed, added to context.\n";
      }
      if (running > 0) {
        s.out << running << " task(s) still running.\n";
      }
    }
    // --- Theme switching: /theme <name> or /theme set <role> <color> (ADR-080) ---
  } else if (command == "theme") {
    LOG_FEATURE("cmd_theme");
    if (arg.empty()) {
      s.out << "Current theme: " << tui::active_theme().name << "\n";
      s.out << "Available: dark, light, mono, hacker\n";
      s.out << "Usage: /theme <name>          — switch theme\n";
      s.out << "       /theme set <role> <options> — customize a role\n";
      s.out << "Roles: prompt, ai, system, error, info, warning, banner, code\n";
      s.out << "Options: <color> [bold] [italic] [underline] [dim] [bg:<color>]\n";
      s.out << "Colors: red green blue cyan yellow magenta white black bright_*\n";
    } else if (arg == "dark" || arg == "light" || arg == "mono" || arg == "hacker") {
      tui::active_theme() = get_theme(arg);
      s.color = (arg != "mono");
      s.out << "[theme: " << arg << "]\n";
    } else if (arg.substr(0, 4) == "set ") {
      // Parse: /theme set <role> <color> [bold] [italic] [underline] [dim] [bg:<color>]
      std::istringstream iss(arg.substr(4));
      std::string role, token;
      iss >> role;
      ThemeStyle style;
      while (iss >> token) {
        if (token == "bold") {
          style.bold = true;
        } else if (token == "italic") {
          style.italic = true;
        } else if (token == "underline") {
          style.underline = true;
        } else if (token == "dim") {
          style.dim = true;
        } else if (token.substr(0, 3) == "bg:") {
          style.bg = token.substr(3);
        } else {
          style.fg = token;
        }
      }
      // Apply to the right role
      Theme& t = tui::active_theme();
      if (role == "prompt") {
        t.prompt = style;
      } else if (role == "ai") {
        t.ai = style;
      } else if (role == "system") {
        t.system = style;
      } else if (role == "error") {
        t.error = style;
      } else if (role == "info") {
        t.info = style;
      } else if (role == "warning") {
        t.warning = style;
      } else if (role == "banner") {
        t.banner = style;
      } else if (role == "code") {
        t.code = style;
      } else {
        s.out << "Unknown role: " << role << "\n";
        return true;
      }
      s.out << "[theme: " << role << " updated]\n";
    } else {
      s.out << "Unknown theme: " << arg << ". Try: dark, light, mono, hacker\n";
    }
    // --- Context compression: summarize history to reduce token usage ---
  } else if (command == "compress") {
    LOG_FEATURE("cmd_compress");
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
