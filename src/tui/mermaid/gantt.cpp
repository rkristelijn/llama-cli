/**
 * @file gantt.cpp
 * @brief Gantt chart renderer — horizontal time bars with section grouping.
 *
 * Parses mermaid gantt syntax:
 *   gantt
 *     title Project Plan
 *     section Design
 *     Research     :a1, 2024-01-01, 10d
 *     Wireframes   :a2, after a1, 5d
 *     section Build
 *     Coding       :b1, after a2, 20d
 *
 * Renders as horizontal bars showing relative task duration:
 *   Project Plan
 *
 *   Design
 *     Research     ████████░░░░░░░░░░░░░░░░░░░░░░
 *     Wireframes   ░░░░░░░░████░░░░░░░░░░░░░░░░░░
 *   Build
 *     Coding       ░░░░░░░░░░░░████████████████████
 */
#include "tui/mermaid/gantt.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace tui {

// --- Internal structures ---

struct GanttTask {
  std::string label;
  std::string section;
  int start = 0;  // relative start (in days from project start)
  int duration = 1;
};

static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

/// Parse duration string like "10d", "2w", "3" → days
static int parse_duration(const std::string& s) {
  std::string trimmed = trim(s);
  if (trimmed.empty()) {
    return 1;
  }
  int val = 0;
  size_t i = 0;
  while (i < trimmed.size() && trimmed[i] >= '0' && trimmed[i] <= '9') {
    val = val * 10 + (trimmed[i] - '0');
    i++;
  }
  if (val == 0) {
    return 1;
  }
  // Check unit suffix
  if (i < trimmed.size() && trimmed[i] == 'w') {
    return val * 7;
  }
  return val;  // default: days
}

bool GanttRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 5, "gantt") == 0;
}

bool GanttRenderer::render(const std::string& input, std::ostream& out, int cols, int /*rows*/) const {
  std::string title;
  std::string current_section;
  std::vector<GanttTask> tasks;
  int cursor = 0;  // tracks current time position for sequential tasks

  // Parse input
  std::istringstream stream(input);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (!header_found) {
      if (line.find("gantt") == 0) {
        header_found = true;
      }
      continue;
    }
    // Title
    if (line.find("title") == 0) {
      title = trim(line.substr(5));
      continue;
    }
    // Section
    if (line.find("section") == 0) {
      current_section = trim(line.substr(7));
      continue;
    }
    // Task line: "Label :id, start, duration" or "Label :start, duration"
    auto colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    std::string label = trim(line.substr(0, colon));
    std::string params = line.substr(colon + 1);

    // Split params by comma
    std::vector<std::string> parts;
    std::istringstream ps(params);
    std::string part;
    while (std::getline(ps, part, ',')) {
      parts.push_back(trim(part));
    }

    // Determine start and duration from params
    int task_start = cursor;
    int task_dur = 1;

    // Last param is usually duration
    if (!parts.empty()) {
      std::string last = parts.back();
      task_dur = parse_duration(last);
    }
    // Check for "after id" in params to set start
    for (const auto& p : parts) {
      if (p.find("after") == 0) {
        // "after" means start after previous task ends
        task_start = cursor;
        break;
      }
    }

    tasks.push_back({label, current_section, task_start, task_dur});
    cursor = task_start + task_dur;
  }

  if (tasks.empty()) {
    return false;
  }

  // Calculate total timeline span
  int total_days = 0;
  for (const auto& t : tasks) {
    int end = t.start + t.duration;
    if (end > total_days) {
      total_days = end;
    }
  }

  // Bar width calculation
  int label_width = 0;
  for (const auto& t : tasks) {
    int len = static_cast<int>(t.label.size());
    if (len > label_width) {
      label_width = len;
    }
  }
  label_width += 2;  // padding

  int bar_width = (cols > 0 ? cols : 80) - label_width - 4;
  if (bar_width < 20) {
    bar_width = 20;
  }

  // ANSI colors for sections
  static const char* colors[] = {"\033[36m", "\033[33m", "\033[32m", "\033[35m", "\033[34m", "\033[31m"};
  static const int num_colors = 6;
  int color_idx = 0;

  // Print title
  if (!title.empty()) {
    out << title << "\n\n";
  }

  // Render tasks grouped by section
  std::string last_section;
  for (const auto& t : tasks) {
    if (t.section != last_section) {
      if (!t.section.empty()) {
        out << "\033[1m" << t.section << "\033[0m\n";
      }
      last_section = t.section;
      color_idx++;
    }

    // Label (right-padded)
    out << "  " << t.label;
    int pad = label_width - static_cast<int>(t.label.size());
    for (int i = 0; i < pad; i++) {
      out << ' ';
    }

    // Bar: show position and duration proportionally
    const char* color = colors[color_idx % num_colors];
    int bar_start = (t.start * bar_width) / std::max(total_days, 1);
    int bar_end = ((t.start + t.duration) * bar_width) / std::max(total_days, 1);
    if (bar_end <= bar_start) {
      bar_end = bar_start + 1;
    }

    // Empty before bar
    out << "\033[2m";
    for (int i = 0; i < bar_start; i++) {
      out << "\xe2\x96\x91";  // ░
    }
    out << "\033[0m";
    // Filled bar
    out << color;
    for (int i = bar_start; i < bar_end; i++) {
      out << "\xe2\x96\x88";  // █
    }
    out << "\033[0m";
    // Empty after bar
    out << "\033[2m";
    for (int i = bar_end; i < bar_width; i++) {
      out << "\xe2\x96\x91";  // ░
    }
    out << "\033[0m\n";
  }

  return true;
}

}  // namespace tui
