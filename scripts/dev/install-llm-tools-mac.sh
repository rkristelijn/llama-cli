#!/usr/bin/env bash
# install-llm-tools-mac.sh — Batch installer for terminal AI/LLM tools (macOS)
# Usage: curl -fsSL https://raw.githubusercontent.com/rkristelijn/llama-cli/main/scripts/install-llm-tools-mac.sh | bash
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'
info()  { printf "${GREEN}[✓]${NC} %s\n" "$1"; }
warn()  { printf "${YELLOW}[!]${NC} %s\n" "$1"; }
fail()  { printf "${RED}[✗]${NC} %s\n" "$1"; }
header(){ printf "\n${GREEN}━━━ %s ━━━${NC}\n" "$1"; }

command_exists() { command -v "$1" &>/dev/null; }

# ─── Prerequisites ───────────────────────────────────────────────────────────
header "Terminal AI/LLM Tools Installer (macOS)"
echo "This script installs CLI tools for AI-assisted development."
echo "Tools: llama-cli, kiro-cli, amazon-q, opencode, tgpt, gemini, copilot, claude-code, aider"
echo ""

if ! command_exists brew; then
  warn "Homebrew not found — installing..."
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

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
  brew install amazon-q
  info "Amazon Q CLI installed — run 'q login' to authenticate"
fi

# ─── OpenCode ────────────────────────────────────────────────────────────────
header "OpenCode (terminal AI coding agent)"
if command_exists opencode; then
  info "opencode already installed"
else
  brew install anomalyco/tap/opencode
  info "opencode installed"
fi

# ─── tgpt ────────────────────────────────────────────────────────────────────
header "tgpt (AI chatbot in terminal, no API keys needed)"
if command_exists tgpt; then
  info "tgpt already installed ($(tgpt --version 2>/dev/null || echo 'unknown'))"
else
  brew install tgpt
  info "tgpt installed"
fi

# ─── Gemini CLI ──────────────────────────────────────────────────────────────
header "Gemini CLI (Google AI in terminal)"
if command_exists gemini; then
  info "gemini-cli already installed"
else
  npm install -g @google/gemini-cli
  info "gemini-cli installed"
fi

# ─── GitHub Copilot CLI ──────────────────────────────────────────────────────
header "GitHub Copilot CLI (agentic coding in terminal)"
if command_exists github-copilot; then
  info "copilot-cli already installed"
else
  brew install copilot-cli
  info "copilot-cli installed"
fi

# ─── Claude Code ─────────────────────────────────────────────────────────────
header "Claude Code (Anthropic AI in terminal)"
if command_exists claude; then
  info "claude-code already installed"
else
  curl -fsSL https://claude.ai/install.sh | bash
  info "claude-code installed"
fi

# ─── Aider ───────────────────────────────────────────────────────────────────
header "Aider (AI pair programming)"
if command_exists aider; then
  info "aider already installed"
else
  python3 -m pip install aider-install && aider-install
  info "aider installed"
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
