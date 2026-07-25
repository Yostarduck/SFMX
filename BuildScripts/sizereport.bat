@echo off
REM sizereport.bat - Windows entry point for the exe size breakdown.
REM Delegates to sizereport.ps1 (the hex parsing / sorting / aggregation is not
REM reasonable in pure batch). Args match the .sh: [Debug|Release] [x64|x86].
REM   BuildScripts\sizereport.bat Release x64
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0sizereport.ps1" %*
exit /b %errorlevel%
