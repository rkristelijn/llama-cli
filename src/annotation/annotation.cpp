/**
 * @file annotation.cpp
 * @brief Parse LLM tool annotations (XML tags) from response text.
 * @see docs/adr/adr-014-tool-annotations.md
 */

#include "annotation/annotation.h"

/**
 * @brief Locates the next <write file="...">...</write> block in the input text starting at pos.
 *
 * Extracts the file path from the opening tag and the enclosed content, trimming at most one
 * leading newline and at most one trailing newline from the content. On success, updates pos
 * to the index immediately after the closing </write> tag and returns the start index of the block.
 *
 * @param text Input string to search.
 * @param[in,out] pos Search start index; updated to the character after the closing tag on success.
 * @param[out] path Set to the path value found in the opening tag.
 * @param[out] content Set to the extracted content with one leading/trailing newline removed if present.
 * @return size_t Start index of the located block, or std::string::npos if no complete block is found.
 */
static size_t find_write_block(const std::string& text, size_t& pos, std::string& path, std::string& content) {
  const std::string open_tag = "<write file=\"";
  const std::string close_tag = "</write>";

  auto start = text.find(open_tag, pos);
  if (start == std::string::npos) {
    return std::string::npos;
  }

  auto path_start = start + open_tag.size();
  auto path_end = text.find("\"", path_start);
  if (path_end == std::string::npos) {
    return std::string::npos;
  }
  path = text.substr(path_start, path_end - path_start);

  auto content_start = text.find(">", path_end);
  if (content_start == std::string::npos) {
    return std::string::npos;
  }
  content_start++;

  auto content_end = text.find(close_tag, content_start);
  if (content_end == std::string::npos) {
    return std::string::npos;
  }

  content = text.substr(content_start, content_end - content_start);
  if (!content.empty() && content.front() == '\n') {
    content.erase(0, 1);
  }
  if (!content.empty() && content.back() == '\n') {
    content.pop_back();
  }

  pos = content_end + close_tag.size();
  return start;
}

/**
 * @brief Extracts all <write file="path">content</write> annotations from the input text.
 *
 * @param text Input string to scan for write-file annotation blocks.
 * @return std::vector<WriteAction> Vector of WriteAction objects containing the `path` and `content` for each found annotation; empty if none are found.
 */
std::vector<WriteAction> parse_write_annotations(const std::string& text) {
  std::vector<WriteAction> actions;
  size_t pos = 0;
  std::string path, content;
  while (find_write_block(text, pos, path, content) != std::string::npos) {
    actions.push_back({path, content});
  }
  return actions;
}

/** Replace annotations with [proposed: write path] summaries
 * Leaves non-annotation text intact for display to user
 * Iterates until no more annotations are found */
std::string strip_annotations(const std::string& text) {
  std::string result = text;
  const std::string open_prefix = "<write file=\"";
  const std::string close_tag = "</write>";

  while (true) {
    auto start = result.find(open_prefix);
    if (start == std::string::npos) {
      break;
    }

    auto path_start = start + open_prefix.size();
    auto path_end = result.find("\"", path_start);
    if (path_end == std::string::npos) {
      break;
    }
    std::string path = result.substr(path_start, path_end - path_start);

    auto end = result.find(close_tag, start);
    if (end == std::string::npos) {
      break;
    }
    end += close_tag.size();

    result.replace(start, end - start, "[proposed: write " + path + "]");
  }
  return result;
}
