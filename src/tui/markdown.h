/**
 * @file markdown.h
 * @brief Markdown rendering functions and StreamRenderer class.
 *
 * SRP: All markdown parsing/rendering lives here. tui.h keeps only
 * trivial color helpers.
 *
 * @see docs/adr/adr-052-markdown-renderer.md
 * @see docs/adr/adr-066-solid-refactoring.md
 */
#ifndef LLAMA_CLI_MARKDOWN_H
#define LLAMA_CLI_MARKDOWN_H

#include <iostream>
#include <string>
#include <vector>

namespace tui {

// --- Inline formatting ---
bool is_horizontal_rule(const std::string& line);
bool try_strikethrough(const std::string& line, size_t& i, std::string& out);
bool try_bold_italic(const std::string& line, size_t& i, std::string& out);
bool try_link(const std::string& line, size_t& i, std::string& out);
bool try_bold(const std::string& line, size_t& i, std::string& out);
bool try_italic(const std::string& line, size_t& i, std::string& out);
bool try_code(const std::string& line, size_t& i, std::string& out);
std::string render_inline(const std::string& line, bool color);

// --- Block-level rendering ---
std::string try_heading(const std::string& line);
std::string try_list(const std::string& line, bool color);

// --- Table rendering (see tui/table.h) ---

// --- Full rendering ---
std::string render_line(const std::string& line, bool color);
std::string render_code_block_line(const std::string& line, bool& in_code_block);
std::string render_lines(const std::string& text, bool color);
std::string render_markdown(const std::string& text, bool color);

}  // namespace tui

/** Streaming markdown renderer — processes tokens as they arrive.
 * Buffers partial lines and renders block-level elements on newline. */
class StreamRenderer {
 public:
  /** Construct a streaming renderer.
   * @param out Output stream to write rendered tokens to.
   * @param color If true, apply ANSI styling. */
  explicit StreamRenderer(std::ostream& out, bool color);
  /** Process a token (may contain partial lines, newlines, etc.) */
  void write(const std::string& token);
  /** Flush any remaining buffered content (call at end of stream) */
  void finish();

 private:
  void flush_line();
  void flush_line_plain(const std::string& content, const std::string& line);
  void flush_table_buf();
  void render_content_line(const std::string& content);

  std::ostream& out_;
  bool color_;
  std::string buf_;
  std::vector<std::string> table_buf_;
  bool in_code_block_ = false;
};

#endif  // LLAMA_CLI_MARKDOWN_H
