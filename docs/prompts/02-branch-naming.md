# Prompt 02: Add branch naming validation

## Context

- The pre-commit hook is at `scripts/git/pre-commit.sh`
- It currently blocks commits to `main` but does NOT validate branch naming
- Current content of the branch check section:

```bash
# Block direct commits to main
branch="$(git symbolic-ref --short HEAD 2>/dev/null || true)"
if [[ "${branch}" == "main" ]]; then
  echo "ERROR: direct commits to main are not allowed. Use a feature branch."
  exit 1
fi
```

## Task

Add branch naming validation to `scripts/git/pre-commit.sh`, right after the existing `main` block.

## Change: Update `scripts/git/pre-commit.sh`

Find this exact text:

```bash
# Block direct commits to main
branch="$(git symbolic-ref --short HEAD 2>/dev/null || true)"
if [[ "${branch}" == "main" ]]; then
  echo "ERROR: direct commits to main are not allowed. Use a feature branch."
  exit 1
fi
```

Replace it with:

```bash
# Block direct commits to main
branch="$(git symbolic-ref --short HEAD 2>/dev/null || true)"
if [[ "${branch}" == "main" ]]; then
  echo "ERROR: direct commits to main are not allowed. Use a feature branch."
  exit 1
fi

# Validate branch naming convention
# @see docs/adr/adr-048-quality-framework.md §3.2 check 0.2
BRANCH_PATTERN="^(feat|fix|chore|spike|refactor|docs|test)/[0-9]+-[a-z0-9-]+$"
if [[ -n "${branch}" && ! "${branch}" =~ ${BRANCH_PATTERN} ]]; then
  echo ""
  echo "ERROR: Branch name '${branch}' does not follow naming convention."
  echo ""
  echo "  Expected: type/issue-id-short-name"
  echo "  Examples: feat/42-add-streaming"
  echo "            fix/15-empty-input"
  echo "            chore/99-update-deps"
  echo ""
  echo "  Valid types: feat fix chore spike refactor docs test"
  echo ""
  exit 1
fi
```

## Verify

```bash
# 1. Test the regex directly (no git needed)
branch="feat/42-add-streaming"
BRANCH_PATTERN="^(feat|fix|chore|spike|refactor|docs|test)/[0-9]+-[a-z0-9-]+$"
[[ "$branch" =~ $BRANCH_PATTERN ]] && echo "PASS: accepted feat/42-add-streaming" || echo "FAIL"

branch="my-feature"
[[ "$branch" =~ $BRANCH_PATTERN ]] && echo "FAIL: should reject" || echo "PASS: rejected my-feature"

branch="fix/15-empty-input"
[[ "$branch" =~ $BRANCH_PATTERN ]] && echo "PASS: accepted fix/15-empty-input" || echo "FAIL"

# 2. Verify the file contains the new check
grep -c "BRANCH_PATTERN" scripts/git/pre-commit.sh && echo "PASS: pattern found in pre-commit.sh" || echo "FAIL: pattern not found"
```

## Expected output

```
PASS: accepted feat/42-add-streaming
PASS: rejected my-feature
PASS: accepted fix/15-empty-input
PASS: pattern found in pre-commit.sh
```

## Commit message

```
chore: add branch naming validation to pre-commit hook
```
