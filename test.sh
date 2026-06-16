#!/bin/bash

set -e

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

cmake -B Build -DCMAKE_BUILD_TYPE=${BUILD_TYPE^^}
cmake --build Build --target UnitTests --config $BUILD_TYPE
ctest --test-dir Build -C $BUILD_TYPE --output-on-failure --timeout 30
