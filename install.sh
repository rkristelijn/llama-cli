#!/usr/bin/env bash
#
# install.sh — Download and install llama-cli from GitHub Releases.
#
# Detects OS and architecture, downloads the correct binary, verifies
# integrity via SHA-256 checksum, and installs to /usr/local/bin
# (or a custom directory via INSTALL_DIR).
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | bash
#   INSTALL_DIR=~/.local/bin bash install.sh
#
# Environment:
#   INSTALL_DIR — Override install directory (default: /usr/local/bin)
#   VERSION     — Install a specific version (default: latest)

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

REPO="rkristelijn/llama-cli"
INSTALL_DIR="${INSTALL_DIR:-/usr/local/bin}"

#######################################
# Print an error message and exit.
# Arguments:
#   $1 — error message
#######################################
die() {
  echo "ERROR: $1" >&2
  exit 1
}

#######################################
# Detect OS and architecture, set BINARY name.
# Sets globals: BINARY
#######################################
detect_platform() {
  local os arch
  os="$(uname -s)"
  arch="$(uname -m)"

  case "$os" in
    Linux)
      case "$arch" in
        x86_64)        BINARY="llama-cli-linux-x64" ;;
        aarch64|arm64) BINARY="llama-cli-linux-arm64" ;;
        *) die "Unsupported Linux architecture: $arch" ;;
      esac
      ;;
    Darwin)
      case "$arch" in
        arm64) BINARY="llama-cli-macos-arm64" ;;
        *) die "Unsupported macOS architecture: $arch (only Apple Silicon is supported)" ;;
      esac
      ;;
    *) die "Unsupported OS: $os" ;;
  esac
}

#######################################
# Resolve the download URL for the release binary.
# Sets globals: URL, RESOLVED_VERSION
#######################################
resolve_url() {
  if [[ -n "${VERSION-}" ]]; then
    RESOLVED_VERSION="$VERSION"
    URL="https://github.com/$REPO/releases/download/v${VERSION}/${BINARY}"
  else
    URL="https://github.com/$REPO/releases/latest/download/${BINARY}"
    RESOLVED_VERSION="latest"
  fi
}

#######################################
# Download a URL to a local file using curl or wget.
# Arguments:
#   $1 — URL to download
#   $2 — output file path
#######################################
download() {
  if command -v curl >/dev/null 2>&1; then
    curl -fSL --progress-bar "$1" -o "$2"
  elif command -v wget >/dev/null 2>&1; then
    wget -q --show-progress "$1" -O "$2"
  else
    die "Neither curl nor wget found. Install one and try again."
  fi
}

#######################################
# Verify SHA-256 checksum of a file.
# Arguments:
#   $1 — file to verify
#   $2 — expected checksum string ("hash  filename")
# Returns:
#   0 on match, dies on mismatch
#######################################
verify_checksum() {
  local file="$1" expected="$2"
  local expected_hash
  expected_hash="$(echo "$expected" | awk '{print $1}')"

  local actual_hash
  if command -v sha256sum >/dev/null 2>&1; then
    actual_hash="$(sha256sum "$file" | awk '{print $1}')"
  elif command -v shasum >/dev/null 2>&1; then
    actual_hash="$(shasum -a 256 "$file" | awk '{print $1}')"
  else
    echo "Warning: sha256sum/shasum not found, skipping checksum verification"
    return 0
  fi

  if [[ "$actual_hash" != "$expected_hash" ]]; then
    die "Checksum mismatch! Expected: $expected_hash Got: $actual_hash"
  fi
  echo "✓ Checksum verified"
}

#######################################
# Ensure INSTALL_DIR exists and sudo is available when needed.
# Sets globals: USE_SUDO
#######################################
ensure_install_dir() {
  USE_SUDO=""

  if [[ ! -d "$INSTALL_DIR" ]]; then
    if [[ -w "$(dirname "$INSTALL_DIR")" ]]; then
      mkdir -p "$INSTALL_DIR"
    else
      command -v sudo >/dev/null 2>&1 || die "sudo is required to create $INSTALL_DIR"
      USE_SUDO="sudo"
      $USE_SUDO mkdir -p "$INSTALL_DIR"
    fi
  fi

  if [[ ! -w "$INSTALL_DIR" ]]; then
    command -v sudo >/dev/null 2>&1 || die "sudo is required to install to $INSTALL_DIR"
    USE_SUDO="sudo"
  fi
}

#######################################
# Download, verify, and install the binary.
#######################################
install_binary() {
  local tmpfile checksum_file
  tmpfile="$(mktemp)"
  checksum_file="$(mktemp)"
  trap 'rm -f "$tmpfile" "$checksum_file"' EXIT

  echo "Downloading llama-cli ($RESOLVED_VERSION) for $(uname -s)/$(uname -m)..."
  download "$URL" "$tmpfile"

  # Verify checksum if available
  local checksum_url="${URL}.sha256"
  if download "$checksum_url" "$checksum_file" 2>/dev/null; then
    verify_checksum "$tmpfile" "$(cat "$checksum_file")"
  else
    echo "Note: No checksum file available, skipping verification"
  fi

  chmod +x "$tmpfile"

  ensure_install_dir

  if [[ -n "$USE_SUDO" ]]; then
    echo "Installing to $INSTALL_DIR (requires sudo)..."
  fi
  $USE_SUDO mv "$tmpfile" "$INSTALL_DIR/llama-cli"

  echo ""
  echo "✓ llama-cli installed to $INSTALL_DIR/llama-cli"
  echo ""
  echo "Get started:"
  echo "  llama-cli              # interactive chat"
  echo "  llama-cli --help       # see all options"
  echo ""
  echo "Requires Ollama running locally: https://ollama.com"
}

main() {
  detect_platform
  resolve_url
  install_binary
}

main "$@"
