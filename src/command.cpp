// command.cpp — REPL slash command parser and execution
// Handles /read, /help, /clear, and exit/quit detection.

#include "command.h"

#include <algorithm>
#include <fstream>
#include <sstream>

// Parse a line of user input into a command or prompt
// Detects exit keywords, slash commands, and regular prompts
ParsedInput parse_input(const std::string& line) {
  // Exit keywords
  if (line == "exit" || line == "quit" || line == "/exit" || line == "/quit") {
    return {InputType::Exit, "", ""};
  }

  // Slash commands: /command [arg]
  // Split on first space to separate command from argument
  if (!line.empty() && line[0] == '/') {
    auto space = line.find(' ');
    std::string cmd = (space != std::string::npos) ? line.substr(1, space - 1) : line.substr(1);
    std::string arg = (space != std::string::npos) ? line.substr(space + 1) : "";
    return {InputType::Command, cmd, arg};
  }

  return {InputType::Prompt, "", line};
}

// Execute /read: load file contents into conversation history
// Adds file as a user message so the LLM has it as context
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
