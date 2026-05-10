#!/usr/bin/env bash
# install-llm-tools-linux.sh — Batch installer for terminal AI/LLM tools (Linux)
# Usage: curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/scripts/install-llm-tools-linux.sh | bash
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'
info() { printf "${GREEN}[✓]${NC} %s\n" "$1"; }
warn() { printf "${YELLOW}[!]${NC} %s\n" "$1"; }
fail() { printf "${RED}[✗]${NC} %s\n" "$1"; }
header() { printf "\n${GREEN}━━━ %s ━━━${NC}\n" "$1"; }

command_exists() { command -v "$1" &>/dev/null; }

check_node() {
  if ! command_exists node; then
    warn "Node.js not found — skipping npm-based tools (gemini, copilot, claude-code)"
    return 1
  fi
  local ver
  ver=$(node -v | sed 's/v//' | cut -d. -f1)
  if [ "$ver" -lt 20 ]; then
    warn "Node.js $ver found but 20+ required — skipping npm-based tools"
    return 1
  fi
  return 0
}

check_python() {
  if command_exists python3; then
    echo "python3"
    return 0
  fi
  if command_exists python; then
    echo "python"
    return 0
  fi
  warn "Python not found — skipping aider"
  return 1
}

header "Terminal AI/LLM Tools Installer (Linux)"
echo "This script installs CLI tools for AI-assisted development."
echo "Tools: llama-cli, kiro-cli, amazon-q, opencode, tgpt, gemini, copilot, claude-code, aider"
echo ""

# ─── llama-cli ───────────────────────────────────────────────────────────────
header "llama-cli (local AI assistant)"
if command_exists hello; then
  info "llama-cli already installed ($(hello --version 2>/dev/null || echo 'unknown'))"
else
  curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/install.sh | bash
  info "llama-cli installed"
fi

# ─── Kiro CLI ────────────────────────────────────────────────────────────────
header "Kiro CLI (AWS agentic IDE in terminal)"
if command_exists kiro-cli; then
  info "kiro-cli already installed"
else
  curl -fsSL https://cli.kiro.dev/install | bash
  info "kiro-cli installed"
fi

# ─── Amazon Q CLI ────────────────────────────────────────────────────────────
header "Amazon Q Developer CLI"
warn "Amazon Q may try to redirect you to install Kiro CLI instead."
warn "If 'q' launches a Kiro installer/wrapper, you can still use the"
warn "underlying Q CLI by running 'q chat' directly or reinstalling from:"
warn "  https://github.com/aws/amazon-q-developer-cli/releases/tag/v1.19.7"
if command_exists q; then
  info "Amazon Q CLI already installed (q command found)"
else
  Q_INSTALL_DIR="${HOME}/.local/bin"
  mkdir -p "$Q_INSTALL_DIR"
  # Detect distro for install method
  if command_exists dpkg && [ -f /etc/debian_version ]; then
    # Ubuntu/Debian: use .deb package
    TMPDIR_Q=$(mktemp -d)
    curl --proto '=https' --tlsv1.2 -sSf "https://desktop-release.q.us-east-1.amazonaws.com/latest/amazon-q.deb" -o "$TMPDIR_Q/amazon-q.deb"
    sudo dpkg -i "$TMPDIR_Q/amazon-q.deb" || sudo apt-get install -f -y
    rm -rf "$TMPDIR_Q"
    info "Amazon Q CLI installed (.deb) — run 'q login' to authenticate"
  else
    # Other distros: use AppImage
    Q_APPIMAGE="$Q_INSTALL_DIR/amazon-q.AppImage"
    curl --proto '=https' --tlsv1.2 -sSf "https://desktop-release.q.us-east-1.amazonaws.com/latest/Amazon-Q-x86_64.AppImage" -o "$Q_APPIMAGE"
    chmod +x "$Q_APPIMAGE"
    # Create symlink so 'q' is on PATH
    ln -sf "$Q_APPIMAGE" "$Q_INSTALL_DIR/q"
    info "Amazon Q CLI installed (AppImage at $Q_APPIMAGE)"
    info "Run '$Q_APPIMAGE' or add $Q_INSTALL_DIR to PATH, then 'q login'"
  fi
fi

# ─── OpenCode ────────────────────────────────────────────────────────────────
header "OpenCode (terminal AI coding agent)"
if command_exists opencode; then
  info "opencode already installed"
else
  curl -fsSL https://opencode.ai/install | bash
  info "opencode installed"
fi

# ─── tgpt ────────────────────────────────────────────────────────────────────
header "tgpt (AI chatbot in terminal, no API keys needed)"
if command_exists tgpt; then
  info "tgpt already installed ($(tgpt --version 2>/dev/null || echo 'unknown'))"
else
  curl -sSL https://raw.githubusercontent.com/aandrew-me/tgpt/main/install | bash -s /usr/local/bin
  info "tgpt installed"
fi

# ─── npm-based tools ─────────────────────────────────────────────────────────
if check_node; then
  # Gemini CLI
  header "Gemini CLI (Google AI in terminal)"
  if command_exists gemini; then
    info "gemini-cli already installed"
  else
    npm install -g @google/gemini-cli
    info "gemini-cli installed"
  fi

  # GitHub Copilot CLI
  header "GitHub Copilot CLI (agentic coding in terminal)"
  if command_exists github-copilot; then
    info "copilot-cli already installed"
  else
    curl -fsSL https://gh.io/copilot-install | bash
    info "copilot-cli installed"
  fi

  # Claude Code
  header "Claude Code (Anthropic AI in terminal)"
  if command_exists claude; then
    info "claude-code already installed"
  else
    curl -fsSL https://claude.ai/install.sh | bash
    info "claude-code installed"
  fi
fi

# ─── Aider ───────────────────────────────────────────────────────────────────
header "Aider (AI pair programming)"
if command_exists aider; then
  info "aider already installed"
else
  PYTHON=$(check_python || true)
  if [ -n "$PYTHON" ]; then
    $PYTHON -m pip install --user aider-install && aider-install
    info "aider installed"
  fi
fi

# ─── Summary ─────────────────────────────────────────────────────────────────
header "Installation complete!"
echo ""
echo "Installed tools:"
for cmd in hello kiro-cli q opencode tgpt gemini github-copilot claude aider; do
  if command_exists "$cmd"; then
    info "$cmd"
  else
    fail "$cmd (not found)"
  fi
done
echo ""
echo "Notes:"
echo "  • Most tools require authentication on first run"
echo "  • Gemini CLI requires a Google account (free tier: 60 req/min)"
echo "  • Copilot CLI requires a GitHub Copilot subscription"
echo "  • Claude Code requires an Anthropic API key or subscription"
echo "  • Aider requires an LLM API key (OpenAI/Anthropic/etc)"
echo "  • llama-cli runs fully offline with Ollama (install: brew install ollama)"
