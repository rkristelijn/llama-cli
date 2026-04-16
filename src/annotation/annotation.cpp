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
    return "";
  }
  pos += key.size();
  auto end = tag.find('"', pos);
  if (end == std::string::npos) {
    return "";
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
  return "";
}

/**
 * @brief Extract all \<str_replace\> annotations with \<old\> and \<new\> children.
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

// --- strip -------------------------------------------------------------------

/**
 * @brief Replace all annotation tags with human-readable summaries.
 *
 * Strips \<write\>, \<str_replace\>, and \<read\> tags, replacing each with
 * a bracketed summary like [proposed: write path].
 *
 * @param text Input string possibly containing annotation tags.
 * @return Cleaned string with annotations replaced by summaries.
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
    result.replace(start, end + 8 - start, "[proposed: write " + path + "]");
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
    result.replace(start, end + 14 - start, "[proposed: str_replace " + path + "]");
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
    result.replace(start, end + 1 - start, "[read " + path + "]");
  }

  return result;
}
