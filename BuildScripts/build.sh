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

rm -rf Build

cmake -B Build -DCMAKE_BUILD_TYPE=${BUILD_TYPE^^}
cmake --build Build --config $BUILD_TYPE
