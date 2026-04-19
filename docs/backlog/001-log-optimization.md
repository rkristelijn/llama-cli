# 001: Log Optimization for Usage Analysis

*Status*: Idea · *Date*: 2026-04-19 · *Issue*: [#79](https://github.com/rkristelijn/llama-cli/issues/79)

## Problem

The event log ([ADR-027](../adr/adr-027-event-logging.md)) captures raw events but there is no tooling to analyze usage patterns and derive feature ideas from real behavior.

## Idea

Build analysis tooling on top of the JSONL event log to answer:

- Which commands are used most via `!!`?
- What are the most common prompt patterns?
- Where does the LLM spend the most tokens?
- Which sessions are longest / most expensive?

### Possible outputs

- **Usage dashboard** — aggregate stats per session, per day
- **Command frequency** — rank `!!` commands to prioritize wingman tips
- **Token heatmap** — identify expensive prompt patterns
- **Session replay summary** — condensed view of what happened

### Implementation sketch

```bash
# scripts/dev/log-analyze.sh
jq -r 'select(.action=="exec") | .input' ~/.llama-cli/events.jsonl | sort | uniq -c | sort -rn
```

## Feeds into

- [002 — Wingman: command tips](002-wingman-command-tips.md) — command frequency data drives tip selection
- [003 — Wingman: preflight](003-wingman-preflight.md) — prompt pattern data identifies fuzzy prompts

## References

- [ADR-027: Event Logging](../adr/adr-027-event-logging.md)
