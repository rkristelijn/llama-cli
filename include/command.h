// command.h — REPL slash command parser
// Parses and executes /commands in interactive mode.

#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

#include "ollama.h"

// Result of parsing a line of input
enum class InputType { Prompt, Command, Exit };

struct ParsedInput {
  InputType type;
  std::string command;  // e.g. "read"
  std::string arg;      // e.g. "main.cpp"
};

// Parse a line of user input into a command or prompt
ParsedInput parse_input(const std::string& line);

// Execute a /read command: read file and add to history
// Returns true on success, false on error
bool cmd_read(const std::string& path, std::vector<Message>& history, std::ostream& out);

#endif
