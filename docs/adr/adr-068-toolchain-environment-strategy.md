
# ADR 068: Toolchain Environment Strategy

**Status:** `Accepted`  
**Date:** 2026-05-02  
**Context:** 
To achieve a "Sonar-level" Quality Gate, the project relies on a "Composable Stack" of specialized tools (e.g., `clang-tidy`, `semgrep`, `pmccabe`, `mull`). The reliability of these checks depends entirely on the consistency of the environment. We need to decide how to manage these tool versions to prevent "environment drift" between local development and CI.

**Options Considered:**

### 1. As-Is: Manual Version Pinning (`.config/versions.env`)
The current approach uses a shell-sourceable file to define versions, with `make setup` handling installation via host package managers (`brew`/`apt`).
* **Pros:** Extremely low overhead, zero new dependencies, easy to audit in Git.
* **Cons:** High maintenance; relies on the host's package manager state; susceptible to "environment drift" if `make setup` is not run.

### 2. Nix: Pure Functional Environment
Using the Nix package manager to create a purely functional, reproducible environment. Every tool and its exact dependency tree are defined in a `flake.nix` or `shell.nix`.
* **Pros:** Absolute reproducibility; "It works on my machine" is mathematically guaranteed; eliminates dependency conflicts entirely.
* **Cons:** Extremely steep learning curve; requires replacing the host's package management paradigm; can be overkill for a C++ CLI project.

### 3. Docker: Containerized Execution
Running the entire toolchain and build process inside a Docker container.
* **Pros:** Total isolation from the host OS; identical environment for Dev and CI; easy to distribute.
* **Cons:** Heavyweight; degrades the "Local-First" interactive REPL experience; filesystem/network overhead slows down the "Fast" dev loop.

**Decision:**
We will adopt a **Hybrid Strategy**:

1.  **Development (Tier 1 & 2):** We will continue with the **As-Is** approach (or move towards a lightweight manager like `mise`) for the local development loop. The priority is a fast, interactive, and "native" feeling REPL.
2.  **Verification (Tier 3):** We will use **Docker** specifically for the "Heavy" and "CI" tiers. This ensures that the final "Quality Gate" is executed in a clean, immutable environment, isolated from any local developer configurations.

**Consequences:**
* **Positive:**
    * Maintains high-speed, interactive development on the host.
    * Ensures absolute reliability of the final "Quality Gate" via Docker in CI.
    * Avoids the extreme complexity of adopting the Nix ecosystem.
* **Negative:**
    * Requires maintaining two execution contexts: "Native" for Dev and "Docker" for CI.
    * Requires ensuring that the "Native" environment (Tier 1/2) is sufficiently synchronized with the "Docker" environment (Tier 3).


