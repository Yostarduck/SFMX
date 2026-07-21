@echo off
setlocal enabledelayedexpansion

set BUILD_TYPE=Debug
set BUILD_ARCH=x64
set "FORWARD_ARGS="

:parse_args
if "%~1"=="" goto args_done
set "ARG=%~1"
if /I "%ARG%"=="Release" (
  set "BUILD_TYPE=Release"
) else if /I "%ARG%"=="Debug" (
  set "BUILD_TYPE=Debug"
) else if /I "%ARG%"=="x64" (
  set "BUILD_ARCH=x64"
) else if /I "%ARG%"=="x86" (
  set "BUILD_ARCH=x86"
) else (
  if defined FORWARD_ARGS (
    set "FORWARD_ARGS=!FORWARD_ARGS! \"%ARG%\""
  ) else (
    set "FORWARD_ARGS=\"%ARG%\""
  )
)
shift
goto parse_args
:args_done

echo Selected Build Type: %BUILD_TYPE%
echo Selected Arhitecture: %BUILD_ARCH%

if %BUILD_TYPE%==Release (
  if %BUILD_ARCH%==x64 (
    start "" cmd /c ""./Build/x64/Release/Game.exe" %FORWARD_ARGS%"
  ) else (
    start "" cmd /c ""./Build/x86/Release/Game.exe" %FORWARD_ARGS%"
  )
) else (
  if %BUILD_ARCH%==x64 (
    start "" cmd /c ""./Build/x64/Debug/Game.exe" %FORWARD_ARGS%"
  ) else (
    start "" cmd /c ""./Build/x86/Debug/Game.exe" %FORWARD_ARGS%"
  )
)