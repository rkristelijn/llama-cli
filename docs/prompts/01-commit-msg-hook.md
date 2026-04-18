# Prompt 01: Add commit message validation hook

## Context

- Project: llama-cli (C++ CLI tool)
- Git hooks live in `scripts/git/` and are installed by `make hooks`
- The Makefile `hooks` target currently copies `pre-commit.sh` and `pre-push.sh` to `.git/hooks/`
- Conventional commit format: `type(scope): description` or `type: description`
- Valid types: feat, fix, chore, refactor, docs, test, ci, style, perf, build
- Shell script conventions: see CONTRIBUTING.md — bash, safety flags, TRACE support

## Task

Create a new file `scripts/git/commit-msg.sh` and update the Makefile `hooks` target to install it.

## File 1: Create `scripts/git/commit-msg.sh`

```bash
#!/usr/bin/env bash
#
# commit-msg — Validate commit message follows Conventional Commits format.
#
# Format: type(scope): description  OR  type: description
# Types:  feat, fix, chore, refactor, docs, test, ci, style, perf, build
#
# Installed by: make hooks
# @see docs/adr/adr-048-quality-framework.md §3.2 check 0.1

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

MSG_FILE="$1"
MSG=$(head -1 "$MSG_FILE")

# Allow merge commits
if [[ "$MSG" =~ ^Merge ]]; then
  exit 0
fi

PATTERN="^(feat|fix|chore|refactor|docs|test|ci|style|perf|build)(\(.+\))?: .{1,72}$"

if ! [[ "$MSG" =~ $PATTERN ]]; then
  echo ""
  echo "ERROR: Commit message does not follow Conventional Commits format."
  echo ""
  echo "  Expected: type(scope): description"
  echo "  Examples: feat: add streaming support"
  echo "            fix(repl): handle empty input"
  echo "            chore: update dependencies"
  echo ""
  echo "  Valid types: feat fix chore refactor docs test ci style perf build"
  echo ""
  exit 1
fi
```

## File 2: Update `Makefile` hooks target

Find this line in the Makefile:

```makefile
hooks: ## Install git hooks (pre-commit, pre-push)
	@cp scripts/git/pre-commit.sh .git/hooks/pre-commit
	@cp scripts/git/pre-push.sh .git/hooks/pre-push
	@chmod +x .git/hooks/pre-commit .git/hooks/pre-push
	@echo "Git hooks installed (pre-commit, pre-push)."
```

Replace it with:

```makefile
hooks: ## Install git hooks (pre-commit, pre-push, commit-msg)
	@cp scripts/git/pre-commit.sh .git/hooks/pre-commit
	@cp scripts/git/pre-push.sh .git/hooks/pre-push
	@cp scripts/git/commit-msg.sh .git/hooks/commit-msg
	@chmod +x .git/hooks/pre-commit .git/hooks/pre-push .git/hooks/commit-msg
	@echo "Git hooks installed (pre-commit, pre-push, commit-msg)."
```

## Verify

```bash
# 1. Make the script executable
chmod +x scripts/git/commit-msg.sh

# 2. Install hooks
make hooks

# 3. Test: bad commit message should fail
echo "bad message" > /tmp/test-msg.txt
bash scripts/git/commit-msg.sh /tmp/test-msg.txt && echo "FAIL: should have rejected" || echo "PASS: rejected bad message"

# 4. Test: good commit message should pass
echo "feat: add streaming support" > /tmp/test-msg.txt
bash scripts/git/commit-msg.sh /tmp/test-msg.txt && echo "PASS: accepted good message" || echo "FAIL: should have accepted"

# 5. Test: scoped message should pass
echo "fix(repl): handle empty input" > /tmp/test-msg.txt
bash scripts/git/commit-msg.sh /tmp/test-msg.txt && echo "PASS: accepted scoped message" || echo "FAIL: should have accepted"

# 6. Cleanup
rm /tmp/test-msg.txt
```

## Expected output

```
Git hooks installed (pre-commit, pre-push, commit-msg).
PASS: rejected bad message
PASS: accepted good message
PASS: accepted scoped message
```

## Commit message

```
chore: add commit message validation hook
```
