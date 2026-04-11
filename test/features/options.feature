# Runtime options: /set, toggles, --no-color, --why-so-serious

Feature: Options

  Scenario: Set shows all options
    When the user runs "/set"
    Then the output contains "markdown", "color", and "bofh"

  Scenario: Set toggles color and bofh
    When the user runs "/set color" then "/set bofh" then "/set"
    Then color is on and bofh is on

  Scenario: Set unknown option shows error
    When the user runs "/set foobar"
    Then the output contains "Unknown option"

  Scenario: Runtime options persist across prompts
    Given the LLM echoes user messages
    When the user runs "/set markdown" then sends "**bold**"
    Then the output contains raw "**bold**" (not ANSI)

  Scenario: BOFH mode via --why-so-serious
    Given config with bofh enabled
    When the user checks /set
    Then bofh shows as on

  Scenario: No-color config flag
    Given config with no_color enabled
    When the user checks /set
    Then color shows as off
