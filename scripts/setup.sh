#!/bin/sh
# setup.sh — Install all development dependencies
# Detects macOS (brew) or Linux (apt) and installs what's missing.

set -e

# Check if a command exists
has() { command -v "$1" >/dev/null 2>&1; }

# Detect package manager
if has brew; then
  PKG="brew install"
elif has apt-get; then
  PKG="sudo apt-get install -y"
else
  echo "ERROR: no supported package manager found (brew or apt)"
  exit 1
fi

echo "==> Checking dependencies..."

# Build tools
has cmake        || { echo "Installing cmake...";        $PKG cmake; }
has clang-format || { echo "Installing clang-format...";  $PKG clang-format; }

# Quality tools
has cppcheck     || { echo "Installing cppcheck...";     $PKG cppcheck; }
has semgrep      || { echo "Installing semgrep...";       $PKG semgrep; }
has gitleaks     || { echo "Installing gitleaks...";      $PKG gitleaks; }
has cloc         || { echo "Installing cloc...";          $PKG cloc; }

# Runtime
has ollama       || { echo "Installing ollama...";        $PKG ollama; }

echo "==> All dependencies installed."

# Install git hooks
echo "==> Installing git hooks..."
make install

echo "==> Setup complete. Run 'make check' to verify."
