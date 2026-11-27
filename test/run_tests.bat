@echo off
cd /d "%~dp0"
set MAKE=mingw32-make
where %MAKE% >nul 2>&1 || set MAKE=make
if "%1"=="known" (%MAKE% known) else (%MAKE% test)
pause