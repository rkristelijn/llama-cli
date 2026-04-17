# GitHub CLI Manual (gh)

This document provides a reference for the `gh` (GitHub CLI) commands used within this project.

## Project Custom Shortcuts

We provide shortcut commands in the `Makefile` to simplify common GitHub workflows.

| Command | Shorthand | Description |
| :--- | :--- | :--- |
| `make gh-pr-status` | `make gps` | Show status of PR jobs and failure details |
| `make gh-pipeline-status` | `make gpls` | Show CI pipeline status |
| `make gh-create-pr` | `make gpr` | Create a new pull request |
| `make gh-pr-feedback` | `make gpf` | Fetch CodeRabbit feedback |
| `make gh-download-issues` | `make gdi` | Download GitHub issues |

## Frequently Used Core Commands

Below are common `gh` commands used for maintenance and issue management.

### Issues

- `gh issue create`: Create a new issue in the current repository.
- `gh issue list`: List open issues.
- `gh issue view <number>`: View details of a specific issue.

### Pull Requests

- `gh pr status`: Check status of your PR.
- `gh pr create`: Open a new pull request.
- `gh pr checkout <number>`: Switch to a PR branch.
- `gh pr view <number>`: View details of a PR.

### Releases

- `gh release create <version>`: Create a new release.
- `gh release list`: List all releases.

### Repository

- `gh repo clone <org/repo>`: Clone a repository.
- `gh repo view`: View the repository on GitHub.

### Actions (Workflow)

- `gh run list [flags]`: List recent workflow runs.
  - `-b, --branch <string>`: Filter runs by branch.
  - `-L, --limit <int>`: Max number of runs to fetch (default: 20).
  - `-s, --status <string>`: Filter by status (queued, completed, failure, success, etc.).
  - `-w, --workflow <string>`: Filter by workflow name/ID.
- `gh run view <run-id>`: View details of a workflow run.
- `gh workflow list`: List all workflows.

## Configuration & Help

- `gh help`: Display full manual.
- `gh config list`: View current CLI configuration.

---

*For the complete list of commands, refer to the official [GitHub CLI documentation](https://cli.github.com/manual/).*
