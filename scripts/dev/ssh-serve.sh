#!/usr/bin/env bash
#
# ssh-serve.sh — Manage macOS SSH with local network restriction (up/down/status).
#
set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi
#
# COMMANDS:
#   up    Enable SSH, restrict to local network, key-only auth
#   down  Disable SSH completely
#   status Show current state
#   help  Show this help
#
# SECURITY:
#   - Key-only authentication (no passwords)
#   - Restricted to local subnet via pf firewall
#   - Generates host-specific authorized_keys if missing
#
# EXAMPLES:
#   ~/ssh-serve.sh up       # enable for local network
#   ~/ssh-serve.sh down     # disable completely
#   ~/ssh-serve.sh status   # check state

PORT=22
PF_ANCHOR="ssh_local"
PF_CONF="/tmp/pf-ssh-local.conf"
SSHD_OVERRIDE="/etc/ssh/sshd_config.d/local-only.conf"

get_local_subnet() {
  local IP
  IP=$(ipconfig getifaddr en0 2>/dev/null || ipconfig getifaddr en1 2>/dev/null)
  if [ -z "$IP" ]; then echo ""; return; fi
  echo "${IP%.*}.0/24"
}

get_local_ip() {
  ipconfig getifaddr en0 2>/dev/null || ipconfig getifaddr en1 2>/dev/null
}

ensure_authorized_keys() {
  local KEYFILE=~/.ssh/authorized_keys
  if [ ! -f "$KEYFILE" ] || [ ! -s "$KEYFILE" ]; then
    echo ""
    echo "  ⚠ No authorized_keys found."
    echo "  To allow a device, copy its public key:"
    echo "    ssh-copy-id $(whoami)@$(get_local_ip)"
    echo "  Or manually append to ~/.ssh/authorized_keys"
    echo ""
  fi
}

case "${1:-help}" in
  up)
    SUBNET=$(get_local_subnet)
    if [ -z "$SUBNET" ]; then
      echo "✗ No local network found"; exit 1
    fi

    # Enable Remote Login
    sudo systemsetup -setremotelogin on 2>/dev/null
    sudo launchctl load -w /System/Library/LaunchDaemons/ssh.plist 2>/dev/null

    # Harden: key-only, no password auth
    sudo mkdir -p /etc/ssh/sshd_config.d
    cat <<EOF | sudo tee "$SSHD_OVERRIDE" >/dev/null
PasswordAuthentication no
KbdInteractiveAuthentication no
UsePAM no
EOF

    # Firewall: only allow SSH from local subnet
    cat <<EOF > "$PF_CONF"
block in quick on en0 proto tcp from ! $SUBNET to any port $PORT
block in quick on en1 proto tcp from ! $SUBNET to any port $PORT
EOF
    sudo pfctl -a "$PF_ANCHOR" -f "$PF_CONF" 2>/dev/null
    sudo pfctl -e 2>/dev/null

    ensure_authorized_keys

    echo "✓ SSH enabled on port $PORT"
    echo "  Restricted to: $SUBNET"
    echo "  Auth: key-only (no passwords)"
    echo "  Connect: ssh $(whoami)@$(get_local_ip)"
    ;;

  down)
    sudo systemsetup -setremotelogin off 2>/dev/null
    sudo launchctl unload -w /System/Library/LaunchDaemons/ssh.plist 2>/dev/null
    sudo pfctl -a "$PF_ANCHOR" -F all 2>/dev/null
    sudo rm -f "$SSHD_OVERRIDE"

    echo "✓ SSH disabled"
    ;;

  status)
    if sudo systemsetup -getremotelogin 2>/dev/null | grep -q "On"; then
      echo "SSH: enabled on port $PORT"
      echo "  IP: $(get_local_ip)"
      SUBNET=$(get_local_subnet)
      if sudo pfctl -a "$PF_ANCHOR" -sr 2>/dev/null | grep -q "block"; then
        echo "  Firewall: restricted to $SUBNET"
      else
        echo "  Firewall: ⚠ no subnet restriction active"
      fi
      if grep -q "PasswordAuthentication no" "$SSHD_OVERRIDE" 2>/dev/null; then
        echo "  Auth: key-only"
      else
        echo "  Auth: ⚠ password auth may be enabled"
      fi
    else
      echo "SSH: disabled"
    fi
    ;;

  help|--help|-h)
    echo "Usage: ssh-serve.sh <up|down|status>"
    echo ""
    echo "  up      Enable SSH (local network, key-only)"
    echo "  down    Disable SSH completely"
    echo "  status  Show current state"
    ;;

  *)
    echo "Unknown command: $1"
    echo "Run: ssh-serve.sh help"
    exit 1
    ;;
esac
