# Task Status Convention

Standard for task checkboxes across this project. Based on the
[Taskwarrior](https://taskwarrior.org/)-inspired convention from
[tasks](https://github.com/rkristelijn/tasks/blob/main/docs/standards.md),
extended with a review state.

## Statuses

| Symbol | Status | Meaning | Taskwarrior |
|--------|--------|---------|-------------|
| `[ ]` | Open | Not started | `pending` |
| `[-]` | WIP | In progress, being worked on | `active` |
| `[>]` | Review | Done coding, awaiting review/merge | — |
| `[~]` | Blocked | Waiting on something external | `waiting` |
| `[x]` | Done | Completed | `completed` |
| `[/]` | Cancelled | Won't do | `deleted` |

### Rendering

- GitHub renders `[ ]` and `[x]` natively as checkboxes
- Obsidian renders all 6 with custom CSS (via `data-task` attribute)
- In plain text / other renderers, the symbol inside `[]` is self-explanatory

### Flow

```text
[ ] Open → [-] WIP → [>] Review → [x] Done
                 ↓                    ↑
                [~] Blocked ──────────┘
                 ↓
                [/] Cancelled
```

## Usage in this project

### TODO.md

```markdown
- [ ] Streaming responses
- [-] Fix make setup — installing missing tools
- [>] Smart confirmation prompt (PR open)
- [~] Branch protection — waiting on admin access
- [x] Pin tool versions in .config/versions.env
- [/] Migrate to new server — no longer needed
```

### docs/backlog/README.md

The backlog Status column uses the same vocabulary:

| Status | Meaning |
|--------|---------|
| Idea | `[ ]` — not started, under consideration |
| WIP | `[-]` — actively being worked on |
| Review | `[>]` — PR open or awaiting feedback |
| Blocked | `[~]` — waiting on dependency |
| Done | `[x]` — completed, has ADR or is shipped |
| Cancelled | `[/]` — won't do |

## Priority

```markdown
- [ ] !!! Critical task        (P0 — drop everything)
- [ ] !! High priority task (P1 — do today)
- [ ] ! Normal priority task (P2 — do this week)
- [ ]     Low priority task    (P3 — someday)
```

## References

- [tasks/docs/standards.md](https://github.com/rkristelijn/tasks/blob/main/docs/standards.md) — original convention
- [Sam Collins: Multi-State Checkboxes](https://memos.smcllns.com/checkboxes) — `[>]` convention for in-progress/on-track
- [Obsidian Tasks](https://publish.obsidian.md/tasks) — custom status rendering
- [GitHub Flavored Markdown](https://github.github.com/gfm/#task-list-items-extension-) — native `[ ]`/`[x]` only
