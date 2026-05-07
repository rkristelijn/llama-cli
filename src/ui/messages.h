/**
 * @file messages.h
 * @brief Centralized user-facing message strings for i18n support (ADR-104)
 *
 * All user-facing strings (errors, prompts, UI text) are defined here as constants.
 * This enables:
 * - Localization/i18n support
 * - Consistent terminology
 * - Easier testing and maintenance
 *
 * Naming convention: MSG_<CONTEXT>_<MESSAGE>
 * Example: MSG_ERROR_FILE_NOT_FOUND, MSG_PROMPT_CONFIRM_DELETE
 */

#ifndef LLAMA_CLI_UI_MESSAGES_H
#define LLAMA_CLI_UI_MESSAGES_H

namespace messages {

// Error messages
constexpr const char* ERROR_FILE_NOT_FOUND = "File not found";
constexpr const char* ERROR_FILE_READ_FAILED = "Failed to read file";
constexpr const char* ERROR_FILE_WRITE_FAILED = "Failed to write file";
constexpr const char* ERROR_INVALID_PATH = "Invalid file path";
constexpr const char* ERROR_PERMISSION_DENIED = "Permission denied";
constexpr const char* ERROR_COMMAND_FAILED = "Command execution failed";
constexpr const char* ERROR_TIMEOUT = "Operation timed out";
constexpr const char* ERROR_CONNECTION_FAILED = "Connection failed";
constexpr const char* ERROR_MODEL_NOT_FOUND = "Model not found";

// Prompts
constexpr const char* PROMPT_CONFIRM = "Confirm? [y/n] ";
constexpr const char* PROMPT_CONTINUE = "Continue? [y/n] ";
constexpr const char* PROMPT_OVERWRITE = "File exists. Overwrite? [y/n] ";
constexpr const char* PROMPT_SELECT_MODEL = "Select model: ";
constexpr const char* PROMPT_SELECT_HOST = "Select host: ";

// Status messages
constexpr const char* STATUS_CONNECTING = "Connecting...";
constexpr const char* STATUS_LOADING = "Loading...";
constexpr const char* STATUS_PROCESSING = "Processing...";
constexpr const char* STATUS_COMPLETE = "Complete";
constexpr const char* STATUS_CANCELLED = "Cancelled";

// Info messages
constexpr const char* INFO_HELP = "Type 'help' for available commands";
constexpr const char* INFO_EXIT = "Type 'exit' to quit";
constexpr const char* INFO_VERSION = "Version: ";

}  // namespace messages

#endif  // LLAMA_CLI_UI_MESSAGES_H
