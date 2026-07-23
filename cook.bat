@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

REM Default build type
set "BUILD_TYPE=Debug"

REM Parse first argument if provided
if not "%~1"=="" (
    set "ARG=%~1"
    set "ARG_LOWER=%ARG:~0,7%"
    if /I "%ARG%"=="Release" (
        set "BUILD_TYPE=Release"
    ) else if /I "%ARG%"=="Debug" (
        set "BUILD_TYPE=Debug"
    ) else (
        echo Invalid Build Type: %ARG%
        echo Valid Build Types: Debug, Release
        echo Defaulting to Debug
    )
)

echo Selected Build Type: %BUILD_TYPE%

REM Determine architecture based on environment
set "ARCH_DIR=x64"
if "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
    set "ARCH_DIR=arm64"
) else if "%PROCESSOR_ARCHITECTURE%"=="ARM" (
    set "ARCH_DIR=arm"
)

REM Paths
set "BUILD_DIR=Build\%ARCH_DIR%\%BUILD_TYPE%"
set "GAME_EXE=%BUILD_DIR%\Game.exe"
set "ASSETS_SRC=Game\assets"
set "CONFIG_SRC=Game\config"
set "ASSETS_DST=%BUILD_DIR%\assets"
set "CONFIG_DST=%BUILD_DIR%\config"

if not exist "%GAME_EXE%" (
    echo Error: Game executable not found at %GAME_EXE%
    echo Please build the project first using build.bat
    exit /b 1
)

echo Cooking media and scene...
"%GAME_EXE%" --cook
"%GAME_EXE%" --cook-scene

echo Staging assets and config...
if not exist "%ASSETS_DST%" mkdir "%ASSETS_DST%"
if not exist "%CONFIG_DST%" mkdir "%CONFIG_DST%"

if exist "%ASSETS_SRC%\*" (
    xcopy "%ASSETS_SRC%\*" "%ASSETS_DST%\" /E /I /Y >nul
)
if exist "%CONFIG_SRC%\*" (
    xcopy "%CONFIG_SRC%\*" "%CONFIG_DST%\" /E /I /Y >nul
)

echo Cook process completed successfully!
