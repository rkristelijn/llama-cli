# Pull Request & Release Workflow

This guide covers the fork-based contribution workflow used in this project.

## Setup (one-time)

```bash
# Clone the upstream repo
git clone git@github.com:rkristelijn/llama-cli.git
cd llama-cli

# Add your fork as a remote
git remote add fork git@github.com:<your-user>/llama-cli.git
```

Verify:

```text
$ git remote -v
origin  git@github.com:rkristelijn/llama-cli.git   # upstream (read)
fork    git@github.com:<your-user>/llama-cli.git    # your fork (push)
```

## Workflow overview

```mermaid
sequenceDiagram
    participant M as origin/main
    participant B as your branch
    participant F as fork (GitHub)
    participant PR as Pull Request

    M->>B: 1. git checkout -b feat/my-feature
    Note over B: make changes, run make check
    B->>B: 2. git add + git commit
    B->>F: 3. git push -u fork feat/my-feature
    F->>PR: 4. Create PR (fork → origin/main)
    Note over PR: CI runs, CodeRabbit reviews
    Note over PR: address feedback, push more commits
    PR->>M: 5. Merge PR on GitHub
    M->>B: 6. git checkout main && git pull origin main
```

## Step by step

### 1. Create a feature branch

Always branch from an up-to-date main:

```bash
git checkout main
git pull origin main
git checkout -b feat/my-feature    # or fix/, refactor/, docs/
```

### 2. Make changes and verify

```bash
# edit files...
make check                         # runs all 15 quality checks
```

### 3. Commit

```bash
git add src/file.cpp src/file.h    # stage specific files, avoid git add .
git commit -m "feat: add banner toggle option"
```

Commit message format: `type: short description`
Types: `feat`, `fix`, `refactor`, `docs`, `chore`, `test`

### 4. Push to your fork

```bash
git push -u fork feat/my-feature
```

> **Important**: push to `fork`, not `origin`. Origin is the upstream repo.

### 5. Create the PR

```bash
# Using GitHub CLI:
gh pr create --repo rkristelijn/llama-cli --base main --head <your-user>:feat/my-feature

# Or use the GitHub web UI — it will suggest creating a PR after you push.
```

### 6. Address review feedback

Push additional commits to the same branch:

```bash
# make fixes...
git add -p                         # stage interactively
git commit -m "fix: address review feedback"
git push fork feat/my-feature
```

The PR updates automatically.

## Rebasing when your branch is behind main

This happens when main has new commits that your branch doesn't have.

```mermaid
sequenceDiagram
    participant M as origin/main
    participant B as your branch
    participant F as fork (GitHub)

    M->>M: new commits merged
    B->>B: your branch is behind
    M->>B: 1. git fetch origin main
    M->>B: 2. git rebase origin/main
    Note over B: resolve conflicts if any
    B->>F: 3. git push --force-with-lease fork feat/my-feature
```

```bash
git fetch origin main
git rebase origin/main
# resolve any conflicts, then: git rebase --continue
git push --force-with-lease fork feat/my-feature
```

> `--force-with-lease` is required after rebase (history was rewritten).
> It's safe: it refuses if someone else pushed to your branch.

## Releasing

Releases are automated. When `VERSION` is changed on `main`, the release workflow triggers.

```mermaid
sequenceDiagram
    participant D as Developer
    participant M as main
    participant CI as GitHub Actions
    participant R as GitHub Release

    D->>D: make bump patch
    D->>M: commit + merge PR
    M->>CI: VERSION changed → release.yml triggers
    CI->>CI: build linux-x64, linux-arm64, macos-arm64
    CI->>R: create tag v0.18.3, upload binaries
```

```bash
make bump patch                    # 0.18.2 → 0.18.3 (also: minor, major)
git add VERSION
git commit -m "chore(version): bump"
# push via PR as usual
```

**VERSION format**: plain semver without `v` prefix (e.g. `0.18.3`).
The release workflow adds the `v` prefix for tags and release names (`v0.18.3`).

## Quick reference

| Task | Command |
|------|---------|
| Update main | `git pull origin main` |
| New branch | `git checkout -b feat/name` |
| Push to fork | `git push -u fork feat/name` |
| Rebase on main | `git fetch origin main && git rebase origin/main` |
| Force push after rebase | `git push --force-with-lease fork feat/name` |
| Run checks | `make check` |
| Bump version | `make bump patch\|minor\|major` |
| Create PR | `gh pr create --repo rkristelijn/llama-cli` |

## Common mistakes

- **Pushing to origin instead of fork** — you'll get permission denied or push to upstream by accident
- **`git pull` after rebase** — creates merge commits; use `git push --force-with-lease` instead
- **Editing VERSION manually** — can introduce trailing whitespace or extra newlines that break the build; use `make bump`
- **Forgetting to rebase** — your PR will have merge conflicts; rebase before requesting review
