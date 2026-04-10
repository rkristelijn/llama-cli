// annotation.h — Parse LLM tool annotations from response text
// Extracts <write file="path">content</write> tags.

#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <string>
#include <vector>

struct WriteAction {
  std::string path;
  std::string content;
};

// Extract all <write> annotations from response text
std::vector<WriteAction> parse_write_annotations(const std::string& text);

// Remove annotations from text, replacing with summary
std::string strip_annotations(const std::string& text);

#endif
