/**
 * @file timeline.cpp
 * @brief Timeline renderer — vertical event list with markers.
 *
 * Parses mermaid timeline syntax:
 *   timeline
 *     title Release History
 *     section 2024
 *       January : Initial release
 *       March   : Added streaming
 *     section 2025
 *       June    : Multi-model support
 *
 * Renders as:
 *   Release History
 *
 *   2024
 *   ●── January — Initial release
 *   │
 *   ●── March — Added streaming
 *   │
 *   2025
 *   ●── June — Multi-model support
 */
#include "tui/mermaid/timeline.h"

#include <sstream>
#include <string>
#include <vector>

namespace tui {

struct TimelineEvent {
  std::string date;
  std::string description;
  bool is_section = false;
};

static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

bool TimelineRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 8, "timeline") == 0;
}

bool TimelineRenderer::render(const std::string& input, std::ostream& out, int /*cols*/, int /*rows*/) const {
  std::string title;
  std::vector<TimelineEvent> events;

  std::istringstream stream(input);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    std::string trimmed = trim(line);
    if (trimmed.empty()) {
      continue;
    }
    if (!header_found) {
      if (trimmed.find("timeline") == 0) {
        header_found = true;
      }
      continue;
    }
    if (trimmed.find("title") == 0) {
      title = trim(trimmed.substr(5));
      continue;
    }
    if (trimmed.find("section") == 0) {
      events.push_back({trim(trimmed.substr(7)), "", true});
      continue;
    }
    // Event: "Date : Description" or just "Description"
    auto colon = trimmed.find(':');
    if (colon != std::string::npos) {
      events.push_back({trim(trimmed.substr(0, colon)), trim(trimmed.substr(colon + 1)), false});
    } else {
      events.push_back({trimmed, "", false});
    }
  }

  if (events.empty()) {
    return false;
  }

  // Print title
  if (!title.empty()) {
    out << "\033[1m" << title << "\033[0m\n\n";
  }

  // Render events
  for (size_t i = 0; i < events.size(); i++) {
    const auto& ev = events[i];

    if (ev.is_section) {
      // Section header
      out << "\033[1;33m" << ev.date << "\033[0m\n";
      continue;
    }

    // Event marker: ●── Date — Description
    out << "  \xe2\x97\x8f\xe2\x94\x80\xe2\x94\x80 ";  // ●──
    out << "\033[1m" << ev.date << "\033[0m";
    if (!ev.description.empty()) {
      out << " \xe2\x80\x94 " << ev.description;  // —
    }
    out << "\n";

    // Connecting line (unless last event or next is section)
    bool next_is_section = (i + 1 < events.size() && events[i + 1].is_section);
    bool is_last = (i + 1 >= events.size());
    if (!is_last && !next_is_section) {
      out << "  \xe2\x94\x82\n";  // │
    } else if (!is_last) {
      out << "\n";
    }
  }

  return true;
}

}  // namespace tui
