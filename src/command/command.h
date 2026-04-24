// Parses user input for interactive mode commands and executes specified actions.
// Parses and executes /commands in interactive mode.

#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

#include "ollama/ollama.h"

// Result of parsing a line of input
enum class InputType { Prompt, Command, Exit, Exec, ExecContext };

/// Result of parsing a line of user input
struct ParsedInput {
  InputType type;       ///< What kind of input was parsed
  std::string command;  ///< Command name, e.g. "read" (empty for prompts)
  std::string arg;      ///< Command argument, e.g. "main.cpp" (empty if none)
};

// Parse a line of user input into a command or prompt
ParsedInput parse_input(const std::string& line);

// Execute a /read command: read file and add to history
// Returns true on success, false on error
bool cmd_read(const std::string& path, std::vector<Message>& history, std::ostream& out);

#endif
