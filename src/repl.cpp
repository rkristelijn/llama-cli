// repl.cpp — Interactive REPL loop with conversation memory
// Maintains message history and sends full context with each request.
// Exits on "exit", "quit", or EOF.

#include "repl.h"

int run_repl(ChatFn chat, std::istream& in, std::ostream& out) {
  int count = 0;
  std::string line;
  std::vector<Message> history;

  while (true) {
    if (&in == &std::cin) out << "> " << std::flush;

    if (!std::getline(in, line)) break;
    if (line == "exit" || line == "quit") break;
    if (line.empty()) continue;

    // Add user message to history
    history.push_back({"user", line});

    // Send full history to get contextual response
    std::string response = chat(history);
    out << response << "\n";

    // Add assistant response to history
    history.push_back({"assistant", response});
    count++;
  }
  return count;
}
