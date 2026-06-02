@echo off
setlocal enabledelayedexpansion

set BUILD_TYPE=Debug

if not "%~1"=="" (
    set ARG=%~1

    if /I "!ARG!"=="Release" (
        set BUILD_TYPE=Release
    ) else if /I "!ARG!"=="Debug" (
        set BUILD_TYPE=Debug
    ) else (
        echo Invalid Build Type: !ARG!
        echo Valid Build Types: Debug, Release
        echo Defaulting to Debug
    )
)

echo Selected Build Type: %BUILD_TYPE%

rmdir /s /q Build 2>nul

cmake -B Build -DCMAKE_BUILD_TYPE=%BUILD_TYPE:~0%
cmake --build Build --config %BUILD_TYPE%