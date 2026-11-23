#!/usr/bin/env bash

# .SYNOPSIS
# Initializes the Git submodules of the project, including the nested ones.
#
# .DESCRIPTION
# Checks out every submodule this repository pins (and, recursively, whatever submodules
# those pin) at exactly the commit it is pinned to: the script never moves a pin.

# Project root directory (one level up from the script's location)
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd -P)"

# Default arguments
if [ $# -eq 0 ]; then
    ARGUMENTS=("--depth" "1")
else
    ARGUMENTS=("$@")
fi

cd "$ROOT_DIR" || { echo "Failed to enter root directory"; exit 1; }

echo "Initializing submodules (recursive) with: ${ARGUMENTS[*]}"
git submodule update --init --recursive --force --jobs 8 "${ARGUMENTS[@]}"
