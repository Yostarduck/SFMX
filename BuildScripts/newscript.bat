@echo off
setlocal EnableDelayedExpansion

if "%~1"=="" (
    echo Usage: create_lua.bat ^<lowerCamelCaseName^>
    exit /b 1
)

set "FILE_NAME=%~1"
set "CLASS_NAME=%~1"

rem Uppercase the first character
set "FIRST_CHAR=!CLASS_NAME:~0,1!"
set "REST=!CLASS_NAME:~1!"

for /f "delims=" %%A in ('powershell -NoProfile -Command "[char]::ToUpper('!FIRST_CHAR!')"') do (
    set "FIRST_CHAR=%%A"
)

set "CLASS_NAME=!FIRST_CHAR!!REST!"

set "OUTPUT_DIR=%~dp0..\Game\resources"

rem Create the Lua file
(
    echo -- File: !FILE_NAME!.lua
    echo -- Description:
    echo.
    echo local !CLASS_NAME! = {}
    echo function !CLASS_NAME!.onCreated^(self^)
    echo end
    echo.
    echo function !CLASS_NAME!.onStart^(self^)
    echo end
    echo.
    echo function !CLASS_NAME!.onUpdate^(self, deltaTime^)
    echo end
    echo.
    echo function !CLASS_NAME!.onDestroyed^(self^)
    echo end
    echo.
    echo return !CLASS_NAME!
) > "%OUTPUT_DIR%\!FILE_NAME!.lua"

echo Created !FILE_NAME!.lua

endlocal