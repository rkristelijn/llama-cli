# 3-tier quality gates
# Include after common.mk: include path/to/cpm/lib/make/quality.mk

.PHONY: check-fast check check-all

check-fast: ## Tier 1: autofix + fast lint (<3s, AI loop)
	@$(MAKE) -s format 2>&1 | tee .tmp/check-fast.log
	$(log_footer)

check: ## Tier 2: full quality gate (pre-push level)
	@$(MAKE) -s format lint test 2>&1 | tee .tmp/check.log
	$(log_footer)

check-all: ## Tier 3: exhaustive (CI level)
	@$(MAKE) -s format lint test sast 2>&1 | tee .tmp/check-all.log
	$(log_footer)
