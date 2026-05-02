/**
 * @file sync_annotations.cpp
 * @brief Annotation processing for sync (non-interactive) mode.
 *
 * SRP: Handles all LLM-proposed actions in sync mode without user confirmation.
 * Extracted from main.cpp to reduce file size (ADR-061, ADR-065).
 *
 * @see docs/adr/adr-030-stdin-sync.md
 */

#include "sync/sync_annotations.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "annotation/annotation.h"
#include "exec/exec.h"
#include "logging/logger.h"
#include "sync/sync.h"

std::string process_sync_annotations(const std::string& response, const Config& cfg) {
  std::string caps = cfg.capabilities;
  std::string followup;

  // Process <read> annotations — read files into context for the LLM.
  // Only allowed when "read" capability is granted (ADR-030).
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

  // Process <exec> annotations — run commands and capture output.
  // "exec" capability allows all commands; "read" allows safe read-only commands.
  auto execs = parse_exec_tags(response);
  for (const auto& cmd : execs) {
    LOG_FEATURE("exec_annotation");
    bool allowed = has_cap(caps, "exec") || (has_cap(caps, "read") && is_read_only(cmd));
    if (!allowed) {
      LOG_FEATURE("exec_blocked");
      std::cerr << "[blocked: " << cmd << "]\n";
      continue;
    }
    LOG_FEATURE("exec_allowed");
    auto r = cmd_exec(cmd, cfg.exec_timeout, cfg.max_output);
    followup += "[command: " + cmd + "]\n" + r.output + "\n";
    std::cerr << "[exec " << cmd << "]\n";
  }

  // Process <write> and <str_replace> annotations (requires "write" capability).
  // All writes are sandboxed — paths outside cfg.sandbox are rejected.
  if (has_cap(caps, "write")) {
    auto writes = parse_write_annotations(response);
    for (const auto& action : writes) {
      if (!path_allowed(action.path, cfg.sandbox)) {
        LOG_FEATURE("sync_write_sandbox_blocked");
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

    // Process <str_replace> annotations
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
        LOG_FEATURE("sync_str_replace_not_found");
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

    // Process <add_line> annotations — insert a line at a specific position.
    // Line numbers are 1-based; index is clamped to valid range.
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
      while (std::getline(rf, line)) {
        lines.push_back(line);
      }
      rf.close();
      // Clamp insertion index to valid range [0, lines.size()]
      int idx = action.line_number - 1;
      if (idx < 0) {
        idx = 0;
      }
      if (idx > static_cast<int>(lines.size())) {
        idx = static_cast<int>(lines.size());
      }
      lines.insert(lines.begin() + idx, action.content);
      std::ofstream wf(action.path);
      if (wf) {
        for (const auto& l : lines) {
          wf << l << "\n";
        }
        std::cerr << "[add_line " << action.path << " at " << action.line_number << "]\n";
      }
    }

    // Process <delete_line> annotations — remove a line matching content.
    // Finds first exact match and removes it; reports error if not found.
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
      while (std::getline(rf, line)) {
        lines.push_back(line);
      }
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
          for (const auto& l : lines) {
            wf << l << "\n";
          }
          std::cerr << "[delete_line " << action.path << "]\n";
        }
      } else {
        followup += "[error: line not found in " + action.path + ": " + action.content + "]\n";
      }
    }
  }

  return followup;
}
