// repl.cpp — Interactive REPL loop with conversation memory and commands
// Supports slash commands (/read, /help, /clear) and regular prompts.
// Exits on "exit", "quit", or EOF.

#include "repl.h"

#include "command.h"

int run_repl(ChatFn chat, const std::string& system_prompt, std::istream& in, std::ostream& out) {
  int count = 0;
  std::string line;
  std::vector<Message> history;

  // Add system prompt as first message if provided
  if (!system_prompt.empty()) {
    history.push_back({"system", system_prompt});
  }

  while (true) {
    if (&in == &std::cin) out << "> " << std::flush;

    if (!std::getline(in, line)) break;
    if (line.empty()) continue;

    auto input = parse_input(line);

    if (input.type == InputType::Exit) break;

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

    // Regular prompt
    history.push_back({"user", line});
    std::string response = chat(history);
    out << response << "\n";
    history.push_back({"assistant", response});
    count++;
  }
  return count;
}
