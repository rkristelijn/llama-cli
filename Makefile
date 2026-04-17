BUILD_DIR = build
CLANG_TIDY = $(shell command -v clang-tidy 2>/dev/null || echo /opt/homebrew/opt/llvm/bin/clang-tidy)

# SMART=1 (default) enables incremental checks (only changed files vs main)
# FULL=1 disables smart mode for exhaustive checks (used in full-check target)
FULL ?= 0
ifeq ($(FULL),1)
  SMART := 0
else
  SMART ?= 1
endif

.PHONY: all build clean run start s log test t test-unit test-e2e end-to-end e2e check full-check check-ai format format-check install hooks help quick index comment-ratio pipeline-status pr-status pr pr-feedback download-issues check-deps check-versions live tidy lint yamllint markdownlint complexity docs sast sast-secret sast-security coverage coverage-folder todo create-issue gh-pr-status gps gh-pipeline-status gpls gh-create-pr gpr gh-pr-feedback gpf gh-download-issues gdi

check-deps:
	@command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found. Run 'make setup' first."; exit 1; }

# Check installed tool versions against .config/versions.env
check-versions:
	@. .config/versions.env; \
	ok=0; fail=0; \
	check() { \
		local name="$$1" expected="$$2" actual="$$3"; \
		if [ -z "$$actual" ]; then \
			printf "  %-20s MISSING (expected %s)\n" "$$name" "$$expected"; fail=$$((fail+1)); \
		elif echo "$$actual" | grep -q "$$expected"; then \
			printf "  %-20s %s ✓\n" "$$name" "$$actual"; ok=$$((ok+1)); \
		else \
			printf "  %-20s %s (expected %s)\n" "$$name" "$$actual" "$$expected"; fail=$$((fail+1)); \
		fi; \
	}; \
	echo "==> Checking tool versions against .config/versions.env"; \
	check "cmake"        "$$CMAKE_VERSION"    "$$(cmake --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)"; \
	check "clang-format" "$$LLVM_VERSION"     "$$(clang-format --version 2>/dev/null | grep -oE '[0-9]+' | head -1)"; \
	check "clang-tidy"   "$$LLVM_VERSION"     "$$(clang-tidy --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+' | head -1 | cut -d. -f1)"; \
	check "cppcheck"     "$$CPPCHECK_VERSION" "$$(cppcheck --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+')"; \
	check "doxygen"      "$$DOXYGEN_VERSION"  "$$(doxygen --version 2>/dev/null)"; \
	check "cloc"         "$$CLOC_VERSION"     "$$(cloc --version 2>/dev/null)"; \
	check "shellcheck"   "$$SHELLCHECK_VERSION" "$$(shellcheck --version 2>/dev/null | grep '^version:' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"; \
	check "yamllint"     "$$YAMLLINT_VERSION"   "$$(yamllint --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"; \
	check "rumdl"        "$$RUMDL_VERSION"      "$$(rumdl version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"; \
	echo ""; \
	echo "  $$ok OK, $$fail mismatch/missing"; \
	[ "$$fail" -eq 0 ] || echo "  Run 'make setup' to install missing tools."

all: check-deps
	@cmake -B $(BUILD_DIR) -S . > /dev/null
	@cmake --build $(BUILD_DIR) --target llama-cli > /dev/null
	@cp $(BUILD_DIR)/llama-cli .

build: all

# Shorthands
s: start
start: all
	./$(BUILD_DIR)/llama-cli $(ARGS)

r: run
run: start

t: test
test: test-unit

end-to-end: e2e
e2e: build
	@echo "==> make e2e"
	@for t in e2e/*.sh; do \
		case "$$t" in *test_live*|*helpers*) continue;; esac; \
		bash "$$t" $(BUILD_DIR)/llama-cli > /dev/null || { echo "FAIL: $$t"; exit 1; }; \
	done
	@echo "  [done] e2e"

log:
	@bash scripts/log-viewer.sh $(ARGS)

test-unit: all
	@echo "==> make test-unit"
	@cmake --build $(BUILD_DIR) --target test_config > /dev/null
	@cmake --build $(BUILD_DIR) --target test_json > /dev/null
	@cmake --build $(BUILD_DIR) --target test_repl > /dev/null
	@cmake --build $(BUILD_DIR) --target test_command > /dev/null
	@cmake --build $(BUILD_DIR) --target test_annotation > /dev/null
	@cmake --build $(BUILD_DIR) --target test_exec > /dev/null
	@./$(BUILD_DIR)/test_config --quiet
	@./$(BUILD_DIR)/test_json --quiet
	@./$(BUILD_DIR)/test_repl --quiet
	@./$(BUILD_DIR)/test_command --quiet
	@./$(BUILD_DIR)/test_annotation --quiet
	@./$(BUILD_DIR)/test_exec --quiet
	@echo "  [done] test-unit"

live: all
	@bash e2e/test_live.sh $(BUILD_DIR)/llama-cli

tidy: all
	@if [ "$(SMART)" = "1" ]; then \
		echo "==> make tidy (smart incremental mode)"; \
		branch=$$(git rev-parse --abbrev-ref HEAD); \
		diff_base=$$( [ "$$branch" = "main" ] && echo "HEAD^" || echo "origin/main" ); \
		files=$$(git diff --name-only $$diff_base | grep "\.cpp$$" | grep "^src/" || true); \
		if [ -z "$$files" ]; then \
			echo "  [skip] no changed files vs $$diff_base"; \
		else \
			for dir in src $$(find src -maxdepth 1 -mindepth 1 -type d); do \
				dir_files=$$(echo "$$files" | grep "^$$dir/[^/]*\.cpp$$" || true); \
				if [ -n "$$dir_files" ]; then \
					echo "  [checking] $$dir/ ($$(echo $$dir_files | wc -w) files)"; \
					$(CLANG_TIDY) --config-file=.config/.clang-tidy $$dir_files -- -std=c++17 -I src/ 2>&1 | grep "warning:" | grep -v "linenoise\|SCENARIO\|cognitive complexity\|identifier-naming\|logging/logger.*function-size" && exit 1 || true; \
				fi; \
			done; \
		fi; \
	else \
		echo "==> make tidy (full mode)"; \
		find src -name '*.cpp' -print0 | xargs -0 $(CLANG_TIDY) --config-file=.config/.clang-tidy -- -std=c++17 -I src/ 2>&1 | grep "warning:" | grep -v "linenoise\|SCENARIO\|cognitive complexity\|identifier-naming\|logging/logger.*function-size" && exit 1 || true; \
	fi
	@echo "  [done] tidy"

lint: all
	@echo "==> make lint (running cppcheck...)"
	@cppcheck --enable=all --suppress=missingIncludeSystem --suppress=missingInclude --suppress=unusedFunction --suppress=unmatchedSuppression --suppress=normalCheckLevelMaxBranches --suppress=checkersReport --suppress=useStlAlgorithm --suppress=knownConditionTrueFalse:*_it.cpp --suppress=knownConditionTrueFalse:*_test.cpp --error-exitcode=1 -I src/ src/ 2>&1 | grep -v "Checking\|files checked" || true
	@echo "  [done] lint"

yamllint:
	@echo "==> make yamllint"
	@yamllint -c .config/yamllint.yml .github/
	@echo "  [done] yamllint"

markdownlint:
	@echo "==> make markdownlint"
	@rumdl check .
	@echo "  [done] markdownlint"

complexity: all
	@echo "==> make complexity (running pmccabe...)"
	@find src -name '*.cpp' | xargs pmccabe 2>/dev/null | while read line; do \
		file=$$(echo $$line | awk '{print $$6}' | cut -d'(' -f1); \
		lno=$$(echo $$line | awk '{print $$6}' | cut -d'(' -f2 | cut -d')' -f1); \
		if ! head -n $$lno $$file | tail -n 6 | grep -q "pmccabe:skip-complexity\|clang-tidy:skip-complexity"; then \
			echo $$line | awk '$$1 > 10 {print; exit 1}'; \
		fi; \
	done || exit 1
	@echo "  [done] complexity"

sast-security:
	@echo "==> make sast-security (semgrep...)"
	@if command -v semgrep >/dev/null; then \
		semgrep scan --config auto --error --quiet 2>&1 | grep -v "┌────\|Semgrep CLI\|└─────────────" || true; \
	fi
	@echo "  [done] sast-security"

sast-secret:
	@echo "==> make sast-secret (gitleaks...)"
	@if command -v gitleaks >/dev/null; then \
		gitleaks detect --source . --log-level error --no-banner; \
	fi
	@echo "  [done] sast-secret"

sast: sast-security sast-secret

docs:
	@echo "==> make docs (generating doxygen...)"
	@doxygen .config/Doxyfile 2>&1 | grep "warning:" | grep -v "No output formats\|Unsupported xml\|falsely parses" && exit 1 || true
	@echo "  [done] docs"

check: format-check yamllint markdownlint tidy complexity lint docs index
	@$(MAKE) -s coverage-folder
	@$(MAKE) -s sast
	@echo "==> make comment-ratio"
	@bash scripts/test_comment_ratio.sh | grep "PASS" || bash scripts/test_comment_ratio.sh
	@echo "==> make todo"
	@$(MAKE) -s todo | grep -v "==> make todo"
	@echo ""
	@echo "All checks passed."

full-check:
	@$(MAKE) FULL=1 check

check-ai:
	@$(MAKE) -s check 2>&1 | grep -E "^\s*([0-9]+|src/|==>|FAIL|All checks passed|knownCondition|always false|too many|warning:|error:)" | grep -v "^$$"

setup:
	bash scripts/setup.sh

install: all
	@cp $(BUILD_DIR)/llama-cli /usr/local/bin/llama-cli
	@echo "Installed to /usr/local/bin/llama-cli"

hooks:
	@cp .config/pre-commit .git/hooks/pre-commit
	@chmod +x .git/hooks/pre-commit
	@echo "Git hooks installed."

todo:
	@echo "==> Markdown TODOs"
	@find . -name "*.md" -not -path "./build/*" -not -path "./.git/*" -exec awk ' \
		FNR == 1 { \
			if (prev_line != "") printf "%s:%d:%s\n", prev_file, prev_lnum, prev_line; \
			prev_line = ""; \
		} \
		/- \[ \]/ { \
			match($$0, /[^ ]/); \
			curr_indent = RSTART; \
			if (prev_line != "") { \
				if (curr_indent > prev_indent) { \
					printf "%s:%d:\033[1m%s\033[0m\n", FILENAME, prev_lnum, prev_line; \
				} else { \
					printf "%s:%d:%s\n", FILENAME, prev_lnum, prev_line; \
				} \
			} \
			prev_line = $$0; \
			prev_indent = curr_indent; \
			prev_lnum = FNR; \
			prev_file = FILENAME; \
		} \
		END { \
			if (prev_line != "") printf "%s:%d:%s\n", FILENAME, prev_lnum, prev_line; \
		}' {} +
	@echo ""
	@echo "==> Code TODOs"
	@grep -rn "TODO\|FIXME\|HACK\|XXX" src/ include/ --include="*.cpp" --include="*.h" 2>/dev/null || true

index:
	@echo "==> make index"
	@bash scripts/build-index.sh

format:
	@echo "==> make format"
	@find src -name '*.cpp' -o -name '*.h' | xargs clang-format -i --style=file:.config/.clang-format

format-check:
	@echo "==> make format-check"
	@find src -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run -Werror --style=file:.config/.clang-format
	@echo "format-check: OK"

comment-ratio:
	@cloc src/ --not-match-f='(_test|_it)\.cpp$$' --by-file --csv --quiet \
	  | grep -v "^language\|^SUM\|^http" \
	  | awk -F',' 'NF==5 && $$5>0 {ratio=int($$4/($$4+$$5)*100); printf "%d%%\t%s\n", ratio, $$2}' \
	  | sort -n

gh-pipeline-status:
	bash scripts/gh-pipeline-status.sh
gpls: gh-pipeline-status

gh-pr-status:
	bash scripts/gh-pr-status.sh $(ARGS)
gps: gh-pr-status

gh-create-pr:
	bash scripts/gh-create-pr.sh
gpr: gh-create-pr

gh-download-issues:
	bash scripts/gh-download-issues.sh
gdi: gh-download-issues

gh-pr-feedback:
	bash scripts/gh-pr-feedback.sh
gpf: gh-pr-feedback

create-issue:
	@if [ -z "$(TITLE)" ] || [ -z "$(DESC)" ]; then \
		echo "Usage: make create-issue TITLE=\"My title\" DESC=\"My description\""; \
		exit 1; \
	fi
	@bash scripts/gh-create-issue.sh "$(TITLE)" "$(DESC)"

coverage:
	@echo "==> make coverage (configuring with --coverage...)"
	@cmake -B $(BUILD_DIR) -S . -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage" > /dev/null
	@cmake --build $(BUILD_DIR) > /dev/null
	@echo "==> make coverage (running tests...)"
	@./$(BUILD_DIR)/test_config --quiet
	@./$(BUILD_DIR)/test_json --quiet
	@./$(BUILD_DIR)/test_repl --quiet
	@./$(BUILD_DIR)/test_command --quiet
	@./$(BUILD_DIR)/test_annotation --quiet
	@./$(BUILD_DIR)/test_exec --quiet
	@echo "  [done] coverage"

coverage-folder: coverage
	@echo "==> make coverage-folder"
	@for dir in src $$(find src -maxdepth 1 -mindepth 1 -type d); do \
		files=$$(ls $(BUILD_DIR)/CMakeFiles/*.dir/$$dir/*.cpp.o 2>/dev/null); \
		if [ -n "$$files" ]; then \
			printf "  %-20s " "$$dir/"; \
			gcov -n $$files 2>/dev/null \
			| sed 's/,/./g' \
			| awk '/Lines executed/ { \
				split($$2, a, ":"); split(a[2], b, "%"); \
				exec_lines += b[1] * $$4 / 100; \
				total_lines += $$4; \
			} END { \
				if (total_lines > 0) printf "%.2f%%\n", (exec_lines / total_lines) * 100; \
				else print "N/A"; \
			}' || echo "N/A"; \
		fi; \
	done
	@echo "  [done] coverage-folder"

clean:
	rm -rf $(BUILD_DIR) llama-cli

quick: all
	@./$(BUILD_DIR)/test_config --quiet
	@./$(BUILD_DIR)/test_json --quiet
	@./$(BUILD_DIR)/test_repl --quiet
	@./$(BUILD_DIR)/test_command --quiet
	@./$(BUILD_DIR)/test_annotation --quiet
	@./$(BUILD_DIR)/test_exec --quiet
	@bash scripts/test_comment_ratio.sh | grep "PASS" || bash scripts/test_comment_ratio.sh

prepush:
	@changed=$$(git diff --name-only origin/main...HEAD); \
	if echo "$$changed" | grep -qE '\.(cpp|h)$$'; then \
		echo "==> make check (code changed)"; \
		$(MAKE) -s check; \
	else \
		echo "==> make index (docs only)"; \
		$(MAKE) -s index; \
		git diff --quiet INDEX.md || { echo "FAIL: INDEX.md outdated"; exit 1; }; \
		echo "All checks passed."; \
	fi

help:
	@echo "Usage:"
	@echo "  make \033[1m(m)\033[0m                    build the project"
	@echo "  make start \033[1m(s)\033[0m              build and run the REPL"
	@echo "  make test \033[1m(t)\033[0m               unit tests"
	@echo "  make end-to-end \033[1m(e2e)\033[0m       end-to-end tests"
	@echo "  make check \033[1m(c)\033[0m              run smart quality checks (default)"
	@echo "  make full-check \033[1m(fc)\033[0m        run exhaustive quality checks"
	@echo "  make sast \033[1m(sast)\033[0m            run all static analysis"
	@echo "  make sast-security \033[1m(ss)\033[0m     run security analysis (semgrep)"
	@echo "  make sast-secret \033[1m(sss)\033[0m      run secret scanning (gitleaks)"
	@echo "  make gh-pr-status \033[1m(gps)\033[0m     show failed PR jobs"
	@echo "  make gh-pr-feedback \033[1m(gpf)\033[0m   show CodeRabbit feedback"
	@echo "  make todo                   show TODO items"
	@echo "  make coverage               generate coverage report"
	@echo "  make clean                  remove build artifacts"
