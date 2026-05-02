/**
 * @file repl_annotations.cpp
 * @brief Annotation handlers for REPL mode (write/read/exec/str_replace).
 *
 * SRP: All interactive annotation processing lives here.
 * Extracted from repl.cpp to reduce file size (ADR-061, ADR-065).
 *
 * @see docs/adr/adr-014-tool-annotations.md
 * @see docs/adr/adr-019-safe-file-writes.md
 */

#include "repl/repl_annotations.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>

#include "dtl/dtl.hpp"
#include "exec/exec.h"
#include "logging/logger.h"
#include "trace/trace.h"
#include "tui/tui.h"

// --- File I/O helpers ---
// These are file-local utilities used by the annotation handlers.
// read_file: slurps entire file into string (returns "" on failure).
// emit_diff_line: prints one colored diff line (red for delete, green for add).

/// Read entire file into a string. Returns empty on failure.
static std::string read_file(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) {
    return "";
  }
  return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

/// Emit one diff line with optional ANSI color prefix.
static void emit_diff_line(std::ostream& out, const char* ansi, const char* prefix, const std::string& line, bool color) {
  if (color) {
    out << ansi << prefix << line << "\033[0m\n";
  } else {
    out << prefix << line << "\n";
  }
}

// --- Diff rendering ---
// Uses Myers diff algorithm (dtl library) for accurate change detection.
// Only changed lines and their surrounding context (3 lines) are shown.
// Unchanged regions are skipped — keeps output focused on actual changes.

/// Print a git-style unified diff with 3 lines of context and @@ hunk headers.
/// Uses Myers diff (via dtl) for accurate change detection.
// pmccabe:skip-complexity
// NOLINTNEXTLINE(readability-function-size)
void show_diff(const std::string& old_text, const std::string& new_text, std::ostream& out, bool color) {
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

  // Emit hunks with @@ headers
  bool in_hunk = false;
  for (size_t i = 0; i < entries.size(); ++i) {
    if (!show[i]) {
      in_hunk = false;
      continue;
    }
    if (!in_hunk) {
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

// --- Write confirmation ---

/// Prompt user to confirm a file write. Shows diff or content, accepts y/n/s/t/c.
/// y=confirm, n=decline, s=show content again, t=trust all, c=copy to clipboard.
/// Returns true if the write should proceed, false if declined.
// pmccabe:skip-complexity
static bool confirm_write(const WriteAction& action, std::istream& in, std::ostream& out, bool color, bool& trust, bool auto_confirm) {
  // Trust mode: auto-approve all remaining actions this session
  if (trust) {
    return true;
  }
  // Check if file exists to decide between diff and full-content display
  std::ifstream check(action.path);
  bool file_exists = check.good();
  check.close();
  std::string existing = file_exists ? read_file(action.path) : "";

  // Always show diff/content before prompting — user sees what will change
  if (file_exists) {
    LOG_FEATURE("write_diff_existing");
    show_diff(existing, action.content, out, color);
  } else {
    LOG_FEATURE("write_new_file");
    // New file: show full content since there's nothing to diff against
    out << action.content << "\n";
  }

  // Auto-confirm mode: skip interactive prompt (used in automated workflows)
  if (auto_confirm) {
    LOG_EVENT("repl", "file_write", action.path, "auto-confirmed", 0, 0, 0);
    out << "[auto-written: " << action.path << "]\n";
    return true;
  }
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
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
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

// --- Action processors ---
// These are the public functions called from handle_response in repl.cpp.
// Each handles one annotation type: write, str_replace, read, or exec.

/// Present a proposed file write to the user and perform the write if confirmed.
/// Creates a .bak backup of existing files before overwriting.
/// Logs both confirmed and declined writes for audit trail.
void process_write(const WriteAction& action, std::istream& in, std::ostream& out, bool color, bool& trust, bool auto_confirm) {
  LOG_FEATURE("write_annotation");
  if (confirm_write(action, in, out, color, trust, auto_confirm)) {
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
      LOG_FEATURE("write_confirmed");
      LOG_EVENT("repl", "file_write", action.path, "ok", 0, 0, 0);
    } else {
      tui::error(out, color, "Error: could not write to " + action.path);
      LOG_EVENT("repl", "file_write", action.path, "error: could not write", 0, 0, 0);
    }
  } else {
    out << "[skipped]\n";
    LOG_FEATURE("write_declined");
    LOG_EVENT("repl", "file_write_declined", action.path, "", 0, 0, 0);
  }
}

/// Apply a <str_replace> action: show diff, prompt user, then do targeted replacement.
/// In trust mode, auto-applies without prompting.
/// Creates a .bak backup before modifying the file.
void process_str_replace(const StrReplaceAction& action, std::istream& in, std::ostream& out, bool color, bool& trust) {
  LOG_FEATURE("str_replace_annotation");
  if (trust) {
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
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
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
    LOG_EVENT("repl", "str_replace", action.path, "ok", 0, 0, 0);
  } else {
    tui::error(out, color, "Error: could not write to " + action.path);
    LOG_EVENT("repl", "str_replace", action.path, "error: could not write", 0, 0, 0);
  }
}

/// Execute a <read> action and return the result as context for the LLM.
/// Supports three modes: full file, line range, and search (grep with context).
/// Returns formatted string with file path and content, or empty on error.
std::string process_read(const ReadAction& action, std::ostream& out, bool color) {
  LOG_FEATURE("read_annotation");
  std::ifstream check(action.path);
  if (!check.good()) {
    LOG_FEATURE("read_not_found");
    tui::error(out, color, "read: file not found: " + action.path);
    LOG_EVENT("repl", "file_read", action.path, "error: not found", 0, 0, 0);
    return "";
  }
  std::string content = read_file(action.path);

  // Split into lines (1-based indexing)
  std::vector<std::string> lines;
  std::istringstream ss(content);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }

  std::ostringstream result;
  result << "[file: " << action.path;

  if (!action.search.empty()) {
    LOG_FEATURE("read_search");
    // Search mode: return lines containing the term with ±3 lines context
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
    LOG_FEATURE("read_line_range");
    // Line range mode
    int from = std::max(1, action.from_line);
    int to = std::min(static_cast<int>(lines.size()), action.to_line);
    result << " lines=" << from << "-" << to << "]\n";
    for (int i = from; i <= to; ++i) {
      result << i << ": " << lines[i - 1] << "\n";
    }
  } else {
    LOG_FEATURE("read_full_file");
    // Full file read
    result << "]\n" << content;
  }

  std::string r = result.str();
  LOG_EVENT("repl", "file_read", action.path, "", 0, 0, 0);
  return r;
}

/// Strip <exec> annotations from text, replacing with readable placeholders.
/// Also removes any remaining raw annotation tags (<write>, <read>, etc.)
/// so the user never sees raw XML in the terminal output.
std::string strip_exec_annotations(const std::string& text) {
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
  // Strip remaining annotation-like tags so raw XML never reaches the user
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

/// Confirm and execute an LLM-proposed command (ADR-015).
/// Prompts user with y/n/t/c. In trust mode, auto-executes.
/// Returns command output if executed, empty string if declined/copied.
std::string confirm_exec(const std::string& cmd, const Config& cfg, std::istream& in, std::ostream& out, bool& trust) {
  LOG_FEATURE("exec_annotation");
  if (!trust) {
    out << "Run: \033[1;33m" << cmd << "\033[0m? [y/n/t/c] " << std::flush;
    std::string answer;
    if (!std::getline(in, answer)) {
      return "";
    }
    if (answer == "t" || answer == "trust") {
      trust = true;
    } else if (answer == "c" || answer == "copy") {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
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

  if (cfg.trace) {
    stderr_trace->log("[TRACE] exec (LLM): %s exit=%d %ldms\n", cmd.c_str(), r.exit_code, ms);
  }

  LOG_EVENT("repl", "exec_confirmed", cmd, r.output, static_cast<int>(ms), 0, 0);
  out << r.output;
  return r.output;
}
