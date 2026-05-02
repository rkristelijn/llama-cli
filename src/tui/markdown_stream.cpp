/**
 * @file markdown_stream.cpp
 * @brief StreamRenderer implementation — token-by-token markdown rendering.
 *
 * Processes streaming LLM tokens, buffering partial lines and rendering
 * block-level elements on newline. Annotation tags are suppressed.
 *
 * @see docs/adr/adr-071-markdown-module-split.md
 * @see docs/adr/adr-052-markdown-renderer.md
 */

#include "tui/highlight.h"
#include "tui/markdown.h"
#include "tui/mermaid/renderer.h"
#include "tui/table.h"
#include "tui/tui.h"

// --- StreamRenderer implementation ---
// Processes tokens character-by-character, buffering until newline.
// On newline: renders the complete line using block-level markdown rules.
// Annotation tags (<exec>, <write>, etc.) are suppressed from output.

StreamRenderer::StreamRenderer(std::ostream& out, bool color) : out_(out), color_(color) {}

void StreamRenderer::write(const std::string& token) {
  try {
    for (char c : token) {
      buf_ += c;
      if (c == '\n') {
        flush_line();
      }
    }
  } catch (...) {
    out_ << token;
  }
}

void StreamRenderer::finish() {
  try {
    flush_table_buf();
    if (!buf_.empty()) {
      buf_ += '\n';
      flush_line();
    }
    if (in_code_block_) {
      out_ << "\033[0m";
    }
  } catch (...) {
    out_ << buf_;
    buf_.clear();
  }
}

void StreamRenderer::flush_line() {
  std::string line = buf_;
  buf_.clear();
  std::string content = line.substr(0, line.size() - 1);

  // Hide annotation tags — processed separately after streaming
  if (content.find("<exec>") != std::string::npos || content.find("</exec>") != std::string::npos ||
      content.find("<write ") != std::string::npos || content.find("</write>") != std::string::npos ||
      content.find("<str_replace ") != std::string::npos || content.find("</str_replace>") != std::string::npos ||
      content.find("<add_line ") != std::string::npos || content.find("<delete_line ") != std::string::npos ||
      content.find("<read ") != std::string::npos || content.find("<search>") != std::string::npos ||
      content.find("</search>") != std::string::npos) {
    return;
  }

  // Plain mode: no ANSI rendering, just buffer tables
  if (!color_) {
    flush_line_plain(content, line);
    return;
  }
  // Buffer table rows until a non-table line triggers rendering
  if (!in_code_block_ && tui::is_table_row(content)) {
    table_buf_.push_back(content);
    return;
  }
  flush_table_buf();

  // Toggle code fence state and extract language tag
  if (content.size() >= 3 && content.substr(0, 3) == "```") {
    if (!in_code_block_) {
      // Opening fence — extract language tag after ```
      code_lang_ = content.size() > 3 ? content.substr(3) : "";
      while (!code_lang_.empty() && code_lang_[0] == ' ') {
        code_lang_.erase(0, 1);
      }
      while (!code_lang_.empty() && code_lang_.back() == ' ') {
        code_lang_.pop_back();
      }
      mermaid_buf_.clear();
      if (code_lang_ == "mermaid") {
        // Don't print opening fence yet — wait for render result
        in_code_block_ = true;
        return;
      }
    } else {
      // Closing fence — render mermaid if buffered
      if (code_lang_ == "mermaid" && !mermaid_buf_.empty()) {
        if (tui::diagram_registry().render(mermaid_buf_, out_)) {
          // Success — diagram already written to out_
        } else {
          // Fallback: show as normal code block
          out_ << "\033[2m```mermaid\n\033[0m";
          out_ << mermaid_buf_;
          out_ << "\033[2m```\n\033[0m";
        }
        mermaid_buf_.clear();
        code_lang_.clear();
        in_code_block_ = false;
        return;
      }
      code_lang_.clear();
    }
    in_code_block_ = !in_code_block_;
    out_ << "\033[2m" << line << "\033[0m";
    return;
  }
  if (in_code_block_) {
    if (code_lang_ == "mermaid") {
      // Buffer mermaid content for rendering on closing fence
      mermaid_buf_ += content + "\n";
    } else {
      out_ << tui::active_highlighter().highlight_line(content, code_lang_) << "\n";
    }
    return;
  }
  render_content_line(content);
}

void StreamRenderer::flush_line_plain(const std::string& content, const std::string& line) {
  if (tui::is_table_row(content)) {
    table_buf_.push_back(content);
    return;
  }
  flush_table_buf();
  out_ << line;
}

void StreamRenderer::flush_table_buf() {
  if (!table_buf_.empty()) {
    out_ << tui::render_table(table_buf_, color_);
    table_buf_.clear();
  }
}

void StreamRenderer::render_content_line(const std::string& content) {
  if (!content.empty() && content[0] == '#') {
    std::string h = tui::try_heading(content);
    if (!h.empty()) {
      out_ << h;
      return;
    }
  }
  if (tui::is_horizontal_rule(content)) {
    out_ << "\033[2m────────────────────\033[0m\n";
    return;
  }
  if (!content.empty() && content[0] == '>') {
    std::string bq = (content.size() > 1 && content[1] == ' ') ? content.substr(2) : content.substr(1);
    out_ << "\033[2m│\033[0m " << tui::render_inline(bq, color_) << "\n";
    return;
  }
  std::string li = tui::try_list(content, color_);
  if (!li.empty()) {
    out_ << li;
    return;
  }
  out_ << tui::render_inline(content, color_) << "\n";
}
