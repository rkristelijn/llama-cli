# Git hook management
# Include: include path/to/cpm/lib/make/git.mk

.PHONY: hooks

hooks: ## Install git hooks (pre-commit, pre-push)
	@mkdir -p .git/hooks
	@ln -sf ../../scripts/git/pre-commit.sh .git/hooks/pre-commit
	@ln -sf ../../scripts/git/pre-push.sh .git/hooks/pre-push
	@echo "  hooks                ✓ (symlinked)"
