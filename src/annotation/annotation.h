// Tool extracts and processes LLM response annotations.

#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <string>
#include <vector>

/// A proposed full-file write extracted from LLM response
struct WriteAction {
  std::string path;     ///< Target file path
  std::string content;  ///< File content to write
};

/// A proposed targeted string replacement in an existing file
struct StrReplaceAction {
  std::string path;     ///< Target file path
  std::string old_str;  ///< Exact string to find and replace
  std::string new_str;  ///< Replacement string
};

/// A read request: either line range or search term
struct ReadAction {
  std::string path;    ///< File to read
  int from_line = 0;   ///< 1-based start line (0 = not set)
  int to_line = 0;     ///< 1-based end line (0 = not set)
  std::string search;  ///< Search term (empty = not set)
};

/// A web search query extracted from LLM response (ADR-057)
struct SearchAction {
  std::string query;  ///< Search query string
};

// Extract all <write> annotations from response text
std::vector<WriteAction> parse_write_annotations(const std::string& text);

// Extract all <str_replace> annotations from response text
std::vector<StrReplaceAction> parse_str_replace_annotations(const std::string& text);

// Extract all <read> annotations from response text
std::vector<ReadAction> parse_read_annotations(const std::string& text);

// Extract all <search> annotations from response text (ADR-057)
std::vector<SearchAction> parse_search_annotations(const std::string& text);

// Remove write/str_replace/read annotations from text, replacing with summary
std::string strip_annotations(const std::string& text);

// Auto-repair malformed closing tags (e.g. <exec>...</bash> → <exec>...</exec>)
std::string fix_malformed_tags(const std::string& text);

#endif
