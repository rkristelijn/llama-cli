# 018: Reduce Complexity

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#59](https://github.com/rkristelijn/llama-cli/issues/59)

## Problem

`show_diff` and `confirm_write` in `src/repl/repl.cpp` exceed cyclomatic complexity threshold (< 10). `json_test.cpp` has pmccabe parsing errors, currently suppressed with `pmccabe:skip-complexity`.

## Idea

- Extract sub-functions from `show_diff` and `confirm_write`
- Fix pmccabe parsing in test files
- Remove `pmccabe:skip-complexity` suppressions once fixed
