---
summary: Map tool binary names to package manager install commands across platforms.
status: proposed
---

# ADR-125: Cross-Platform Tool Install Registry

## Context

`cpm.toml [tools]` declares which tools a project needs (binary name + version). But installing them differs per platform:

- macOS: `brew install llvm` (gives you clang-format)
- Debian/Ubuntu: `apt install clang-format-19`
- Arch: `pacman -S clang`
- Alpine: `apk add clang-extra-tools`
- Nix: `nix-env -iA nixpkgs.clang-tools`
- Cargo: `cargo install rumdl`

## Decision

Ship a tool registry in `lib/cpm/registry/tools.toml` that maps binary names to package names per manager.

### Registry format

```toml
# lib/cpm/registry/tools.toml

[g++]
brew = "gcc"
apt = "g++"
pacman = "gcc"
alpine = "g++"
nix = "gcc"

[clang-format]
brew = "llvm"
apt = "clang-format-${version}"
pacman = "clang"
alpine = "clang-extra-tools"
nix = "clang-tools"

[cppcheck]
brew = "cppcheck"
apt = "cppcheck"
pacman = "cppcheck"
nix = "cppcheck"

[pmccabe]
brew = "pmccabe"
apt = "pmccabe"

[shellcheck]
brew = "shellcheck"
apt = "shellcheck"
pacman = "shellcheck"
nix = "shellcheck"

[rumdl]
cargo = "rumdl"
url = "https://github.com/ploys/rumdl/releases"

[yamllint]
brew = "yamllint"
apt = "yamllint"
pip = "yamllint"

[gitleaks]
brew = "gitleaks"
url = "https://github.com/gitleaks/gitleaks/releases"

[semgrep]
brew = "semgrep"
pip = "semgrep"

[doxygen]
brew = "doxygen"
apt = "doxygen"
pacman = "doxygen"
```

### Package manager detection

```bash
# Priority order (first found wins)
if command -v brew >/dev/null; then PM=brew
elif command -v apt >/dev/null; then PM=apt
elif command -v pacman >/dev/null; then PM=pacman
elif command -v apk >/dev/null; then PM=alpine
elif command -v nix-env >/dev/null; then PM=nix
elif command -v cargo >/dev/null; then PM=cargo
fi
```

### Install flow (consent-based, see ADR-121)

```text
$ cpm check
  rumdl not found.
  Install? [g]lobal (brew install rumdl) / [s]kip / [n]ever
  > g
  ✓ installed rumdl 0.1.73
```

### Version interpolation

Some packages need the version in the name (e.g. `clang-format-19`):

```toml
[clang-format]
apt = "clang-format-${version}"
```

`${version}` is replaced with the major version from `cpm.toml [tools]`.

## Consequences

- One registry covers all platforms
- Projects only declare binary names + versions in `cpm.toml`
- `cpm install` works on any machine (when implemented)
- Easy to extend: add a new tool = add a TOML block
- Testable on different machines without changing project config

## References

- @see docs/adr/adr-121-cpm-quality-layer.md (tool consent, install modes)
- @see lib/cpm/checks/cpp/check-deps.sh (reads [tools] from cpm.toml)
