// annotation.cpp — Parse LLM tool annotations from response text
// Simple state machine: find <write file="...">, extract content until </write>

#include "annotation.h"

// Find all <write file="path">content</write> in text
// Returns empty vector if no valid annotations found
std::vector<WriteAction> parse_write_annotations(const std::string& text) {
  std::vector<WriteAction> actions;
  const std::string open_tag = "<write file=\"";
  const std::string close_tag = "</write>";

  size_t pos = 0;
  while (pos < text.size()) {
    // Find opening tag
    auto start = text.find(open_tag, pos);
    if (start == std::string::npos) break;

    // Extract file path between quotes
    auto path_start = start + open_tag.size();
    auto path_end = text.find("\"", path_start);
    if (path_end == std::string::npos) break;
    std::string path = text.substr(path_start, path_end - path_start);

    // Find end of opening tag (the >)
    auto content_start = text.find(">", path_end);
    if (content_start == std::string::npos) break;
    content_start++;  // skip the >

    // Find closing tag
    auto content_end = text.find(close_tag, content_start);
    if (content_end == std::string::npos) break;

    // Extract content, trim leading/trailing newlines
    std::string content = text.substr(content_start, content_end - content_start);
    if (!content.empty() && content.front() == '\n') content.erase(0, 1);
    if (!content.empty() && content.back() == '\n') content.pop_back();

    actions.push_back({path, content});
    pos = content_end + close_tag.size();
  }
  return actions;
}

// Replace annotations with [proposed: write path] summaries
// Leaves non-annotation text intact
std::string strip_annotations(const std::string& text) {
  std::string result = text;
  const std::string open_prefix = "<write file=\"";
  const std::string close_tag = "</write>";

  while (true) {
    auto start = result.find(open_prefix);
    if (start == std::string::npos) break;

    // Extract path for summary
    auto path_start = start + open_prefix.size();
    auto path_end = result.find("\"", path_start);
    if (path_end == std::string::npos) break;
    std::string path = result.substr(path_start, path_end - path_start);

    // Find closing tag
    auto end = result.find(close_tag, start);
    if (end == std::string::npos) break;
    end += close_tag.size();

    // Replace entire annotation with summary
    result.replace(start, end - start, "[proposed: write " + path + "]");
  }
  return result;
}
