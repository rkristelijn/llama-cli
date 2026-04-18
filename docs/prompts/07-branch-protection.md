# Prompt 07: Enable GitHub branch protection (peer review required)

## Context

- The repository is at `rkristelijn/llama-cli` on GitHub
- CI runs on pull requests but there are no branch protection rules
- CMMI 1 check 1.6 requires peer review before merge
- This is a GitHub API/CLI operation, not a code change

## Task

Enable branch protection on `main` requiring 1 reviewer and CI to pass.

## Command: Run with GitHub CLI

```bash
gh api repos/rkristelijn/llama-cli/branches/main/protection \
  --method PUT \
  --input - <<'EOF'
{
  "required_status_checks": {
    "strict": true,
    "contexts": ["build", "unit-test", "sast-security", "sast-secret"]
  },
  "enforce_admins": false,
  "required_pull_request_reviews": {
    "required_approving_review_count": 1
  },
  "restrictions": null
}
EOF
```

## Alternative: If you prefer the GitHub web UI

1. Go to <https://github.com/rkristelijn/llama-cli/settings/branches>
2. Click "Add branch protection rule"
3. Branch name pattern: `main`
4. Check: "Require a pull request before merging"
5. Set: "Required approvals" = 1
6. Check: "Require status checks to pass before merging"
7. Add status checks: `build`, `unit-test`, `sast-security`, `sast-secret`
8. Click "Create"

## Verify

```bash
# Check branch protection is enabled
gh api repos/rkristelijn/llama-cli/branches/main/protection \
  --jq '.required_pull_request_reviews.required_approving_review_count' \
  && echo "PASS: branch protection enabled"
```

## Expected output

```text
1
PASS: branch protection enabled
```

## Note

This is a one-time setup. For solo developers, you can still merge your own PRs by using the "admin override" option. The point is that the process exists and is visible.

## Commit message

No code change — this is a GitHub settings change. No commit needed.
