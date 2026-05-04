#!/usr/bin/env bash
#
# gen-test-images.sh — Generate clean and "dirty" PNG files for testing zsteg.
#
# Usage:
#   bash scripts/security/gen-test-images.sh

set -o errexit
set -o nounset
set -o pipefail

mkdir -p tests/security

# 1x1 pixel PNG (hex format)
# Header + IHDR + IDAT + IEND
PNG_HEX="89504e470d0a1a0a0000000d49484452000000010000000108060000001f15c4890000000a49444154789c63000100000500010d0a2db40000000049454e44ae426082"

# Function to convert hex to binary file
hex_to_bin() {
  local hex="$1"
  local output="$2"
  # Use python3 as a reliable hex-to-bin converter if xxd is missing, 
  # but we want to stick to shell if possible. 
  # However, printf '\xHH' is the most standard shell way.
  
  # Prepare the printf string: 89 -> \x89
  local fmt=""
  for (( i=0; i<${#hex}; i+=2 )); do
    fmt="${fmt}\\x${hex:$i:2}"
  done
  printf "${fmt}" > "${output}"
}

echo "==> generating test images..."
hex_to_bin "${PNG_HEX}" "tests/security/clean.png"

# Create a "dirty" image by appending data (extradata steganography)
cp tests/security/clean.png tests/security/steg.png
echo "THIS_IS_HIDDEN_DATA_12345" >> tests/security/steg.png

echo "  created tests/security/clean.png"
echo "  created tests/security/steg.png (contains hidden data)"
