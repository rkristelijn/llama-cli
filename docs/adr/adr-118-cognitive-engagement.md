---
summary: File content has been updated with recommendations for cognitive engagement while using AI-assisted development tools, emphasizing user review and modification of generated outputs, and incorporating anti-patterns to avoid in the UX design.
status: accepted
---

# ADR-118: Cognitive Engagement — Learning While Using AI

## Context

Research ([arxiv:2601.20245](https://arxiv.org/abs/2601.20245)) shows AI assistance impairs skill formation when users fully delegate. However, 3 of 6 identified interaction patterns preserve learning:

1. **Verify-then-adapt** — AI generates, human reviews and modifies
2. **Scaffold-then-extend** — AI provides structure, human fills in details
3. **Interleave** — alternate between AI-assisted and manual work

Full delegation (copy-paste without understanding) showed productivity gains but at the cost of conceptual understanding, debugging ability, and code reading skills.

## Decision

llama-cli's design already encourages engagement via confirmation prompts (y/n/s on writes, exec). We reinforce this:

1. **Confirmation is non-negotiable** — never auto-apply without user review (except in trust mode, which is opt-in)
2. **Show diffs** — always show what changes before applying (str_replace shows old→new)
3. **Tips encourage learning** — periodic tips teach users to use the tool effectively
4. **Rating system** — /rate forces reflection on response quality

### Anti-patterns to avoid in our UX

- Don't make "accept all" the default
- Don't hide what the AI changed
- Don't discourage manual editing alongside AI suggestions

## Consequences

- Users build skills while using the tool (not despite it)
- Slightly slower workflow than full-auto, but sustainable skill growth
- Aligns with the project's philosophy: "AI does the work, human controls and verifies"
