@echo off
setlocal enabledelayedexpansion

set BUILD_TYPE=Debug

if not "%~1"=="" (
    set "ARG=%~1"
    if /I "!ARG!"=="release" set BUILD_TYPE=Release
    if /I "!ARG!"=="debug" set BUILD_TYPE=Debug
)

echo Selected Build Type: !BUILD_TYPE!

cmake --build Build --config !BUILD_TYPE!