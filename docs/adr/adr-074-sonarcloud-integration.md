# ADR-074: SonarCloud Integration and Local Check Coverage

*Status*: Accepted

## Date

2026-05-04

## Context

We added SonarCloud (free for open source) as a cloud-based static analysis tool for C++ and shell scripts. SonarQube Developer Edition was initially tried locally via Docker but requires a paid license for C++ support. SonarCloud provides the same analysis for free on public repositories.

The first scan reported 886 issues. Most are code smells, but some overlap with what local tools could catch. This ADR documents the integration, maps SonarCloud rules to local tooling, and identifies where we can shift-left.

## Decision

### Use SonarCloud for deep analysis, local tools for fast feedback

SonarCloud runs automatically on every push via GitHub App integration. Local tools (`make check`) remain the primary quality gate — SonarCloud adds a second layer for issues that local tools miss.

### Rule coverage mapping

| Sonar Rule | Severity | Description | Local tool | Status |
|---|---|---|---|---|
| S912 | Blocker | Side effects in `&&`/`||` operands | — | No local equivalent; manual review |
| S1912 | Blocker | Non-reentrant functions (gmtime) | `concurrency-mt-unsafe` | Added to clang-tidy |
| S2612 | Vulnerability | File permissions too broad (0755) | — | No local check; fixed manually |
| S7040 | Major | C++23 escape syntax `\o{33}` | — | Suppressed (we target C++17) |
| S134 | Critical | Deep nesting | `readability-function-cognitive-complexity` | Already covered ✓ |
| S3776 | Critical | Cognitive complexity | `readability-function-cognitive-complexity` | Already covered ✓ |
| S6003 | Major | Use `std::string_view` | `modernize-use-string-view` | Not enabled (C++17 partial) |
| S6009 | Major | Use range-based for | `modernize-loop-convert` | Not enabled (style choice) |
| S6004 | Major | Use `std::any_of`/`std::all_of` | — | Not enabled (readability) |
| S1135 | Minor | TODO comments | `make todo` | Already tracked ✓ |
| S7034 | Major | Use `auto` for type deduction | — | Not enabled (ADR-053: conflicts with simple C++) |

### Suppressed rules (with rationale)

Configured in `sonar-project.properties`:

- **S7040** — C++23 octal escape syntax. We target C++17; `\033[` is the standard way to write ANSI escapes.

### Rules NOT added locally (with rationale)

- **S912** — No reliable clang-tidy check exists for side effects in logical operators. Too rare to justify a custom script.
- **S1912** — `concurrency-mt-unsafe` catches this but is too noisy for a single-threaded CLI (flags `getenv`, `setenv`, `std::cout`). The gmtime→gmtime_r fix was applied manually; future occurrences are caught by SonarCloud.
- **S2612** — File permission checks are OS-specific. SonarCloud catches these; adding a grep-based script would be brittle.
- **S6003/S6009/S6004** — Modernize suggestions conflict with "simple C++ for beginners" philosophy (CONTRIBUTING.md).

### Rules added locally

None — all SonarCloud-specific rules are either too noisy or lack a reliable local equivalent. The existing clang-tidy config already covers cognitive complexity (S134/S3776).

## Consequences

- SonarCloud provides a dashboard at <https://sonarcloud.io/project/overview?id=rkristelijn_llama-cli>
- `make sonar-report` prints a CLI summary of open issues
- `make sonar` runs a manual local scan (requires `SONAR_TOKEN` in `.env`)
- Local Docker SonarQube setup removed from `docker-compose.yml`
- Blockers and vulnerabilities are fixed immediately; code smells are triaged per sprint

## References

- @see `sonar-project.properties` — rule exclusions
- @see `.config/.clang-tidy` — local static analysis config
- @see `scripts/security/sonar-report.sh` — CLI report script
- @see ADR-053 (C++ static analysis coverage)
- @see ADR-048 (quality framework)
