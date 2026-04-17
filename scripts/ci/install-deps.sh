#!/usr/bin/env bash
#
# install-deps.sh — Install CI dependencies from versions.env.
#
# Accepts tool names as arguments. Installs only what's requested.
# Designed for Ubuntu CI runners (apt-get).
#
# Usage:
#   bash scripts/ci/install-deps.sh llvm          # clang-format + clang-tidy
#   bash scripts/ci/install-deps.sh cppcheck pmccabe cloc
#   bash scripts/ci/install-deps.sh doxygen       # direct download (pinned)
#   bash scripts/ci/install-deps.sh rumdl         # direct download (pinned)
#   bash scripts/ci/install-deps.sh yamllint
#   bash scripts/ci/install-deps.sh lcov bc
#
# @see .config/versions.env
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then set -o xtrace; fi

# shellcheck source=../../.config/versions.env
source .config/versions.env

install_llvm() {
  wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
    | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc > /dev/null
  # shellcheck source=/dev/null
  source /etc/os-release
  local codename="${UBUNTU_CODENAME:-${VERSION_CODENAME}}"
  echo "deb http://apt.llvm.org/${codename}/ llvm-toolchain-${codename} main" \
    | sudo tee /etc/apt/sources.list.d/llvm.list > /dev/null
  sudo apt-get update -qq
  sudo apt-get install -y "clang-format-${LLVM_VERSION}" "clang-tidy-${LLVM_VERSION}"
  sudo ln -sf "/usr/bin/clang-format-${LLVM_VERSION}" /usr/bin/clang-format
  sudo ln -sf "/usr/bin/clang-tidy-${LLVM_VERSION}" /usr/bin/clang-tidy
}

install_doxygen() {
  wget -q "https://doxygen.nl/files/doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz"
  tar -xzf "doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz"
  sudo cp "doxygen-${DOXYGEN_VERSION}/bin/doxygen" /usr/local/bin/doxygen
  rm -rf "doxygen-${DOXYGEN_VERSION}" "doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz"
}

install_rumdl() {
  curl -LsSf "https://github.com/rvben/rumdl/releases/download/v${RUMDL_VERSION}/rumdl-v${RUMDL_VERSION}-x86_64-unknown-linux-gnu.tar.gz" \
    | sudo tar xzf - -C /usr/local/bin
}

main() {
  for tool in "$@"; do
    case "${tool}" in
      llvm)      install_llvm ;;
      doxygen)   install_doxygen ;;
      rumdl)     install_rumdl ;;
      *)         sudo apt-get install -y "${tool}" ;;
    esac
  done
}

main "$@"
