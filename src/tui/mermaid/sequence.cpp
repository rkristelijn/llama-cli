/**
 * @file sequence.cpp
 * @brief Sequence diagram renderer — parses mermaid sequenceDiagram syntax.
 *
 * Renders participants as labeled columns with ASCII arrows between them.
 * Supports: ->> (solid), -->> (dashed), Note over/left/right.
 *
 * Example output:
 *   Client         Server
 *     |               |
 *     |--GET /data--->|
 *     |               |
 *     |<--200 OK------|
 *     |               |
 */
#include "tui/mermaid/sequence.h"

#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace tui {

// --- Internal data structures ---

struct Participant {
  std::string name;
  int col = 0;  // center column position in output
};

enum class ArrowStyle { SOLID, DASHED };
enum class ArrowDir { LEFT, RIGHT };

struct Message {
  int from = 0;
  int to = 0;
  std::string label;
  ArrowStyle style = ArrowStyle::SOLID;
};

struct Note {
  int participant = 0;
  std::string text;
};

// Each action is either a message or a note
struct Action {
  bool is_note = false;
  Message msg;
  Note note;
};

// --- Parser helpers ---

static std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }
  auto end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

static int find_participant(std::vector<Participant>& parts, const std::string& name) {
  for (int i = 0; i < static_cast<int>(parts.size()); i++) {
    if (parts[i].name == name) {
      return i;
    }
  }
  // Auto-add participant on first reference
  parts.push_back({name, 0});
  return static_cast<int>(parts.size()) - 1;
}

/// Parse arrow syntax: ->>, -->>, ->, -->
/// Returns position after arrow, or npos if not found.
static size_t find_arrow(const std::string& line, size_t start, ArrowStyle& style) {
  // Try -->> first (dashed async)
  auto pos = line.find("-->>", start);
  if (pos != std::string::npos) {
    style = ArrowStyle::DASHED;
    return pos;
  }
  // Try ->> (solid async)
  pos = line.find("->>", start);
  if (pos != std::string::npos) {
    style = ArrowStyle::SOLID;
    return pos;
  }
  // Try --> (dashed)
  pos = line.find("-->", start);
  if (pos != std::string::npos) {
    style = ArrowStyle::DASHED;
    return pos;
  }
  // Try -> (solid)
  pos = line.find("->", start);
  if (pos != std::string::npos) {
    style = ArrowStyle::SOLID;
    return pos;
  }
  return std::string::npos;
}

static size_t arrow_len(const std::string& line, size_t pos) {
  if (line.compare(pos, 4, "-->>") == 0) {
    return 4;
  }
  if (line.compare(pos, 3, "->>") == 0) {
    return 3;
  }
  if (line.compare(pos, 3, "-->") == 0) {
    return 3;
  }
  return 2;  // ->
}

// --- Renderer ---

bool SequenceRenderer::can_render(const std::string& input) const {
  auto pos = input.find_first_not_of(" \t\r\n");
  if (pos == std::string::npos) {
    return false;
  }
  return input.compare(pos, 15, "sequenceDiagram") == 0;
}

bool SequenceRenderer::render(const std::string& input, std::ostream& out, int cols, int /*rows*/) const {
  std::vector<Participant> participants;
  std::vector<Action> actions;

  // Parse input line by line
  std::istringstream stream(input);
  std::string line;
  bool header_found = false;

  while (std::getline(stream, line)) {
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (!header_found) {
      if (line.find("sequenceDiagram") != std::string::npos) {
        header_found = true;
      }
      continue;
    }
    // Skip activate/deactivate/loop/end/alt/else
    if (line.find("activate") == 0 || line.find("deactivate") == 0 || line.find("loop") == 0 || line.find("alt") == 0 ||
        line.find("else") == 0 || line == "end") {
      continue;
    }
    // Parse "participant X" or "participant X as Label"
    if (line.find("participant") == 0) {
      std::string rest = trim(line.substr(11));
      auto as_pos = rest.find(" as ");
      std::string pname = (as_pos != std::string::npos) ? trim(rest.substr(as_pos + 4)) : rest;
      std::string id = (as_pos != std::string::npos) ? trim(rest.substr(0, as_pos)) : rest;
      int idx = find_participant(participants, id);
      if (as_pos != std::string::npos) {
        participants[idx].name = pname;
      }
      continue;
    }
    // Parse "Note over/left of/right of Participant: text"
    if (line.find("Note") == 0) {
      auto colon = line.find(':');
      if (colon != std::string::npos) {
        // Extract participant name from between "Note ... of " and ":"
        std::string mid = line.substr(4, colon - 4);
        auto of_pos = mid.rfind("of ");
        std::string pname;
        if (of_pos != std::string::npos) {
          pname = trim(mid.substr(of_pos + 3));
        } else {
          // "Note over Client" without "of"
          auto over_pos = mid.find("over ");
          pname = (over_pos != std::string::npos) ? trim(mid.substr(over_pos + 5)) : "";
        }
        // Handle "Note over Client,Server" — use first participant
        auto comma = pname.find(',');
        if (comma != std::string::npos) {
          pname = trim(pname.substr(0, comma));
        }
        int idx = pname.empty() ? 0 : find_participant(participants, pname);
        Action a;
        a.is_note = true;
        a.note = {idx, trim(line.substr(colon + 1))};
        actions.push_back(a);
      }
      continue;
    }
    // Parse message: "From->>To: Label" or "From-->>To: Label"
    ArrowStyle style = ArrowStyle::SOLID;
    auto arrow_pos = find_arrow(line, 0, style);
    if (arrow_pos != std::string::npos) {
      std::string from_name = trim(line.substr(0, arrow_pos));
      size_t alen = arrow_len(line, arrow_pos);
      std::string rest = line.substr(arrow_pos + alen);
      // Split on colon for label
      auto colon = rest.find(':');
      std::string to_name = (colon != std::string::npos) ? trim(rest.substr(0, colon)) : trim(rest);
      std::string label = (colon != std::string::npos) ? trim(rest.substr(colon + 1)) : "";
      int from_idx = find_participant(participants, from_name);
      int to_idx = find_participant(participants, to_name);
      Action a;
      a.msg = {from_idx, to_idx, label, style};
      actions.push_back(a);
      continue;
    }
  }

  if (participants.empty()) {
    return false;
  }

  // Calculate minimum column width: max of participant name and longest label
  int max_label_len = 0;
  for (const auto& a : actions) {
    if (!a.is_note && static_cast<int>(a.msg.label.size()) > max_label_len) {
      max_label_len = static_cast<int>(a.msg.label.size());
    }
  }

  // Layout: assign column positions, constrained by terminal width
  // Auto-detect terminal width if not specified
  int term_cols = cols;
  if (term_cols <= 0) {
    struct winsize w = {};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
      term_cols = w.ws_col;
    } else {
      term_cols = 80;
    }
  }

  int num_parts = static_cast<int>(participants.size());
  // Divide available width evenly among participants
  int col_width = term_cols / std::max(num_parts, 1);
  // Ensure minimum usable width
  if (col_width < 10) {
    col_width = 10;
  }
  for (int i = 0; i < num_parts; i++) {
    participants[i].col = i * col_width + col_width / 2;
  }
  int total_width = term_cols;

  // Helper: draw a lifeline row (just vertical bars at each participant column)
  auto lifeline = [&]() -> std::string {
    std::string row(total_width, ' ');
    for (const auto& p : participants) {
      if (p.col < static_cast<int>(row.size())) {
        row[p.col] = '|';
      }
    }
    return row;
  };

  // Print participant header (truncate names that exceed column width)
  std::string header(total_width, ' ');
  for (const auto& p : participants) {
    std::string pname = p.name;
    int max_name = col_width - 2;
    if (static_cast<int>(pname.size()) > max_name) {
      pname = pname.substr(0, max_name - 1) + "\xe2\x80\xa6";  // …
    }
    int start = p.col - static_cast<int>(pname.size()) / 2;
    if (start < 0) {
      start = 0;
    }
    for (size_t i = 0; i < pname.size() && start + static_cast<int>(i) < total_width; i++) {
      header[start + i] = pname[i];
    }
  }
  out << header << "\n";
  out << lifeline() << "\n";

  // Render each action
  for (const auto& action : actions) {
    if (action.is_note) {
      // Render note as text next to the participant's lifeline
      std::string row(total_width, ' ');
      for (const auto& p : participants) {
        if (p.col < static_cast<int>(row.size())) {
          row[p.col] = '|';
        }
      }
      int note_start = participants[action.note.participant].col + 2;
      std::string note_text = "[" + action.note.text + "]";
      for (size_t i = 0; i < note_text.size() && note_start + static_cast<int>(i) < total_width; i++) {
        row[note_start + i] = note_text[i];
      }
      out << row << "\n";
    } else {
      // Render message arrow
      int from_col = participants[action.msg.from].col;
      int to_col = participants[action.msg.to].col;
      int left = std::min(from_col, to_col);
      int right = std::max(from_col, to_col);

      std::string row(total_width, ' ');
      // Draw other lifelines
      for (const auto& p : participants) {
        if (p.col < static_cast<int>(row.size()) && p.col != from_col && p.col != to_col) {
          row[p.col] = '|';
        }
      }
      // Draw arrow line between from and to
      char fill = '-';
      for (int c = left + 1; c < right; c++) {
        row[c] = fill;
      }
      // Arrow head and tail
      if (to_col > from_col) {
        row[from_col] = '|';
        row[right] = '>';
      } else {
        row[from_col] = '|';
        row[left] = '<';
      }
      // Place label centered on the arrow line
      if (!action.msg.label.empty()) {
        int mid = (left + right) / 2;
        int label_start = mid - static_cast<int>(action.msg.label.size()) / 2;
        if (label_start <= left + 1) {
          label_start = left + 2;
        }
        int label_end = label_start + static_cast<int>(action.msg.label.size());
        if (label_end >= right) {
          label_end = right - 1;
        }
        for (int i = label_start; i < label_end && (i - label_start) < static_cast<int>(action.msg.label.size()); i++) {
          row[i] = action.msg.label[i - label_start];
        }
      }
      out << row << "\n";
      out << lifeline() << "\n";
    }
  }

  return true;
}

}  // namespace tui
