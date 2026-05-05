#!/usr/bin/env bash
#
# ollama-remote.sh — Remote Ollama management via SSH.
# Check hardware, recommend models, pull/list models on remote hosts.
#
# COMMANDS:
#   info HOST    Show hardware info via inxi (RAM, CPU, GPU)
#   recommend HOST  Recommend models based on available RAM
#   pull HOST MODEL Pull a model on the remote host
#   list HOST    List models available on the remote host
#   start HOST   Start ollama in network mode on remote host
#   stop HOST    Stop ollama on remote host
#   status HOST  Show ollama status on remote host
#   help         Show this help
#
# EXAMPLES:
#   ./ollama-remote.sh info pepper.local
#   ./ollama-remote.sh recommend pepper.local
#   ./ollama-remote.sh pull pepper.local gemma4:e4b
#   ./ollama-remote.sh list pepper.local
#   ./ollama-remote.sh start pepper.local
#
# REQUIRES: SSH key-based access to the remote host, inxi on remote host.
#
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# ─── Helpers ─────────────────────────────────────────────────────────────

# ssh_cmd — Run a command on the remote host.
# Arguments: HOST, COMMAND...
ssh_cmd() {
  local host=$1; shift
  ssh -o ConnectTimeout=5 "$host" "$@" 2>/dev/null
}

# get_ram_gb — Parse total RAM in GB from inxi output.
# Arguments: INXI_OUTPUT (stdin)
get_ram_gb() {
  grep -i "Memory" | grep -oP 'total:\s*\K[0-9.]+' | head -1
}

# get_cpu_info — Parse CPU model from inxi output.
# Arguments: INXI_OUTPUT (stdin)
get_cpu_info() {
  grep -i "model" | grep -oP 'model\s*\K[^\x03]+' | head -1 | sed 's/\x1b\[[0-9;]*m//g'
}

# recommend_models — Suggest models based on available RAM.
# Arguments: RAM_GB (integer)
recommend_models() {
  local ram=$1
  echo "  Recommended models for ${ram}GB RAM:"
  echo ""
  if (( ram >= 64 )); then
    echo "  ★ gemma4:31b        (19GB) — best quality, fits easily"
    echo "  ★ qwen2.5-coder:32b (18GB) — excellent for code"
    echo "  ✓ gemma4:e4b        (6GB)  — fast general purpose"
    echo "  ✓ qwen2.5-coder:14b (9GB)  — good code model"
  elif (( ram >= 32 )); then
    echo "  ★ gemma4:31b        (19GB) — fits tight, best quality"
    echo "  ✓ qwen2.5-coder:14b (9GB)  — good code model"
    echo "  ✓ gemma4:e4b        (6GB)  — fast general purpose"
  elif (( ram >= 16 )); then
    echo "  ★ qwen2.5-coder:14b (9GB)  — best code model that fits"
    echo "  ✓ gemma4:e4b        (6GB)  — fast general purpose"
    echo "  ✓ phi3:medium       (8GB)  — good reasoning"
    echo "  ✓ llama3.1:8b       (5GB)  — solid all-rounder"
  elif (( ram >= 8 )); then
    echo "  ★ gemma4:e4b        (6GB)  — best fit for 8GB"
    echo "  ✓ phi3:mini         (2GB)  — lightweight, fast"
    echo "  ✓ qwen2.5:3b        (2GB)  — small but capable"
    echo "  ⚠ CPU-only, expect 5-10 t/s"
  else
    echo "  ✓ phi3:mini         (2GB)  — only realistic option"
    echo "  ✓ qwen2.5:0.5b     (0.4GB) — tiny, fast"
    echo "  ⚠ Very limited, CPU-only"
  fi
}

# ─── Commands ────────────────────────────────────────────────────────────

cmd_info() {
  local host=$1
  echo "==> Hardware info for $host"
  echo ""

  local inxi_out
  inxi_out=$(ssh_cmd "$host" "inxi -c 0 -Cm 2>/dev/null || echo 'inxi not installed'")

  # Parse key info
  local ram cpu gpu
  ram=$(echo "$inxi_out" | grep -oP 'total:\s*\K[0-9.]+' | head -1)
  cpu=$(echo "$inxi_out" | grep -oP 'model:\s*\K[^\s].*' | head -1)
  gpu=$(ssh_cmd "$host" "lspci 2>/dev/null | grep -i 'vga\|3d\|nvidia' | sed 's/.*: //'")

  echo "  Host:   $host"
  echo "  RAM:    ${ram:-?} GiB"
  echo "  CPU:    ${cpu:-?}"
  echo "  GPU:    ${gpu:-integrated only}"
  echo ""

  # Check if ollama is installed
  if ssh_cmd "$host" "command -v ollama" >/dev/null 2>&1; then
    echo "  Ollama: installed"
  else
    echo "  Ollama: ✗ NOT installed"
  fi
}

cmd_recommend() {
  local host=$1
  echo "==> Model recommendations for $host"
  echo ""

  local ram
  ram=$(ssh_cmd "$host" "free --giga 2>/dev/null | awk '/Mem:/{print \$2}'" || echo "")

  if [[ -z "$ram" || "$ram" == "0" ]]; then
    echo "  ✗ Could not determine RAM"; exit 1
  fi

  local gpu
  gpu=$(ssh_cmd "$host" "lspci 2>/dev/null | grep -i nvidia" || echo "")

  echo "  RAM: ${ram}GB | GPU: ${gpu:-CPU-only (no discrete GPU)}"
  echo ""
  recommend_models "$ram"
}

cmd_pull() {
  local host=$1 model=$2
  echo "==> Pulling $model on $host..."
  ssh -o ConnectTimeout=5 "$host" "ollama pull $model"
  echo ""
  echo "✓ $model pulled on $host"
}

cmd_list() {
  local host=$1
  echo "==> Models on $host:"
  echo ""
  ssh_cmd "$host" "ollama list 2>/dev/null || echo '  (ollama not running or not installed)'"
}

cmd_start() {
  local host=$1
  echo "==> Starting ollama on $host (network mode)..."
  ssh_cmd "$host" "~/ollama-serve.sh net"
}

cmd_stop() {
  local host=$1
  echo "==> Stopping ollama on $host..."
  ssh_cmd "$host" "~/ollama-serve.sh down"
}

cmd_status() {
  local host=$1
  ssh_cmd "$host" "~/ollama-serve.sh status"
}

# ─── Main ────────────────────────────────────────────────────────────────

case "${1:-help}" in
info)
  [[ -z "${2:-}" ]] && { echo "Usage: ollama-remote.sh info HOST"; exit 1; }
  cmd_info "$2"
  ;;
recommend)
  [[ -z "${2:-}" ]] && { echo "Usage: ollama-remote.sh recommend HOST"; exit 1; }
  cmd_recommend "$2"
  ;;
pull)
  [[ -z "${2:-}" || -z "${3:-}" ]] && { echo "Usage: ollama-remote.sh pull HOST MODEL"; exit 1; }
  cmd_pull "$2" "$3"
  ;;
list)
  [[ -z "${2:-}" ]] && { echo "Usage: ollama-remote.sh list HOST"; exit 1; }
  cmd_list "$2"
  ;;
start)
  [[ -z "${2:-}" ]] && { echo "Usage: ollama-remote.sh start HOST"; exit 1; }
  cmd_start "$2"
  ;;
stop)
  [[ -z "${2:-}" ]] && { echo "Usage: ollama-remote.sh stop HOST"; exit 1; }
  cmd_stop "$2"
  ;;
status)
  [[ -z "${2:-}" ]] && { echo "Usage: ollama-remote.sh status HOST"; exit 1; }
  cmd_status "$2"
  ;;
help | --help | -h)
  echo "Usage: ollama-remote.sh <command> HOST [MODEL]"
  echo ""
  echo "  info HOST         Hardware info (RAM, CPU, GPU)"
  echo "  recommend HOST    Suggest models based on hardware"
  echo "  pull HOST MODEL   Pull a model on remote host"
  echo "  list HOST         List installed models"
  echo "  start HOST        Start ollama (network mode)"
  echo "  stop HOST         Stop ollama"
  echo "  status HOST       Show ollama status"
  ;;
*)
  echo "Unknown command: $1"
  echo "Run: ollama-remote.sh help"
  exit 1
  ;;
esac
