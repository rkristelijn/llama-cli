# Integration tests — full REPL session flows
# These scenarios are implemented in test_integration.cpp

Feature: REPL integration

  Scenario: Full conversation with history
    Given the LLM echoes user messages
    When the user sends "hello" then "what did I say?"
    Then the LLM sees both messages in history
    And both responses are printed

  Scenario: Command chaining — read file then ask about it
    Given the LLM echoes user messages
    When the user runs "!!echo file content here"
    And then asks "what was in the file?"
    Then the LLM sees the command output in history

  Scenario: Runtime options persist across prompts
    Given the LLM echoes user messages
    When the user runs "/set markdown" then sends "**bold**"
    Then the output contains raw "**bold**" (not ANSI)

  Scenario: Version and help commands
    When the user runs "/version"
    Then the output contains "llama-cli"
    When the user runs "/help"
    Then the output contains "/set" and "!command"

  Scenario: Clear history resets conversation
    Given the LLM echoes user messages
    When the user sends "remember this" then "/clear" then "what did I say?"
    Then the LLM does not see "remember this" in history

  Scenario: Write annotation with confirm and skip
    Given the LLM responds with a write annotation
    When the user confirms with "y"
    Then the file is written
    When the user declines with "n"
    Then the file is not written

  Scenario: Exec annotation with confirm and skip
    Given the LLM responds with an exec annotation
    When the user confirms with "y"
    Then the command output is shown
    And the LLM receives a follow-up with the output
    When the user declines with "n"
    Then "[skipped]" is shown

  Scenario: Unknown command shows error
    When the user runs "/foobar"
    Then the output contains "Unknown command"

  Scenario: Shell command with ! and !!
    When the user runs "!echo direct"
    Then "direct" is shown but not in LLM history
    When the user runs "!!echo context"
    Then "context" is shown and added to LLM history
