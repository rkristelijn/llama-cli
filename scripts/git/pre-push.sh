#!/usr/bin/env bash
#
# pre-push — Run appropriate checks before pushing based on what changed.
#
# If code changed: runs make check. If only docs: runs index check.
#
# Installed by: make hooks / make setup
# Location:     .git/hooks/pre-push (symlink or copy from scripts/git/pre-push.sh)
#
# @see docs/adr/adr-44-tidy-boilerplate.md

set -o errexit
set -o nounset
set -o pipefail

exec bash scripts/dev/prepush.sh
