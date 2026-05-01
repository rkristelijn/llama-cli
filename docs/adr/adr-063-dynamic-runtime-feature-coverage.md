
# ADR 063: Dynamic Runtime Feature Coverage via Log Instrumentation

**Status:** `Accepted`

**Context**
Currently, `llama-cli` maintains two separate sets of information regarding test coverage:

1. **Unit/Integration Scenario List:** Generated via `make features`, representing granular technical capabilities (e.g., `json: whitespace around colon`).
2. **E2E Test Suite:** A collection of shell scripts in `e2e/` that validate high-level user flows (e.g., `test_session.sh`).

There is currently no automated way to verify if the E2E test suite actually exercises the specific technical scenarios defined in the unit tests. We can only statically observe that the scripts *exist*, but not that the code paths within them are *executed*.

**Problem Statement**
We lack "Proof of Execution" for E2E tests.

- **Static Analysis Gap:** A `grep` on `e2e/*.sh` only shows which scripts exist, not which code paths are hit.
- **Traceability Gap:** We cannot programmatically confirm that a change in a low-level module (like `src/json/`) is covered by the high-level E2E suite.
- **Manual Verification:** Developers must manually ensure that new features are integrated into the E2E scripts, which is error-prone.

**Proposed Solution: Runtime Trace-Token Instrumentation**
Instead of static analysis, we will use **Dynamic Runtime Instrumentation**. The goal is to verify coverage by analyzing the logs produced during E2E execution.

1. **Instrumentation:** Introduce a lightweight logging macro `LOG_FEATURE(feature_id)` in the C++ source. This macro will emit a unique, searchable token (e.g., `[FEATURE-TOKEN: json_whitespace_colon]`) to the trace log when a specific code path is executed.
2. **Execution:** Run the E2E test suite with `TRACE=true`. This ensures all `[FEATURE-TOKEN: ...]` messages are captured in the output of the test scripts and the global event log.
3. **Verification (The "Grep" Phase):** Implement a verification tool that:
   - Extracts the complete list of all expected `feature_id`s from the project (e.g., by parsing `make features` or a central registry).
   - Scans the captured E2E logs/traces for these specific tokens.
   - Generates a report showing `[MATCHED]` vs `[MISSING]` features.

**Consequences**

**Positive:**

- **Actual Proof:** Provides empirical evidence that a specific line of code was actually reached during an E2E run.
- **Automated Traceability:** Bridges the gap between low-level unit tests and high-level E2E scripts automatically.
- **Low Overhead:** Logging a simple string is much cheaper than full instruction-level coverage (like `gcov`) for high-level E2E tests.
- **CI Integration:** Can be added to the `make check` pipeline to fail if E2E coverage drops.

**Negative/Risks:**

- **Log Volume:** Adding many tokens could increase log size (mitigated by using a `TRACE` level toggle).
- **Maintenance:** Developers must remember to add `LOG_FEATURE` when introducing new critical paths.
- **Instrumentation Drift:** If a feature name is changed in the code but not updated in the test registry, the coverage report will show a false `[MISSING]`.

**Alternatives Considered**

- **Static Grep:** (Rejected) Only proves the script *exists*, not that it *ran*.
- **Instruction-level Coverage (gcov):** (Rejected) Too heavy for E2E-level testing and difficult to map back to high-level "features".
- **Manual Mapping:** (Rejected) Does not scale and is prone to human error.
