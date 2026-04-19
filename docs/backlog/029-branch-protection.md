# 029: Branch Protection (Peer Review)

*Status*: Idea · *Date*: 2026-04-19 · *CMMI*: 1.6 · *Issue*: [#93](https://github.com/rkristelijn/llama-cli/issues/93)

## Problem

CI runs on PRs but no branch protection rules enforce peer review before merge.

## Idea

Enable GitHub branch protection on `main`: require PR, require CI pass, require 1 approval.

### References

- [ADR-048 §3.3](../adr/adr-048-quality-framework.md#33-cmmi-1--managed-first-release) — CMMI 1 check 1.6
- [Prompt: 07-branch-protection](../prompts/07-branch-protection.md)
