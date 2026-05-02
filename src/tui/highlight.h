/**
 * @file highlight.h
 * @brief Pluggable syntax highlighting for code blocks.
 *
 * Strategy pattern: SyntaxHighlighter is the interface, with a built-in
 * regex highlighter and an optional external tool adapter (bat/pygmentize).
 * The active highlighter is selected at startup based on tool availability.
 *
 * @see docs/backlog/041-tree-sitter-highlighting.md
 */
#ifndef LLAMA_CLI_HIGHLIGHT_H
#define LLAMA_CLI_HIGHLIGHT_H

#include <memory>
#include <string>

namespace tui {

/// Strategy interface for syntax highlighting a code block.
/// Implementations receive the full code block content and language tag,
/// and return ANSI-colored output.
class SyntaxHighlighter {
 public:
  virtual ~SyntaxHighlighter() = default;
  /// Highlight a single line of code. Returns ANSI-colored string.
  virtual std::string highlight_line(const std::string& line, const std::string& lang) const = 0;
  /// Human-readable name for /set or diagnostics
  virtual std::string name() const = 0;
};

/// Built-in regex highlighter — keywords, strings, comments, numbers.
/// Supports: cpp, python, bash/sh, javascript/js/typescript/ts, rust, go.
class RegexHighlighter : public SyntaxHighlighter {
 public:
  std::string highlight_line(const std::string& line, const std::string& lang) const override;
  std::string name() const override { return "builtin"; }
};

/// External tool adapter — pipes code through bat or pygmentize.
/// Falls back to RegexHighlighter if the tool is not available.
class ExternalHighlighter : public SyntaxHighlighter {
 public:
  /// Construct with tool path (e.g. "/usr/bin/bat")
  explicit ExternalHighlighter(std::string tool_path, std::string tool_name);
  std::string highlight_line(const std::string& line, const std::string& lang) const override;
  std::string name() const override { return tool_name_; }

 private:
  std::string tool_path_;
  std::string tool_name_;
  RegexHighlighter fallback_;
};

/// Detect available highlighter: tries bat, then pygmentize, then builtin.
std::unique_ptr<SyntaxHighlighter> detect_highlighter();

/// Get the global highlighter instance (lazy-initialized via detect_highlighter).
const SyntaxHighlighter& active_highlighter();

}  // namespace tui

#endif  // LLAMA_CLI_HIGHLIGHT_H
