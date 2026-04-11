# Chat conversation flows

Feature: Conversation

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

  Scenario: Clear history resets conversation
    Given the LLM echoes user messages
    When the user sends "remember this" then "/clear" then "what did I say?"
    Then the LLM does not see "remember this" in history
