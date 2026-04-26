/**
 * @file annotation.cpp
 * @brief Parse LLM tool annotations (XML tags) from response text.
 * @see docs/adr/adr-014-tool-annotations.md
 */

#include "annotation/annotation.h"

#include <sstream>

// --- helpers -----------------------------------------------------------------

/// Extract the value of attribute `name` from an opening XML tag string.
/// Returns empty string if not found.
static std::string attr(const std::string& tag, const std::string& name) {
  auto key = name + "=\"";
  auto pos = tag.find(key);
  if (pos == std::string::npos) {
    return "";  // error-handling:ok — not-found is normal for optional attributes
  }
  pos += key.size();
  auto end = tag.find('"', pos);
  if (end == std::string::npos) {
    return "";  // error-handling:ok — malformed tag, caller handles empty result
  }
  return tag.substr(pos, end - pos);
}

/// Find the next complete \<tag ...\>content\</tag\> block starting at pos.
/// Returns npos if not found; updates pos past the closing tag.
static size_t find_block(const std::string& text, const std::string& tag_name, size_t& pos, std::string& opening_tag,
                         std::string& content) {
  std::string open_start = "<" + tag_name;
  std::string close_tag = "</" + tag_name + ">";

  auto start = text.find(open_start, pos);
  if (start == std::string::npos) {
    return std::string::npos;
  }

  auto tag_end = text.find('>', start);
  if (tag_end == std::string::npos) {
    return std::string::npos;
  }
  opening_tag = text.substr(start, tag_end - start + 1);

  auto content_start = tag_end + 1;
  auto content_end = text.find(close_tag, content_start);
  if (content_end == std::string::npos) {
    return std::string::npos;
  }

  content = text.substr(content_start, content_end - content_start);
  // trim one leading/trailing newline
  if (!content.empty() && content.front() == '\n') {
    content.erase(0, 1);
  }
  if (!content.empty() && content.back() == '\n') {
    content.pop_back();
  }

  pos = content_end + close_tag.size();
  return start;
}

// --- <write> -----------------------------------------------------------------

/**
 * @brief Extract all \<write file="path"\>content\</write\> annotations from response text.
 * @param text Input string to scan for write annotations.
 * @return Vector of WriteAction with path and content for each found annotation.
 */
std::vector<WriteAction> parse_write_annotations(const std::string& text) {
  std::vector<WriteAction> actions;
  size_t pos = 0;
  std::string opening, content;
  while (find_block(text, "write", pos, opening, content) != std::string::npos) {
    auto path = attr(opening, "file");
    if (!path.empty()) {
      actions.push_back({path, content});
    }
  }
  return actions;
}

// --- <str_replace> -----------------------------------------------------------

/// Find the first child tag (e.g. \<old\>...\</old\>) inside content.
static std::string child_content(const std::string& text, const std::string& tag) {
  size_t pos = 0;
  std::string opening, content;
  if (find_block(text, tag, pos, opening, content) != std::string::npos) {
    return content;
  }
  return "";  // error-handling:ok — no matching block found, normal for parsing
}

/**
 * @brief Extract all \<str_replace\> annotations with \<old\> and \<new\> children.
 * @param text Input string to scan.
 * @param text Input string to scan.
 * @return Vector of StrReplaceAction with path, old_str, and new_str.
 */
std::vector<StrReplaceAction> parse_str_replace_annotations(const std::string& text) {
  std::vector<StrReplaceAction> actions;
  size_t pos = 0;
  std::string opening, content;
  while (find_block(text, "str_replace", pos, opening, content) != std::string::npos) {
    auto path = attr(opening, "path");
    auto old_str = child_content(content, "old");
    auto new_str = child_content(content, "new");
    if (!path.empty()) {
      actions.push_back({path, old_str, new_str});
    }
  }
  return actions;
}

// --- <read> ------------------------------------------------------------------

/**
 * @brief Extract all \<read path="..." lines="x-y" search="term"/\> annotations.
 * @param text Input string to scan.
 * @return Vector of ReadAction with path, optional line range, and optional search term.
 */
std::vector<ReadAction> parse_read_annotations(const std::string& text) {
  std::vector<ReadAction> actions;
  std::string open_start = "<read ";
  size_t pos = 0;
  while (true) {
    auto start = text.find(open_start, pos);
    if (start == std::string::npos) {
      break;
    }
    auto tag_end = text.find('>', start);
    if (tag_end == std::string::npos) {
      break;
    }
    std::string tag = text.substr(start, tag_end - start + 1);
    pos = tag_end + 1;

    ReadAction action;
    action.path = attr(tag, "path");
    if (action.path.empty()) {
      continue;
    }

    auto lines_val = attr(tag, "lines");
    if (!lines_val.empty()) {
      auto dash = lines_val.find('-');
      if (dash != std::string::npos) {
        try {
          action.from_line = std::stoi(lines_val.substr(0, dash));
          action.to_line = std::stoi(lines_val.substr(dash + 1));
        } catch (...) {
          // malformed line range from LLM, leave as 0
        }
      }
    }
    action.search = attr(tag, "search");
    actions.push_back(action);
  }
  return actions;
}

// --- <search> ----------------------------------------------------------------

/**
 * @brief Extract all \<search\>query\</search\> annotations from response text.
 * @param text Input string to scan for search annotations.
 * @return Vector of SearchAction with query for each found annotation.
 */
std::vector<SearchAction> parse_search_annotations(const std::string& text) {
  std::vector<SearchAction> actions;
  size_t pos = 0;
  std::string opening, content;
  while (find_block(text, "search", pos, opening, content) != std::string::npos) {
    if (!content.empty()) {
      actions.push_back({content});
    }
  }
  return actions;
}

// --- strip -------------------------------------------------------------------

/**
 * @brief Replace all annotation tags with human-readable summaries (bold white).
 *
 * Strips \<write\>, \<str_replace\>, and \<read\> tags, replacing each with
 * a bold white bracketed summary like [proposed: write path].
 * Bold white makes annotations visually distinct from normal LLM output.
 *
 * @param text Input string possibly containing annotation tags.
 * @return Cleaned string with annotations replaced by bold white summaries.
 */
// todo: reduce complexity of strip_annotations
// pmccabe:skip-complexity
// NOLINTNEXTLINE(readability-function-size)
std::string strip_annotations(const std::string& text) {
  std::string result = text;

  // strip <write file="...">...</write>
  while (true) {
    auto start = result.find("<write file=\"");
    if (start == std::string::npos) {
      break;
    }
    auto path_start = start + 13;
    auto path_end = result.find('"', path_start);
    if (path_end == std::string::npos) {
      break;
    }
    std::string path = result.substr(path_start, path_end - path_start);
    auto end = result.find("</write>", start);
    if (end == std::string::npos) {
      break;
    }
    result.replace(start, end + 8 - start, "\033[1;37m[proposed: write " + path + "]\033[0m");
  }

  // strip <str_replace path="...">...</str_replace>
  while (true) {
    auto start = result.find("<str_replace ");
    if (start == std::string::npos) {
      break;
    }
    auto path_start = result.find("path=\"", start);
    if (path_start == std::string::npos) {
      break;
    }
    path_start += 6;
    auto path_end = result.find('"', path_start);
    if (path_end == std::string::npos) {
      break;
    }
    std::string path = result.substr(path_start, path_end - path_start);
    auto end = result.find("</str_replace>", start);
    if (end == std::string::npos) {
      break;
    }
    result.replace(start, end + 14 - start, "\033[1;37m[proposed: str_replace " + path + "]\033[0m");
  }

  // strip <read ...>
  while (true) {
    auto start = result.find("<read ");
    if (start == std::string::npos) {
      break;
    }
    auto end = result.find('>', start);
    if (end == std::string::npos) {
      break;
    }
    std::string tag = result.substr(start, end - start + 1);
    std::string path = attr(tag, "path");
    result.replace(start, end + 1 - start, "\033[1;37m[read " + path + "]\033[0m");
  }

  // strip <search>...</search>
  while (true) {
    auto start = result.find("<search>");
    if (start == std::string::npos) {
      break;
    }
    auto end = result.find("</search>", start);
    if (end == std::string::npos) {
      break;
    }
    std::string query = result.substr(start + 8, end - start - 8);
    result.replace(start, end + 9 - start, "\033[1;37m[search: " + query + "]\033[0m");
  }

  return result;
}

// --- malformed tag repair -----------------------------------------------------

/// Auto-repair broken closing tags from LLM output.
/// When the LLM writes e.g. <exec>cmd</bash> instead of <exec>cmd</exec>,
/// this replaces the wrong closing tag with the correct one so downstream
/// parsers can match normally.
std::string fix_malformed_tags(const std::string& text) {
  std::string result = text;
  // For each known tag, find openers without a matching closer and fix
  // the next </...> to be the correct closer.
  static const struct {
    const char* open;
    const char* close;
  } tags[] = {
      {"<exec>", "</exec>"},
      {"<write ", "</write>"},
      {"<str_replace ", "</str_replace>"},
      {"<search>", "</search>"},
  };
  for (const auto& t : tags) {
    size_t pos = 0;
    while (true) {
      auto start = result.find(t.open, pos);
      if (start == std::string::npos) {
        break;
      }
      // Already has correct closer? Skip.
      auto correct = result.find(t.close, start);
      auto next_open = result.find(t.open, start + 1);
      if (correct != std::string::npos && (next_open == std::string::npos || correct < next_open)) {
        pos = correct + std::string(t.close).size();
        continue;
      }
      // Find the next </...> tag and replace it with the correct closer,
      // but skip if it belongs to a valid inner element (e.g. </write> inside <exec>)
      auto bad = result.find("</", start + std::string(t.open).size());
      if (bad == std::string::npos) {
        break;
      }
      auto gt = result.find('>', bad);
      if (gt == std::string::npos) {
        break;
      }
      // Extract the tag name from the closing tag (e.g. "write" from "</write>")
      std::string bad_name = result.substr(bad + 2, gt - bad - 2);
      // Check if this closing tag belongs to a valid inner opener
      bool is_inner = false;
      for (const auto& inner : tags) {
        std::string inner_open(inner.open);
        // Match tag name: "<write " → "write", "<exec>" → "exec"
        std::string inner_name = inner_open.substr(1, inner_open.size() - 2);
        if (inner_name.back() == ' ') {
          inner_name.pop_back();
        }
        if (bad_name == inner_name) {
          // Check if there's a matching opener between start and bad
          auto inner_start = result.find(inner.open, start + std::string(t.open).size());
          if (inner_start != std::string::npos && inner_start < bad) {
            is_inner = true;
          }
          break;
        }
      }
      if (is_inner) {
        pos = gt + 1;  // skip this valid inner closer
        continue;
      }
      result.replace(bad, gt - bad + 1, t.close);
      pos = bad + std::string(t.close).size();
    }
  }
  return result;
}