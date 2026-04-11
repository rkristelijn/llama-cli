# LLM annotations: write y/n/s, exec y/n

Feature: Annotations

  Scenario: Write annotation with confirm, skip, and show
    Given the LLM responds with a write annotation
    When the user confirms with "y"
    Then the file is written
    When the user declines with "n"
    Then the file is not written
    When the user types "s" then "y"
    Then the content is previewed and the file is written

  Scenario: Exec annotation with confirm and skip
    Given the LLM responds with an exec annotation
    When the user confirms with "y"
    Then the command output is shown
    And the LLM receives a follow-up with the output
    When the user declines with "n"
    Then "[skipped]" is shown
