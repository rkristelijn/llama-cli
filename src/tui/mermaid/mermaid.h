/**
 * @file mermaid.h
 * @brief Mermaid diagram rendering for terminal output.
 *
 * Parses a mermaid flowchart string and renders it as braille art
 * to an ostream. Used by the StreamRenderer when a ```mermaid block
 * is detected. Integrated from ../mermaid-tui/ project.
 *
 * @see docs/backlog/032-mermaid-rendering.md
 */
#ifndef LLAMA_CLI_MERMAID_H
#define LLAMA_CLI_MERMAID_H

#include <ostream>
#include <string>

namespace tui {

/// Render a mermaid diagram string as braille art to the given stream.
/// Returns true if rendering succeeded, false if parsing failed.
/// @param input  mermaid source (e.g. "graph TD; A-->B-->C")
/// @param out    output stream for the rendered diagram
/// @param cols   terminal width in columns (0 = auto-detect)
/// @param rows   terminal height in rows (0 = use 12 lines)
bool render_mermaid(const std::string& input, std::ostream& out, int cols = 0, int rows = 0);

}  // namespace tui

#endif  // LLAMA_CLI_MERMAID_H
