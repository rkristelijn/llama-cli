#!/usr/bin/env bash
#
# ollama-serve.sh — Manage Ollama with network control (up/net/down/status).
#
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi
#
# COMMANDS:
#   up    Start Ollama on localhost only (127.0.0.1:11434)
#   net   Start Ollama exposed to network (0.0.0.0:11434)
#   down  Stop Ollama completely, free all resources
#   help  Show this help
#
# EXAMPLES:
#   ~/ollama-serve.sh up     # safe local mode
#   ~/ollama-serve.sh net    # share models on network
#   ~/ollama-serve.sh down   # shut it all down
#
# NOTE: The homebrew launchd plist has KeepAlive=true, which auto-respawns
#       ollama on kill. Every command unloads it first to prevent that.
#
# ─── M2 Pro 32GB Tuning Notes ───────────────────────────────────────────
#
# BOTTLENECK: Memory bandwidth (200 GB/s unified). LLM token generation
# is memory-bandwidth-bound — each token requires reading the full model
# weights. Discrete GPUs (RTX 3080: 760 GB/s, RTX 4090: 1008 GB/s) have
# 2-5x more bandwidth, so they're faster on small models (7B-14B).
#
# ADVANTAGE: 32GB unified memory. Models up to ~26GB fit entirely in GPU
# memory without PCIe bottleneck. A 31B model on this Mac beats a 7B
# model on a 12GB GPU in quality, even if slower in raw t/s.
#
# SETTINGS RATIONALE:
#
#   OLLAMA_FLASH_ATTENTION=1
#     Uses Flash Attention v2 for Metal. Reduces memory usage for the
#     attention computation from O(n²) to O(n), enabling larger context
#     windows without extra memory. ~20% faster prompt processing.
#
#   OLLAMA_KV_CACHE_TYPE=q4_0
#     Quantizes the KV cache to 4-bit instead of f16 (default) or q8_0.
#     KV cache for 32K context at f16 ≈ 2GB, at q8_0 ≈ 1GB, at q4_0 ≈ 0.5GB.
#     This frees ~1.5GB for model weights, critical for fitting 31B models
#     (19GB weights + KV cache) in the ~27GB available GPU memory.
#     Quality loss is negligible — KV cache quantization has minimal
#     impact on output quality compared to weight quantization.
#
#   OLLAMA_NUM_PARALLEL=1
#     Single-user mode. Each parallel slot reserves its own KV cache,
#     so parallel=2 would double KV memory usage. Keep at 1 unless
#     you're serving multiple users simultaneously.
#
#   OLLAMA_KEEP_ALIVE=30m
#     Keep model loaded for 30 minutes (default 5m). Loading a 19GB
#     model from SSD takes ~3-5 seconds. Longer keep-alive avoids
#     repeated cold starts during interactive sessions.
#
#   OLLAMA_MAX_LOADED_MODELS=1
#     Only keep 1 model in memory. With 32GB total (minus ~5GB for macOS),
#     loading two large models would force swap and kill performance.
#
# BENCHMARK REFERENCE (this machine, 2026-04-26):
#   gemma4-uncensored:e4b (6.3GB) → ~63 t/s generation, 196 t/s prompt
#   gemma4:31b (19GB) → fits in memory, ~15-20 t/s expected
#
# ─────────────────────────────────────────────────────────────────────────

PLIST=~/Library/LaunchAgents/homebrew.mxcl.ollama.plist
PORT=11434
LOG=/tmp/ollama.log

# M2 Pro 32GB optimized settings
OLLAMA_SETTINGS=(
  OLLAMA_FLASH_ATTENTION=1
  OLLAMA_KV_CACHE_TYPE=q4_0
  OLLAMA_NUM_PARALLEL=1
  OLLAMA_KEEP_ALIVE=30m
  OLLAMA_MAX_LOADED_MODELS=1
)

stop_ollama() {
  launchctl unload "$PLIST" 2>/dev/null
  local PIDS
  PIDS=$(pgrep -f "ollama serve" 2>/dev/null)
  if [ -n "$PIDS" ]; then
    kill $PIDS 2>/dev/null
    for pid in $PIDS; do
      while kill -0 "$pid" 2>/dev/null; do :; done
    done
  fi
}

start_ollama() {
  local HOST=$1
  export OLLAMA_HOST=$HOST
  for setting in "${OLLAMA_SETTINGS[@]}"; do
    export "$setting"
  done
  nohup ollama serve > "$LOG" 2>&1 &
  sleep 2
}

verify() {
  local MODE=$1
  local MY_IP
  MY_IP=$(ipconfig getifaddr en0 2>/dev/null || ipconfig getifaddr en1 2>/dev/null)
  local OK=true

  echo ""
  echo "  Verification:"

  if [ "$MODE" = "down" ]; then
    if pgrep -f "ollama serve" >/dev/null 2>&1; then
      echo "  ✗ Process still running"; OK=false
    else
      echo "  ✓ No ollama process"
    fi
    if lsof -iTCP:$PORT -sTCP:LISTEN >/dev/null 2>&1; then
      echo "  ✗ Port $PORT still open"; OK=false
    else
      echo "  ✓ Port $PORT free"
    fi
  else
    if ! curl -s --connect-timeout 2 http://127.0.0.1:$PORT >/dev/null; then
      echo "  ✗ Not reachable on localhost"; OK=false
    else
      echo "  ✓ Reachable on localhost"
    fi

    # Show active settings
    echo "  ✓ Flash Attention: $OLLAMA_FLASH_ATTENTION"
    echo "  ✓ KV Cache: $OLLAMA_KV_CACHE_TYPE"
    echo "  ✓ Keep Alive: $OLLAMA_KEEP_ALIVE"
    echo "  ✓ Max Loaded Models: $OLLAMA_MAX_LOADED_MODELS"

    if [ "$MODE" = "net" ]; then
      if ! curl -s --connect-timeout 2 http://$MY_IP:$PORT >/dev/null; then
        echo "  ✗ Not reachable on $MY_IP"; OK=false
      else
        echo "  ✓ Reachable on $MY_IP (network exposed)"
      fi
    else
      if curl -s --connect-timeout 2 http://$MY_IP:$PORT >/dev/null 2>&1; then
        echo "  ✗ Still reachable on $MY_IP (should be localhost only)"; OK=false
      else
        echo "  ✓ Not reachable on $MY_IP (localhost only)"
      fi
    fi
  fi

  $OK || exit 1
}

case "${1:-help}" in
  up)
    stop_ollama
    start_ollama 127.0.0.1
    echo "✓ Ollama running on 127.0.0.1:$PORT (localhost only)"
    echo "  Log: $LOG"
    verify up
    ;;
  net)
    stop_ollama
    start_ollama 0.0.0.0
    echo "✓ Ollama running on 0.0.0.0:$PORT (network exposed)"
    echo "  Log: $LOG"
    verify net
    ;;
  down)
    stop_ollama
    echo "✓ Ollama stopped"
    verify down
    ;;
  status)
    if ! pgrep -f "ollama serve" >/dev/null 2>&1; then
      echo "Ollama: stopped"
    elif lsof -iTCP:$PORT -sTCP:LISTEN -nP 2>/dev/null | tail -n +2 | grep -v '127.0.0.1\|\[::1\]' | grep -q .; then
      echo "Ollama: running (network exposed on 0.0.0.0:$PORT)"
    else
      echo "Ollama: running (localhost only on 127.0.0.1:$PORT)"
    fi
    ;;
  help|--help|-h)
    echo "Usage: ollama-serve.sh <up|net|down|status>"
    echo ""
    echo "  up      Start on localhost only (safe)"
    echo "  net     Start exposed to network (share models)"
    echo "  down    Stop completely (free resources)"
    echo "  status  Show current state"
    ;;
  *)
    echo "Unknown command: $1"
    echo "Run: ollama-serve.sh help"
    exit 1
    ;;
esac
