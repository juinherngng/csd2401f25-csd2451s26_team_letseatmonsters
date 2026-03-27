@echo off
setlocal

powershell -ExecutionPolicy Bypass -File "%~dp0sort-includes.ps1" %*

endlocal
