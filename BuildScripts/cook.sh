#!/bin/bash

set -e  # Exit on error

# Run from the repo root regardless of where this script is called from.
cd "$(dirname "${BASH_SOURCE[0]}")/.." || exit 1

BUILD_TYPE="Debug"

if [[ -n "$1" ]]; then
    ARG="$1"
    case "${ARG,,}" in
        "release") BUILD_TYPE="Release" ;;
        "debug") BUILD_TYPE="Debug" ;;
        *)
            echo "Invalid Build Type: $ARG"
            echo "Valid Build Types: Debug, Release"
            echo "Defaulting to Debug"
            ;;
    esac
fi

echo "Selected Build Type: $BUILD_TYPE"

# Determine architecture (matching CMakeLists.txt logic)
ARCH_DIR="x64"
if [[ "$(uname -m)" == "aarch64" ]]; then
    ARCH_DIR="arm64"
elif [[ "$(uname -m)" == "arm"* ]]; then
    ARCH_DIR="arm"
fi

# Paths
BUILD_DIR="Build/${ARCH_DIR}/${BUILD_TYPE}"
GAME_EXE="${BUILD_DIR}/Game"
ASSETS_SRC="Game/assets"
CONFIG_SRC="Game/config"
ASSETS_DST="${BUILD_DIR}/assets"
CONFIG_DST="${BUILD_DIR}/config"

# Check if executable exists
if [[ ! -f "$GAME_EXE" ]]; then
    echo "Error: Game executable not found at $GAME_EXE"
    echo "Please build the project first using build.sh"
    exit 1
fi

echo "Cooking media and scene..."
"$GAME_EXE" --cook
"$GAME_EXE" --cook-scene

echo "Staging assets and config..."
mkdir -p "$ASSETS_DST" "$CONFIG_DST"
cp -r "$ASSETS_SRC"/* "$ASSETS_DST/" 2>/dev/null || true
cp -r "$CONFIG_SRC"/* "$CONFIG_DST/" 2>/dev/null || true

echo "Cook process completed successfully!"