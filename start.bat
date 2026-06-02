@echo off
setlocal enabledelayedexpansion

set BUILD_TYPE=Debug
set BUILD_ARCH=x64

if not "%~1"=="" (
  set ARG1=%~1

  if /I "!ARG1!"=="Release" (
    set BUILD_TYPE=Release
  ) else if /I "!ARG1!"=="Debug" (
    set BUILD_TYPE=Debug
  ) else if /I "!ARG1!"=="x64" (
    set BUILD_ARCH=x64
  ) else if /I "!ARG1!"=="x86" (
    set BUILD_ARCH=x86
  ) else (
    echo Invalid Parameter: !ARG1!
    echo Valid Parameters: Debug, Release, x64, x86
    echo Defaulting to Debug and x64
  )
)

if not "%~2"=="" (
  set ARG2=%~2

  if /I "!ARG2!"=="Release" (
    set BUILD_TYPE=Release
  ) else if /I "!ARG2!"=="Debug" (
    set BUILD_TYPE=Debug
  ) else if /I "!ARG2!"=="x64" (
    set BUILD_ARCH=x64
  ) else if /I "!ARG2!"=="x86" (
    set BUILD_ARCH=x86
  ) else (
    echo Invalid Parameter: !ARG2!
    echo Valid Parameters: Debug, Release, x64, x86
    echo Defaulting to Debug and x64
  )
)

echo Selected Build Type: %BUILD_TYPE%
echo Selected Arhitecture: %BUILD_ARCH%

if %BUILD_TYPE%==Release (
  if %BUILD_ARCH%==x64 (
    start ./Build/x64/Release/Game.exe
  ) else (
    start ./Build/x86/Release/Game.exe
  )
) else (
  if %BUILD_ARCH%==x64 (
    start ./Build/x64/Debug/Game.exe
  ) else (
    start ./Build/x86/Debug/Game.exe
  )
)