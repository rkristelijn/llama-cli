// repl.cpp — Interactive REPL loop with conversation memory and commands
// Supports slash commands, tool annotations, and regular prompts.
// Exits on "exit", "quit", or EOF.
// See ADR-012 (REPL), ADR-013 (commands), ADR-014 (annotations).

#include "repl.h"

#include <fstream>

#include "annotation.h"
#include "command.h"

// Handle a slash command, return true if handled
static bool handle_command(const ParsedInput& input, std::vector<Message>& history, std::ostream& out) {
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
  return true;
}

// Prompt user for confirmation, return true if confirmed
// Supports "s"/"show" to preview content first
static bool confirm_write(const WriteAction& action, std::istream& in, std::ostream& out) {
  out << "Write to " << action.path << "? [y/n/s] " << std::flush;
  std::string answer;
  if (!std::getline(in, answer)) {
    return false;
  }
  if (answer == "s" || answer == "show") {
    out << action.content << "\n";
    out << "Write to " << action.path << "? [y/n] " << std::flush;
    if (!std::getline(in, answer)) {
      return false;
    }
  }
  return answer == "y" || answer == "yes";
}

// Process a single write action with user confirmation
// Writes file on "y"/"yes", skips otherwise
static void process_write(const WriteAction& action, std::istream& in, std::ostream& out) {
  if (confirm_write(action, in, out)) {
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

// Handle LLM response: check for annotations, display, process writes
// If annotations found, show stripped response and prompt for each write
static void handle_response(const std::string& response, std::istream& in, std::ostream& out) {
  auto actions = parse_write_annotations(response);
  if (actions.empty()) {
    out << response << "\n";
    return;
  }
  out << strip_annotations(response) << "\n";
  for (const auto& action : actions) {
    process_write(action, in, out);
  }
}

// Read one line of input, showing prompt for interactive terminals
// Returns false on EOF (signals loop exit)
static bool read_line(std::istream& in, std::ostream& out, std::string& line) {
  if (&in == &std::cin) {
    out << "> " << std::flush;
  }
  return static_cast<bool>(std::getline(in, line));
}

/// REPL session state — groups related data to reduce parameter passing
struct ReplState {
  ChatFn& chat;                   ///< Chat function for LLM interaction
  std::vector<Message>& history;  ///< Conversation history
  std::istream& in;               ///< Input stream
  std::ostream& out;              ///< Output stream
  int count = 0;                  ///< Number of prompts processed
};

// Dispatch a single REPL input: command, prompt, or exit
// Returns false if the loop should exit
static bool dispatch(const std::string& line, ReplState& s) {
  auto input = parse_input(line);
  if (input.type == InputType::Exit) {
    return false;
  }
  if (input.type == InputType::Command) {
    handle_command(input, s.history, s.out);
    return true;
  }
  // Send prompt to LLM and handle response
  s.history.push_back({"user", line});
  std::string response = s.chat(s.history);
  handle_response(response, s.in, s.out);
  s.history.push_back({"assistant", response});
  s.count++;
  return true;
}

// Main REPL loop: read → parse → dispatch → respond
// Returns number of LLM prompts processed
int run_repl(ChatFn chat, const std::string& system_prompt, std::istream& in, std::ostream& out) {
  std::string line;
  std::vector<Message> history;

  if (!system_prompt.empty()) {
    history.push_back({"system", system_prompt});
  }

  ReplState state = {chat, history, in, out, 0};

  // Read-eval-print loop until EOF or exit command
  while (read_line(in, out, line)) {
    if (line.empty()) {
      continue;
    }
    if (!dispatch(line, state)) {
      break;
    }
  }
  return state.count;
}
