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
constexpr const char* ERROR_FILE_NOT_FOUND = "File not found";            ///< File does not exist
constexpr const char* ERROR_FILE_READ_FAILED = "Failed to read file";     ///< File read operation failed
constexpr const char* ERROR_FILE_WRITE_FAILED = "Failed to write file";   ///< File write operation failed
constexpr const char* ERROR_INVALID_PATH = "Invalid file path";           ///< Path is malformed or invalid
constexpr const char* ERROR_PERMISSION_DENIED = "Permission denied";      ///< Insufficient permissions
constexpr const char* ERROR_COMMAND_FAILED = "Command execution failed";  ///< Shell command failed
constexpr const char* ERROR_TIMEOUT = "Operation timed out";              ///< Operation exceeded timeout
constexpr const char* ERROR_CONNECTION_FAILED = "Connection failed";      ///< Network connection failed
constexpr const char* ERROR_MODEL_NOT_FOUND = "Model not found";          ///< LLM model not available

// Prompts
constexpr const char* PROMPT_CONFIRM = "Confirm? [y/n] ";                   ///< Generic confirmation prompt
constexpr const char* PROMPT_CONTINUE = "Continue? [y/n] ";                 ///< Continue operation prompt
constexpr const char* PROMPT_OVERWRITE = "File exists. Overwrite? [y/n] ";  ///< File overwrite confirmation
constexpr const char* PROMPT_SELECT_MODEL = "Select model: ";               ///< Model selection prompt
constexpr const char* PROMPT_SELECT_HOST = "Select host: ";                 ///< Host selection prompt

// Status messages
constexpr const char* STATUS_CONNECTING = "Connecting...";  ///< Connection in progress
constexpr const char* STATUS_LOADING = "Loading...";        ///< Loading operation
constexpr const char* STATUS_PROCESSING = "Processing...";  ///< Processing operation
constexpr const char* STATUS_COMPLETE = "Complete";         ///< Operation completed
constexpr const char* STATUS_CANCELLED = "Cancelled";       ///< Operation cancelled

// Info messages
constexpr const char* INFO_HELP = "Type 'help' for available commands";  ///< Help hint
constexpr const char* INFO_EXIT = "Type 'exit' to quit";                 ///< Exit hint
constexpr const char* INFO_VERSION = "Version: ";                        ///< Version prefix

}  // namespace messages

#endif  // LLAMA_CLI_UI_MESSAGES_H
