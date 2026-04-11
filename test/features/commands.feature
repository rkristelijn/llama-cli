# REPL commands: !, !!, /help, /version, unknown, empty, quit

Feature: Commands

  Scenario: Shell command with ! and !!
    When the user runs "!echo direct"
    Then "direct" is shown but not in LLM history
    When the user runs "!!echo context"
    Then "context" is shown and added to LLM history

  Scenario: Version and help commands
    When the user runs "/version"
    Then the output contains "llama-cli"
    When the user runs "/help"
    Then the output contains "/set" and "!command"

  Scenario: Unknown command shows error
    When the user runs "/foobar"
    Then the output contains "Unknown command"

  Scenario: Empty lines are skipped
    When the user sends empty lines between prompts
    Then no LLM calls are made for empty lines

  Scenario: Exit and quit both work
    When the user types "quit"
    Then the REPL exits cleanly
