# Markdown rendering: renderer, on/off toggle

Feature: Markdown

  Scenario: Markdown renderer
    Given color is enabled
    Then headings strip # and render bold+underline
    And **bold** renders without asterisks
    And *italic* renders without asterisks
    And `code` renders without backticks
    And bullet lists use • symbol
    And numbered lists preserve numbers
    And code blocks preserve content

  Scenario: Markdown off returns raw text
    Given color is disabled
    Then text is returned unchanged

  Scenario: Markdown on vs off in REPL
    Given the LLM responds with markdown
    When markdown is off via /set
    Then raw markdown is preserved in output
