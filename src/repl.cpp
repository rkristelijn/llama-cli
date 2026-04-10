// repl.cpp — Interactive REPL loop with conversation memory and commands
// Supports slash commands, tool annotations, and regular prompts.
// Exits on "exit", "quit", or EOF.
// See ADR-012 (REPL), ADR-013 (commands), ADR-014 (annotations).

#include "repl.h"

#include <fstream>

#include "annotation.h"
#include "command.h"

int run_repl(ChatFn chat, const std::string& system_prompt, std::istream& in, std::ostream& out) {
  int count = 0;
  std::string line;
  std::vector<Message> history;

  // Initialize conversation with system prompt (ADR-012)
  if (!system_prompt.empty()) {
    history.push_back({"system", system_prompt});
  }

  // Main REPL loop: read input, process, respond
  while (true) {
    // Show prompt indicator only in interactive terminal
    if (&in == &std::cin) out << "> " << std::flush;

    // EOF (Ctrl+D) or stream end exits cleanly
    if (!std::getline(in, line)) break;
    if (line.empty()) continue;

    // Parse input: exit, slash command, or regular prompt
    auto input = parse_input(line);

    if (input.type == InputType::Exit) break;

    // Handle slash commands (ADR-013)
    if (input.type == InputType::Command) {
      if (input.command == "read") {
        cmd_read(input.arg, history, out);
      } else if (input.command == "clear") {
        history.clear();
        out << "[history cleared]\n";
      } else if (input.command == "help") {
        out << "Commands:\n";
        out << "  /read <file>  Load file into context\n";
        out << "  /clear        Clear conversation history\n";
        out << "  /help         Show this help\n";
        out << "  exit, quit    Exit the REPL\n";
      } else {
        out << "Unknown command: /" << input.command << ". Type /help for options.\n";
      }
      continue;
    }

    // Send prompt with full conversation history to LLM
    history.push_back({"user", line});
    std::string response = chat(history);

    // Check for tool annotations in response (ADR-014)
    auto actions = parse_write_annotations(response);
    if (!actions.empty()) {
      // Show response with annotations replaced by summaries
      out << strip_annotations(response) << "\n";

      // Process each proposed write with user confirmation
      for (const auto& action : actions) {
        out << "Write to " << action.path << "? [y/n/s] " << std::flush;
        std::string answer;
        if (!std::getline(in, answer)) break;
        // Show content before confirming if requested
        if (answer == "s" || answer == "show") {
          out << action.content << "\n";
          out << "Write to " << action.path << "? [y/n] " << std::flush;
          if (!std::getline(in, answer)) break;
        }
        // Write file on confirmation
        if (answer == "y" || answer == "yes") {
          std::ofstream file(action.path);
          if (file.is_open()) {
            file << action.content << "\n";
            out << "[wrote " << action.path << "]\n";
          } else {
            out << "Error: could not write to " << action.path << "\n";
          }
        } else {
          out << "[skipped]\n";
        }
      }
    } else {
      out << response << "\n";
    }

    // Store response in history for conversation memory
    history.push_back({"assistant", response});
    count++;
  }
  return count;
}
