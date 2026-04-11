/**
 * @file command.cpp
 * @brief REPL slash-command parser and dispatch.
 * @see docs/adr/adr-013-file-reading.md
 * @see docs/adr/adr-015-command-execution.md
 */

#include "command.h"

#include <algorithm>
#include <fstream>
#include <sstream>

// Check if line is an exit keyword (exit, quit, /exit, /quit)
static bool is_exit(const std::string& line) {
  return line == "exit" || line == "quit" || line == "/exit" || line == "/quit";
}

// Parse a slash command: extract command name and argument
// Splits "/command arg" into {Command, "command", "arg"}
static ParsedInput parse_slash(const std::string& line) {
  auto space = line.find(' ');
  std::string cmd = (space != std::string::npos) ? line.substr(1, space - 1) : line.substr(1);
  std::string arg = (space != std::string::npos) ? line.substr(space + 1) : "";
  return {InputType::Command, cmd, arg};
}

// Parse a line of user input into a typed result
// Detects !! (exec+context), ! (exec), exit, slash commands, prompts
ParsedInput parse_input(const std::string& line) {
  if (is_exit(line)) {
    return {InputType::Exit, "", ""};
  }
  if (line.size() > 2 && line[0] == '!' && line[1] == '!') {
    return {InputType::ExecContext, "", line.substr(2)};
  }
  if (line.size() > 1 && line[0] == '!') {
    return {InputType::Exec, "", line.substr(1)};
  }
  if (!line.empty() && line[0] == '/') {
    return parse_slash(line);
  }
  return {InputType::Prompt, "", line};
}

// Execute /read: load file contents into conversation history
// Adds file as a user message so the LLM has it as context
// Returns true on success, false if file cannot be opened
bool cmd_read(const std::string& path, std::vector<Message>& history, std::ostream& out) {
  std::ifstream file(path);
  if (!file.is_open()) {
    out << "Error: could not read " << path << "\n";
    return false;
  }

  // Read entire file
  std::ostringstream buf;
  buf << file.rdbuf();
  std::string content = buf.str();

  // Count lines for user feedback
  int lines = std::count(content.begin(), content.end(), '\n');

  // Add file as user message so the LLM has it as context
  history.push_back({"user", "[file: " + path + "]\n" + content});
  out << "[loaded " << path << " — " << lines << " lines]\n";
  return true;
}
