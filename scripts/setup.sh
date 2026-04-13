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
has jq           || { echo "Installing jq...";           $PKG jq; }
has cmake        || { echo "Installing cmake...";        $PKG cmake; }
has clang-format || { echo "Installing clang-format...";  $PKG clang-format; }
has ccache       || { echo "Installing ccache...";        $PKG ccache; }

# Quality tools
DOXYGEN_VERSION=$(jq -r '.doxygen' .config/versions.json)
has cppcheck     || { echo "Installing cppcheck...";     $PKG cppcheck; }
has clang-tidy   || { echo "Installing clang-tidy...";   if has brew; then $PKG llvm; else $PKG clang-tidy; fi; }
has pmccabe      || { echo "Installing pmccabe...";       $PKG pmccabe; }

# Doxygen (pinned version check)
CURRENT_DOX=$(doxygen --version 2>/dev/null || echo "0")
if [ "$(printf '%s\n%s' "$DOXYGEN_VERSION" "$CURRENT_DOX" | sort -V | head -n1)" != "$DOXYGEN_VERSION" ]; then
  echo "Current doxygen $CURRENT_DOX is older than required $DOXYGEN_VERSION. Installing..."
  if has brew; then
    $PKG doxygen
  else
    $PKG doxygen="$DOXYGEN_VERSION"
  fi
else
  echo "Doxygen $CURRENT_DOX is >= $DOXYGEN_VERSION. Skipping."
fi

has semgrep      || { echo "Installing semgrep...";       $PKG semgrep; }
has gitleaks     || { echo "Installing gitleaks...";      $PKG gitleaks; }
has cloc         || { echo "Installing cloc...";          $PKG cloc; }

# Runtime
has ollama       || { echo "Installing ollama...";        $PKG ollama; }

echo "==> All dependencies installed."

# Install git hooks (without requiring make/cmake)
echo "==> Installing git hooks..."
cp .config/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit

echo "==> Setup complete. Run 'make check' to verify."
