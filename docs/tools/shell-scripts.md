# Shell Scripts

Best practices, conventions, and compliance checking for shell scripts in this project.

> **Conventions are enforced by ShellCheck and `scripts/check/lint-scripts.sh`.**
> See [CONTRIBUTING.md](../../CONTRIBUTING.md) for the summary rules.
> See [ADR-044](../adr/adr-044-tidy-boilerplate.md) for the rationale.

## Quick reference

| Rule | Convention | Source |
|------|-----------|--------|
| Shell | `bash` only (no `sh`, `zsh`, `fish`) | Google Shell Style Guide |
| Shebang | `#!/usr/bin/env bash` | Sharat's best practices |
| Safety flags | `set -o errexit`, `set -o nounset`, `set -o pipefail` | Google + Sharat |
| Debug support | `if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi` | Sharat's best practices |
| Indent | 2 spaces, no tabs | Google Shell Style Guide |
| Line length | 80 characters max | Google Shell Style Guide |
| File naming | kebab-case, `.sh` extension | OSS convention (K8s, Git, Docker) |
| Variable naming | `snake_case` for locals, `UPPER_CASE` for constants/env | Google Shell Style Guide |
| Function naming | `snake_case` | Google Shell Style Guide |
| Quoting | Always double-quote variables: `"${var}"` | Google Shell Style Guide |
| Conditions | `[[ ]]` not `[ ]` or `test` | Google Shell Style Guide |
| Linter | ShellCheck (all scripts must pass) | Google Shell Style Guide |
| Comments | ~20% ratio; file header + function headers required | Project convention |

## Script template

Every new script should start from this template:

```bash
#!/usr/bin/env bash
#
# brief-description.sh — One-line description of what this script does.
#
# Usage:
#   ./scripts/category/brief-description.sh [options]
#
# Environment:
#   TRACE=1    Enable debug tracing (set -o xtrace)
#
# @see docs/tools/shell-scripts.md — Shell script conventions
# @see docs/adr/adr-44-tidy-boilerplate.md — ADR for build boilerplate

set -o errexit   # Exit on error (same as set -e)
set -o nounset   # Error on unset variables (same as set -u)
set -o pipefail  # Pipe fails if any command fails

# Enable debug tracing when TRACE=1
if [[ "${TRACE-0}" == "1" ]]; then
  set -o xtrace
fi

#######################################
# Main entry point.
# Globals:
#   None
# Arguments:
#   $@ — all command-line arguments
# Outputs:
#   Writes status to stdout, errors to stderr
#######################################
main() {
  # Script logic here
  echo "done"
}

main "$@"
```

### Why this template?

- **`#!/usr/bin/env bash`** — more portable than `#!/bin/bash` across macOS/Linux/NixOS.
  Google recommends `#!/bin/bash` for internal consistency, but `env bash` is the safer
  default for open-source projects where bash may not be at `/bin/bash` (e.g., NixOS,
  FreeBSD, custom installs). We follow Sharat's recommendation here.
- **`set -o errexit`** — stops the script on the first error instead of silently continuing.
  Without this, a failed `cd` followed by `rm -rf *` could delete the wrong directory.
- **`set -o nounset`** — catches typos in variable names. Use `"${VAR-}"` for variables
  that may be unset (the `-` provides an empty default).
- **`set -o pipefail`** — ensures `cmd1 | cmd2` fails if `cmd1` fails, not just `cmd2`.
  Without this, `grep missing file.txt | wc -l` would succeed even if `grep` fails.
- **`TRACE` support** — allows `TRACE=1 ./script.sh` for debugging without modifying the
  script. Prints every command before execution. Invaluable for CI debugging.
- **`main()` pattern** — keeps variables local, makes the entry point obvious, and matches
  the Google Shell Style Guide recommendation for scripts with multiple functions.
- **File header** — describes purpose, usage, and environment variables. Required by both
  Google and this project's self-documentation rules (ADR-023).

## File naming and organization

### Naming rules

- **kebab-case** for filenames: `check-format.sh`, not `check_format.sh` or `checkFormat.sh`
- **verb-object** pattern: the name describes what the script does
- **`.sh` extension** on all scripts (aids syntax highlighting, makes language obvious)

```text
# Good
scripts/check/tidy.sh
scripts/gh/create-pr.sh
scripts/dev/setup.sh

# Bad
scripts/check/comment-ratio.sh   # snake_case
scripts/gh/createPr.sh           # camelCase
scripts/setup                   # no extension
```

### Directory structure

Scripts are organized by purpose in subdirectories:

```text
scripts/
├── check/    # Quality gate scripts (called by make check)
├── ci/       # CI-specific scripts (install deps, CI-only logic)
├── dev/      # Local development helpers (setup, log viewer, index)
├── gh/       # GitHub CLI helpers (PRs, issues, pipeline status)
└── test/     # Test infrastructure (integration test runners)
```

Why subdirectories over flat prefixes (like `ci-build.sh`, `gh-create-pr.sh`)?

- Tab completion works better: type `scripts/gh/` then tab
- Scales past 15+ scripts without becoming noisy
- Matches Kubernetes (`hack/`), Git (`ci/`), Docker (`hack/`) patterns
- CODEOWNERS and `.gitattributes` can target directories

## Coding conventions

### Variables

```bash
# Constants and environment variables: UPPER_CASE
readonly THRESHOLD=20
export OLLAMA_HOST="localhost"

# Local variables: snake_case, always declare with local
my_func() {
  local file_count
  file_count="$(find src -name '*.cpp' | wc -l)"
  echo "${file_count}"
}
```

**Always separate `local` declaration from command substitution assignment.**
The `local` builtin masks the exit code of the command substitution:

```bash
# Bad — $? is always 0 (exit code of local, not my_command)
local result="$(my_command)"

# Good — $? reflects my_command's exit code
local result
result="$(my_command)"
```

### Quoting

Always double-quote variables. Use `"${var}"` with braces for clarity:

```bash
# Good
echo "${file_path}"
rm "${build_dir}/output"

# Bad — word splitting and globbing risks
echo $file_path
rm $build_dir/output
```

The only exception is inside `(( ))` arithmetic:

```bash
# OK — arithmetic context doesn't need quoting
if (( count > 10 )); then
  echo "too many"
fi
```

### Conditions

Use `[[ ]]` (bash builtin), not `[ ]` or `test`:

```bash
# Good — no word splitting, supports regex and pattern matching
if [[ -z "${my_var}" ]]; then
  echo "empty"
fi

if [[ "${name}" =~ ^test_ ]]; then
  echo "test file"
fi

# Bad — subject to word splitting, no regex support
if [ -z "$my_var" ]; then
  echo "empty"
fi
```

### Error handling

Redirect error messages to stderr:

```bash
# Good
echo "ERROR: file not found: ${path}" >&2
exit 1

# Better — use an err() helper for consistent formatting
err() {
  echo "[ERROR] $*" >&2
}

err "file not found: ${path}"
exit 1
```

### Command substitution

Use `$(command)`, never backticks:

```bash
# Good — nestable, readable
result="$(basename "$(dirname "${path}")")"

# Bad — requires escaping for nesting
result=`basename \`dirname "${path}"\``
```

### Help flag

Scripts that accept arguments should handle `-h` / `--help`:

```bash
if [[ "${1-}" =~ ^-*h(elp)?$ ]]; then
  echo "Usage: ./scripts/check/tidy.sh [--full]"
  echo ""
  echo "Run clang-tidy on changed files (default) or all files (--full)."
  exit 0
fi
```

## Comments

### Target: ~20% comment ratio

This project maintains a ~20% comment ratio across all code, including shell scripts.
Comments explain *why*, not *what*. Every script should have:

1. **File header** (required) — purpose, usage, environment variables
2. **Function headers** (required for non-trivial functions) — Google-style with
   Globals/Arguments/Outputs/Returns
3. **Inline comments** — for non-obvious logic, workarounds, and "why" explanations

### File header format

```bash
#!/usr/bin/env bash
#
# tidy.sh — Run clang-tidy on source files.
#
# In smart mode (default), only checks files changed vs main branch.
# In full mode (--full), checks all .cpp files in src/.
#
# Usage:
#   ./scripts/check/tidy.sh [--full]
#
# Environment:
#   TRACE=1    Enable debug tracing
#   SMART=1    Smart mode (default, only changed files)
#
# @see docs/tools/clang-tidy.md — clang-tidy configuration
# @see docs/adr/adr-44-tidy-boilerplate.md
```

### Function header format

Follow the Google Shell Style Guide format:

```bash
#######################################
# Check if a tool is installed and meets the minimum version.
# Globals:
#   VERSIONS_FILE — path to versions.env
# Arguments:
#   $1 — tool name (e.g., "cmake")
#   $2 — minimum version (e.g., "3.28")
# Outputs:
#   Writes status to stdout
# Returns:
#   0 if tool is installed and version matches, 1 otherwise
#######################################
check_tool_version() {
  local tool="$1"
  local min_version="$2"
  # ...
}
```

### Inline comments

```bash
# Good — explains WHY
# grep -v filters out linenoise (vendored) and SCENARIO (doctest macro)
# that clang-tidy doesn't understand
grep -v "linenoise\|SCENARIO" || true

# Bad — restates WHAT the code does
# grep to remove lines containing linenoise or SCENARIO
grep -v "linenoise\|SCENARIO" || true
```

## Compliance checking

### ShellCheck

[ShellCheck](https://www.shellcheck.net/) is the standard static analysis tool for shell
scripts. It catches common bugs, style issues, and portability problems.

**Install:**

```bash
# macOS
brew install shellcheck

# Ubuntu/Debian
sudo apt-get install shellcheck

# Or via make setup (once version pinning is complete)
make setup
```

**Run manually:**

```bash
# Check a single script
shellcheck scripts/check/tidy.sh

# Check all scripts
find scripts -name '*.sh' | xargs shellcheck

# Check with specific shell dialect
shellcheck --shell=bash scripts/check/tidy.sh
```

**Configuration** (`.shellcheckrc` in project root):

```text
# .shellcheckrc — ShellCheck configuration
# @see docs/tools/shell-scripts.md

# Use bash dialect for all scripts
shell=bash

# Disable specific rules if needed (with justification):
# SC2059: Don't use variables in printf format string
#   We use this intentionally for colored output in make help
# disable=SC2059
```

**Integration with `make check`:**

```bash
# Added as a check target
make shellcheck    # Run ShellCheck on all scripts
```

### Convention lint script

Beyond ShellCheck (which checks shell syntax and bugs), a project-specific lint script
validates our naming and structural conventions:

**`scripts/check/lint-scripts.sh`** checks:

1. All `.sh` files use `#!/usr/bin/env bash` (not `#!/bin/sh` or `#!/bin/bash`)
2. All `.sh` files have `set -o errexit`, `set -o nounset`, `set -o pipefail`
3. All `.sh` files in `scripts/` use kebab-case naming
4. All `.sh` files have a file header comment in the first 5 lines
5. Makefile targets with >5 lines of shell delegate to a script

This is a lightweight ~40-line script that catches convention drift. It runs as part of
`make check`.

## What works, what doesn't

### What works well

- **`set -o errexit`** catches most errors early. Combined with `pipefail`, it's the
  single most impactful safety improvement for shell scripts.
- **ShellCheck** catches real bugs that humans miss: unquoted variables, unused variables,
  incorrect `[` vs `[[`, subshell variable scope issues.
- **`main()` pattern** keeps variables local and makes scripts easier to test — you can
  source the script and call individual functions.
- **`TRACE=1`** is invaluable for debugging CI failures without modifying the script.

### What doesn't work well

- **`set -o errexit` with pipes** — `cmd | grep pattern` fails if `grep` finds nothing
  (exit code 1). Fix: use `cmd | grep pattern || true` when empty output is acceptable.
- **`set -o nounset` with optional variables** — accessing `$1` when no argument is passed
  triggers an error. Fix: use `"${1-}"` (empty default) or `"${1-default_value}"`.
- **`local` masking exit codes** — `local var="$(cmd)"` always succeeds. Fix: separate
  declaration and assignment (see Variables section above).
- **macOS vs Linux differences** — `sed -i` requires `sed -i ''` on macOS. `date` flags
  differ. `readlink -f` doesn't exist on macOS (use `realpath` or `cd + pwd`). Test on
  both platforms.
- **Arithmetic with `set -o errexit`** — `(( i++ ))` when `i=0` evaluates to 0 (falsy)
  and triggers errexit. Fix: use `(( i += 1 ))` or `(( ++i ))`.

## Troubleshooting

### Script fails silently in CI

Enable tracing: `TRACE=1 make <target>`. This prints every command before execution.
If the script is called from a Makefile, add `TRACE=1` to the environment:

```bash
TRACE=1 bash scripts/check/tidy.sh
```

### ShellCheck reports SC2086 (double quote to prevent globbing)

This is the most common ShellCheck warning. Always quote variables:

```bash
# Triggers SC2086
echo $my_var

# Fixed
echo "${my_var}"
```

### Script works locally but fails in CI

Common causes:

1. **Different bash version** — macOS ships bash 3.2 (2007!), Linux has bash 5.x.
   Avoid bash 4+ features (associative arrays, `readarray`, `${var,,}`) or ensure
   CI and local both use bash 4+.
2. **Missing tool** — CI may not have a tool installed. Check with `command -v tool`.
3. **Different `PATH`** — CI runners have different default paths. Use absolute paths
   or verify tool location.
4. **Line endings** — Windows-style `\r\n` breaks shebangs. Use `.gitattributes` to
   enforce LF for `.sh` files.

### `set -o nounset` breaks my script

Use default values for optional variables:

```bash
# Optional argument with empty default
local arg="${1-}"

# Optional env var with fallback
local host="${OLLAMA_HOST-localhost}"

# Check if variable is set (not just non-empty)
if [[ -n "${MY_VAR+x}" ]]; then
  echo "MY_VAR is set to: ${MY_VAR}"
fi
```

## References

- [Google Shell Style Guide](https://google.github.io/styleguide/shellguide.html) —
  comprehensive style guide, basis for most of our conventions
- [Shell Script Best Practices](https://sharats.me/posts/shell-script-best-practices/) —
  Sharat's practical recommendations (template, TRACE, help flags)
- [ShellCheck](https://www.shellcheck.net/) — static analysis tool for shell scripts
- [Bash FAQ](http://tiswww.case.edu/php/chet/bash/FAQ) — official bash FAQ
- [r/devops shell scripting](https://www.reddit.com/r/devops/comments/7baj4c/shell_scripting_best_practices/) —
  community discussion on best practices
- [ADR-044](../adr/adr-044-tidy-boilerplate.md) — architecture decision for build boilerplate
- [ADR-023](../adr/adr-023-self-documenting-processes.md) — self-documenting processes
- [CONTRIBUTING.md](../../CONTRIBUTING.md) — project contribution rules
