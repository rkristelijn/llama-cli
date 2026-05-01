
# ADR-064: Enforcement of Dead Code Detection

**Status:** `[proposed]`

**Context:**
The current `llama-cli` quality framework (ADR-048) utilizes `clang-tidy`, `cppcheck`, and `gcov` to monitor code quality. While unused variables or functions currently trigger warnings, these are not treated as build blockers. This leads to an accumulation of technical debt in the form of dead code, which violates our principle of simple, maintainable C++ (as defined in `CONTRIBUTING.md`) and increases cognitive load for developers.

**Decision:**
We will transition from "informative warnings" to "active enforcement" for dead code detection across three levels:

1. **Static Analysis (Compile-time):**
   The `clang-tidy` configuration will be updated to treat `unused-parameter`, `unused-function`, and `unused-variable` as `error` instead of `warning`. This ensures that any unused code results in a non-zero exit code during the linting phase.

2. **Dynamic Analysis (Runtime/Test-time):**
   The `make coverage` target will be augmented with a threshold check. If the branch coverage of the `src/` directory drops below the established threshold (target: 60%, as per `TODO.md`), the build process will fail.

3. **Project-wide (Orphaned Assets):**
   We will introduce a new `make dead-code-check` target. This target will scan the `scripts/` directory to identify "orphaned" scripts that are not referenced by any `Makefile` target, test script, or CI workflow.

**Consequences:**

* **Positive:**
  * **Codebase Integrity:** Keeps the codebase clean, compact, and free of unnecessary complexity.
  * **Shift-Left:** Detects and forces the removal of unused code at the earliest possible stage (commit/PR).
  * **Test Reliability:** Ensures that the code being tested is actually being exercised by the test suite.
* **Negative/Risks:**
  * **Increased Build Friction:** Developers must actively clean up unused parameters or variables during development, which may slightly increase the-effort of small changes.
  * **Test Maintenance:** Requires proactive updates to test suites to ensure coverage thresholds are met when new logic is added.

**Compliance:**
This ADR extends the existing `ADR-048: AI-Guided Quality Framework` by defining specific enforcement mechanisms for dead code detection.
