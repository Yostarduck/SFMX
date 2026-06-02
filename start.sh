#!/usr/bin/env bash
set -euo pipefail

# Default values
BUILD_TYPE=Debug
BUILD_ARCH=x64

usage() {
  cat <<EOF
Usage: $0 [Debug|Release] [x64|x86]
If multiple args are provided the script will apply them in order.
EOF
}

if [ "$#" -gt 0 ]; then
  for arg in "$@"; do
    arg_lc=$(echo "$arg" | tr '[:upper:]' '[:lower:]')
    case "$arg_lc" in
      release)
        BUILD_TYPE=Release
        ;;
      debug)
        BUILD_TYPE=Debug
        ;;
      x64)
        BUILD_ARCH=x64
        ;;
      x86)
        BUILD_ARCH=x86
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      *)
        echo "Invalid Parameter: $arg"
        usage
        ;;
    esac
  done
fi

echo "Selected Build Type: ${BUILD_TYPE}"
echo "Selected Architecture: ${BUILD_ARCH}"

if [ "${BUILD_TYPE}" = "Release" ]; then
  if [ "${BUILD_ARCH}" = "x64" ]; then
    EXE=./Build/x64/Release/Game
  else
    EXE=./Build/x86/Release/Game
  fi
else
  if [ "${BUILD_ARCH}" = "x64" ]; then
    EXE=./Build/x64/Debug/Game
  else
    EXE=./Build/x86/Debug/Game
  fi
fi

if [ ! -x "${EXE}" ] && [ ! -f "${EXE}" ]; then
  echo "Executable not found: ${EXE}"
  exit 1
fi

# Attach to the process and show logs in foreground (exec replaces shell)
echo "Starting ${EXE} in foreground (logs will appear here)"
echo ""
exec "${EXE}"
