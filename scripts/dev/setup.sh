#!/usr/bin/env bash
#
# setup.sh — Install all development dependencies at pinned versions.
#
# Detects macOS (brew) or Linux (apt) and installs what's missing.
# Reads tool versions from .config/versions.env (single source of truth).
#
# Usage:
#   make setup              # from project root
#   bash scripts/dev/setup.sh   # direct invocation
#
# @see .config/versions.env — pinned tool versions
# @see docs/adr/adr-026-version-pinning.md — version pinning rationale
# @see docs/adr/adr-44-tidy-boilerplate.md — build boilerplate ADR

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# Navigate to project root (setup.sh lives in scripts/dev/)
cd "$(dirname "$0")/../.."

# Load pinned versions
# shellcheck source=../.config/versions.env
source .config/versions.env

#######################################
# Check if a command exists.
# Arguments:
#   $1 — command name
# Returns:
#   0 if found, 1 if not
#######################################
has() { command -v "$1" >/dev/null 2>&1; }

#######################################
# Detect the OS and package manager.
# Sets globals: OS, PKG, IS_MAC, IS_LINUX
#######################################
detect_platform() {
  IS_MAC=false
  IS_LINUX=false

  if [[ "$(uname -s)" == "Darwin" ]]; then
    IS_MAC=true
    if ! has brew; then
      echo "ERROR: macOS detected but Homebrew not found. Install from https://brew.sh" >&2
      exit 1
    fi
    PKG="brew install"
  elif has apt-get; then
    IS_LINUX=true
    PKG="sudo apt-get install -y"
  else
    echo "ERROR: no supported package manager found (brew or apt)" >&2
    exit 1
  fi
}

#######################################
# Install a tool via the platform package manager.
# Arguments:
#   $1 — tool name (display)
#   $2 — brew package name
#   $3 — apt package name (or "SKIP" to skip on apt)
#######################################
install_pkg() {
  local name="$1"
  local brew_pkg="$2"
  local apt_pkg="$3"

  if [[ "${IS_MAC}" == true ]]; then
    echo "  Installing ${name} (brew install ${brew_pkg})..."
    brew install "${brew_pkg}"
  elif [[ "${apt_pkg}" == "SKIP" ]]; then
    echo "  SKIP: ${name} not available via apt (see install instructions below)"
    return 1
  else
    echo "  Installing ${name} (apt-get install ${apt_pkg})..."
    sudo apt-get install -y "${apt_pkg}"
  fi
}

#######################################
# Install LLVM tools (clang-format, clang-tidy) at pinned version.
# On macOS: brew install llvm (provides both, accessible via /opt/homebrew/opt/llvm/bin/)
# On Linux: add LLVM apt repo, install clang-format-$VER and clang-tidy-$VER
#######################################
install_llvm_tools() {
  local ver="${LLVM_VERSION}"

  if [[ "${IS_MAC}" == true ]]; then
    # brew llvm provides clang-format and clang-tidy
    if ! has clang-format; then
      echo "  Installing LLVM ${ver} (brew install llvm)..."
      brew install llvm
      echo "  NOTE: add /opt/homebrew/opt/llvm/bin to your PATH"
    fi
  else
    # On Linux, install from LLVM apt repo for the pinned version
    local need_install=false
    has "clang-format-${ver}" || need_install=true
    has "clang-tidy-${ver}" || need_install=true

    if [[ "${need_install}" == true ]]; then
      echo "  Installing LLVM ${ver} tools from apt.llvm.org..."
      wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
        | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc > /dev/null
      # shellcheck source=/dev/null
      source /etc/os-release
      local codename="${UBUNTU_CODENAME:-${VERSION_CODENAME}}"
      echo "deb http://apt.llvm.org/${codename}/ llvm-toolchain-${codename} main" \
        | sudo tee /etc/apt/sources.list.d/llvm.list > /dev/null
      sudo apt-get update -qq
      sudo apt-get install -y "clang-format-${ver}" "clang-tidy-${ver}"

      # Create symlinks so bare `clang-format` / `clang-tidy` work
      if ! has clang-format; then
        sudo ln -sf "/usr/bin/clang-format-${ver}" /usr/bin/clang-format
      fi
      if ! has clang-tidy; then
        sudo ln -sf "/usr/bin/clang-tidy-${ver}" /usr/bin/clang-tidy
      fi
    fi
  fi
}

#######################################
# Install doxygen at pinned version.
# brew: `brew install doxygen` (latest, usually close enough)
# apt:  direct download from doxygen.nl (apt version is too old)
#######################################
install_doxygen() {
  local ver="${DOXYGEN_VERSION}"
  local current
  current="$(doxygen --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo "0")"

  # Check if installed version is >= required
  if [[ "${current}" != "0" ]]; then
    local min
    min="$(printf '%s\n%s' "${ver}" "${current}" | sort -V | head -n1)"
    if [[ "${min}" == "${ver}" ]]; then
      return 0
    fi
  fi

  if [[ "${IS_MAC}" == true ]]; then
    echo "  Installing doxygen (brew install doxygen)..."
    brew install doxygen
  else
    # Direct download — apt only has 1.9.8, we need 1.16.1
    echo "  Installing doxygen ${ver} (direct download from doxygen.nl)..."
    local url="https://doxygen.nl/files/doxygen-${ver}.linux.bin.tar.gz"
    wget -q "${url}" -O "/tmp/doxygen-${ver}.tar.gz"
    tar -xzf "/tmp/doxygen-${ver}.tar.gz" -C /tmp
    sudo cp "/tmp/doxygen-${ver}/bin/doxygen" /usr/local/bin/doxygen
    rm -rf "/tmp/doxygen-${ver}" "/tmp/doxygen-${ver}.tar.gz"
  fi
}

#######################################
# Install semgrep.
# brew: `brew install semgrep` or `pipx install semgrep`
# apt:  not available — use pipx
#######################################
install_semgrep() {
  if has semgrep; then
    return 0
  fi

  if [[ "${IS_MAC}" == true ]]; then
    echo "  Installing semgrep (brew install semgrep)..."
    brew install semgrep
  else
    # semgrep is not in apt — install via pipx
    if ! has pipx; then
      echo "  Installing pipx first..."
      sudo apt-get install -y pipx
    fi
    echo "  Installing semgrep (pipx install semgrep)..."
    pipx install semgrep
  fi
}

#######################################
# Install git-cliff (release changelog generator).
# brew: `brew install git-cliff`
# apt:  not available — direct download from GitHub releases
#######################################
install_git_cliff() {
  if has git-cliff; then
    return 0
  fi

  local ver="${GIT_CLIFF_VERSION}"

  if [[ "${IS_MAC}" == true ]]; then
    echo "  Installing git-cliff (brew install git-cliff)..."
    brew install git-cliff
  else
    echo "  Installing git-cliff ${ver} (direct download from GitHub)..."
    local arch
    arch="$(uname -m)"
    local url="https://github.com/orhun/git-cliff/releases/download/v${ver}/git-cliff-${ver}-${arch}-unknown-linux-gnu.tar.gz"
    wget -q "${url}" -O "/tmp/git-cliff-${ver}.tar.gz"
    mkdir -p /tmp/git-cliff-extract
    tar -xzf "/tmp/git-cliff-${ver}.tar.gz" -C /tmp/git-cliff-extract
    sudo find /tmp/git-cliff-extract -name "git-cliff" -type f -exec mv {} /usr/local/bin/ \;
    sudo chmod +x /usr/local/bin/git-cliff
    rm -rf /tmp/git-cliff-extract "/tmp/git-cliff-${ver}.tar.gz"
  fi
}

#######################################
# Install ollama (LLM runtime).
# brew: `brew install ollama`
# linux: official curl installer
#######################################
# Install rumdl (Rust markdown linter).
# brew: `brew install rumdl`
# linux: direct download from GitHub releases
#######################################
install_rumdl() {
  if has rumdl; then
    return 0
  fi

  local ver="${RUMDL_VERSION}"

  if [[ "${IS_MAC}" == true ]]; then
    echo "  Installing rumdl (brew install rumdl)..."
    brew install rumdl
  else
    echo "  Installing rumdl ${ver} (direct download from GitHub)..."
    local arch
    arch="$(uname -m)"
    local url="https://github.com/rvben/rumdl/releases/download/v${ver}/rumdl-v${ver}-${arch}-unknown-linux-gnu.tar.gz"
    wget -q "${url}" -O "/tmp/rumdl-${ver}.tar.gz"
    tar -xzf "/tmp/rumdl-${ver}.tar.gz" -C /tmp
    sudo mv /tmp/rumdl /usr/local/bin/rumdl
    sudo chmod +x /usr/local/bin/rumdl
    rm -f "/tmp/rumdl-${ver}.tar.gz"
  fi
}

#######################################
install_ollama() {
  if has ollama; then
    return 0
  fi

  if [[ "${IS_MAC}" == true ]]; then
    echo "  Installing ollama (brew install ollama)..."
    brew install ollama
  else
    echo "  Installing ollama (official installer)..."
    curl -fsSL https://ollama.com/install.sh | sh
  fi
}

#######################################
# Main entry point.
#######################################
main() {
  detect_platform
  echo "==> Checking tools..."

  # Build tools (available in both brew and apt)
  has jq      || install_pkg "jq"      "jq"      "jq"
  has cmake   || install_pkg "cmake"   "cmake"   "cmake"
  has ccache  || install_pkg "ccache"  "ccache"  "ccache"

  # LLVM tools (clang-format, clang-tidy) — special handling per platform
  install_llvm_tools

  # Quality tools (available in both brew and apt)
  has cppcheck   || install_pkg "cppcheck"   "cppcheck"   "cppcheck"
  has pmccabe    || install_pkg "pmccabe"     "pmccabe"    "pmccabe"
  has cloc       || install_pkg "cloc"        "cloc"       "cloc"
  has shellcheck || install_pkg "shellcheck"  "shellcheck" "shellcheck"
  has gitleaks   || install_pkg "gitleaks"    "gitleaks"   "gitleaks"
  has yamllint   || install_pkg "yamllint"    "yamllint"   "yamllint"

  # Tools with platform-specific install methods
  install_doxygen
  install_semgrep
  install_git_cliff
  install_rumdl

  # Coverage tools (linux only, comes with build-essential on mac)
  if [[ "${IS_LINUX}" == true ]]; then
    has lcov || install_pkg "lcov" "lcov" "lcov"
    has bc   || install_pkg "bc"   "bc"   "bc"
  fi

  # Runtime
  install_ollama

  local clang_tidy="clang-tidy"
  if ! has clang-tidy && [[ -f "/opt/homebrew/opt/llvm/bin/clang-tidy" ]]; then
    clang_tidy="/opt/homebrew/opt/llvm/bin/clang-tidy"
  fi

  echo ""
  echo "==> All tools:"
  printf "  %-20s %s\n" "cmake"        "$(cmake --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'missing')"
  printf "  %-20s %s\n" "clang-format" "$(clang-format --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'missing')"
  printf "  %-20s %s\n" "clang-tidy"   "$("${clang_tidy}" --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1 || echo 'missing')"
  printf "  %-20s %s\n" "cppcheck"     "$(cppcheck --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+' || echo 'missing')"
  printf "  %-20s %s\n" "pmccabe"      "$(dpkg -s pmccabe 2>/dev/null | grep '^Version:' | grep -oE '[0-9]+\.[0-9]+' || echo 'missing')"
  printf "  %-20s %s\n" "doxygen"      "$(doxygen --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'missing')"
  printf "  %-20s %s\n" "cloc"         "$(cloc --version 2>/dev/null || echo 'missing')"
  printf "  %-20s %s\n" "shellcheck"   "$(shellcheck --version 2>/dev/null | grep '^version:' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'missing')"
  printf "  %-20s %s\n" "yamllint"     "$(yamllint --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'missing')"
  printf "  %-20s %s\n" "rumdl"        "$(rumdl version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'missing')"
  printf "  %-20s %s\n" "semgrep"      "$(semgrep --version 2>/dev/null || echo 'missing')"
  printf "  %-20s %s\n" "gitleaks"     "$(dpkg -s gitleaks 2>/dev/null | grep '^Version:' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1 || echo "$(has gitleaks && echo 'installed' || echo 'missing')")"
  printf "  %-20s %s\n" "git-cliff"    "$(git-cliff --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'missing')"
  printf "  %-20s %s\n" "ollama"       "$(ollama --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' || echo 'missing')"

  # Install git hooks (without requiring make/cmake)
  if [[ -d .git ]]; then
    echo "==> Installing git hooks..."
    cp scripts/git/pre-commit.sh .git/hooks/pre-commit
    cp scripts/git/pre-push.sh .git/hooks/pre-push
    chmod +x .git/hooks/pre-commit .git/hooks/pre-push
  fi

  echo "==> Setup complete. Run 'make check' to verify."
}

main "$@"
