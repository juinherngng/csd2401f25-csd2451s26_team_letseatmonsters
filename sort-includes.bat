@echo off
setlocal

rem Canonical entry point for include normalization.
set "SCRIPT_DIR=%~dp0"
set "SORTER_SCRIPT=%SCRIPT_DIR%sort-includes.ps1"

powershell -NoProfile -ExecutionPolicy Bypass -File "%SORTER_SCRIPT%" %*

endlocal
