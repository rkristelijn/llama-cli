---
summary: Policy to allow Unicode characters and symbols in source code, except for emojis.
title: "ADR-102: Unicode Character Policy"
status: accepted
date: 2026-05-07
---

summary: Policy to allow Unicode characters and symbols in source code, except for emojis.

## ADR-102: Unicode Character Policy

### Context

Emoji render inconsistently across terminals, fonts, and platforms. Some terminals
show them double-width, some don't render them at all, and screen readers struggle
with them. We need a consistent policy for what characters are allowed in source,
scripts, and TUI output.

### Decision

**No emoji. Unicode symbols and kaomoji are allowed.**

#### Forbidden (emoji)

Codepoints U+1F300–U+1FFFF (Supplementary Multilingual Plane emoji):

```text
🖥️ 🎮 🧠 🔴 🟠 🟡 🔵 ⚪ ✗ ✓ 🎉 🔍 📄 🚀 💡 🐛 etc.
```

Enforced by `make inclusivity` (emoji policy check).

#### Allowed: Unicode Symbols

These render consistently in all monospace fonts and terminals:

##### Status indicators

```text
✓  U+2713  CHECK MARK
✗  U+2717  BALLOT X
✔  U+2714  HEAVY CHECK MARK
✘  U+2718  HEAVY BALLOT X
⚠  U+26A0  WARNING SIGN
●  U+25CF  BLACK CIRCLE
○  U+25CB  WHITE CIRCLE
◆  U+25C6  BLACK DIAMOND
◇  U+25C7  WHITE DIAMOND
→  U+2192  RIGHTWARDS ARROW
←  U+2190  LEFTWARDS ARROW
↑  U+2191  UPWARDS ARROW
↓  U+2193  DOWNWARDS ARROW
»  U+00BB  RIGHT DOUBLE ANGLE QUOTE
«  U+00AB  LEFT DOUBLE ANGLE QUOTE
```

##### Box drawing (TUI borders, tables)

```text
─  U+2500  HORIZONTAL
│  U+2502  VERTICAL
┌  U+250C  TOP LEFT
┐  U+2510  TOP RIGHT
└  U+2514  BOTTOM LEFT
┘  U+2518  BOTTOM RIGHT
├  U+251C  LEFT TEE
┤  U+2524  RIGHT TEE
┬  U+252C  TOP TEE
┴  U+2534  BOTTOM TEE
┼  U+253C  CROSS
═  U+2550  DOUBLE HORIZONTAL
║  U+2551  DOUBLE VERTICAL
╔  U+2554  DOUBLE TOP LEFT
╗  U+2557  DOUBLE TOP RIGHT
╚  U+255A  DOUBLE BOTTOM LEFT
╝  U+255D  DOUBLE BOTTOM RIGHT
```

##### Block elements (progress bars, charts)

```text
█  U+2588  FULL BLOCK
▓  U+2593  DARK SHADE
▒  U+2592  MEDIUM SHADE
░  U+2591  LIGHT SHADE
▏  U+258F  LEFT 1/8 BLOCK
▎  U+258E  LEFT 1/4 BLOCK
▍  U+258D  LEFT 3/8 BLOCK
▌  U+258C  LEFT HALF BLOCK
▋  U+258B  LEFT 5/8 BLOCK
▊  U+258A  LEFT 3/4 BLOCK
▉  U+2589  LEFT 7/8 BLOCK
```

##### Braille patterns (high-res graphs, sparklines)

U+2800–U+28FF — 256 patterns, each a 2x4 dot grid. Perfect for terminal graphics:

```text
⠀  U+2800  BLANK
⠁  U+2801  DOT-1
⠂  U+2802  DOT-2
⠄  U+2804  DOT-3
⡀  U+2840  DOT-7
⠈  U+2808  DOT-4
⠐  U+2810  DOT-5
⠠  U+2820  DOT-6
⢀  U+2880  DOT-8
⣿  U+28FF  ALL DOTS (full)
⡇  U+2847  LEFT COLUMN
⣸  U+28F8  RIGHT COLUMN
⣀  U+28C0  BOTTOM ROW
⠛  U+281B  TOP ROW
```

Use cases:

- Sparkline graphs: `⣀⣤⣶⣿⣿⣶⣤⣀` (bell curve)
- High-res bar charts (2x resolution vs block elements)
- Dot-matrix style rendering of diagrams
- Mini heatmaps in terminal output

##### Mathematical / logical

```text
×  U+00D7  MULTIPLICATION
÷  U+00F7  DIVISION
±  U+00B1  PLUS-MINUS
≈  U+2248  ALMOST EQUAL
≠  U+2260  NOT EQUAL
≤  U+2264  LESS THAN OR EQUAL
≥  U+2265  GREATER THAN OR EQUAL
∞  U+221E  INFINITY
∑  U+2211  SUMMATION
∆  U+2206  INCREMENT (delta)
√  U+221A  SQUARE ROOT
```

##### Miscellaneous useful

```text
·  U+00B7  MIDDLE DOT (separator)
…  U+2026  ELLIPSIS
—  U+2014  EM DASH
–  U+2013  EN DASH
†  U+2020  DAGGER (footnote)
§  U+00A7  SECTION SIGN
¶  U+00B6  PILCROW (paragraph)
©  U+00A9  COPYRIGHT
®  U+00AE  REGISTERED
™  U+2122  TRADEMARK
°  U+00B0  DEGREE
µ  U+00B5  MICRO (microseconds)
```

##### Menu / UI indicators

```text
☰  U+2630  TRIGRAM (hamburger menu)
⋮  U+22EE  VERTICAL ELLIPSIS (kebab menu)
⋯  U+22EF  HORIZONTAL ELLIPSIS (meatball menu)
⌘  U+2318  COMMAND KEY
⌥  U+2325  OPTION KEY
⇧  U+21E7  SHIFT KEY
⏎  U+23CE  RETURN SYMBOL
⌫  U+232B  DELETE/BACKSPACE
```

#### Allowed: Kaomoji

Text emoticons using standard Unicode characters. Preferred over emoji for personality:

```text
╰(*°▽°*)╯   celebration
(ノ°▽°)ノ    excitement
(¬_¬)        skepticism
(╥_╥)        sadness
¯\_(ツ)_/¯   shrug
(╯°□°)╯︵ ┻━┻  table flip
ᕕ( ᐛ )ᕗ     happy walk
ε(´סּ︵סּ`)з   sad
(－‸ლ)       facepalm
(*・‿・)ノ    wave
ヘ( ^o^)ノ＼(^_^ )  high five
```

### Replacement Guide

When replacing emoji in existing code:

| Context | Emoji | Replace with |
|---------|-------|-------------|
| Pass/success | ✓ | `✓` or `[ok]` |
| Fail/error | ✗ | `✗` or `[FAIL]` |
| Warning | ⚠ | `⚠` (U+26A0, already allowed) |
| Info | ℹ | `(i)` or `·` |
| Arrow/next | ➡️ | `→` |
| Search | 🔍 | `[?]` or `·` |
| Hardware | 🖥️🎮🧠 | Plain text: `CPU:` `GPU:` `RAM:` |
| Severity | 🔴🟠🟡🔵 | `[!!]` `[!]` `[~]` `[-]` |
| Celebration | 🎉 | kaomoji: `╰(*°▽°*)╯` |
| Thinking | 🤔 | kaomoji: `(¬_¬)` |

### Consequences

- Consistent rendering across all terminals and fonts
- Screen reader friendly (Unicode symbols have proper names)
- No double-width rendering issues
- Kaomoji add personality without accessibility problems
- Enforced by `make inclusivity` (emoji policy section)
