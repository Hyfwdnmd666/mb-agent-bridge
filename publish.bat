@echo off
REM publish.bat - double-click this after creating the empty repo on GitHub.
REM It asks for your GitHub username, then hands everything to publish.ps1.
REM Git Credential Manager opens a browser for sign-in, so no token is needed.

setlocal
cd /d "%~dp0"

echo.
echo ============================================================
echo   Push mb-agent-bridge to GitHub
echo ============================================================
echo.
echo   Before this works, create an EMPTY repository:
echo.
echo     1. open  https://github.com/new
echo     2. Repository name:  mb-agent-bridge
echo     3. leave every checkbox UNTICKED
echo        (no README, no .gitignore, no license)
echo     4. click "Create repository"
echo.
echo   Then come back here.
echo.
set /p GHUSER=Your GitHub username: 

if "%GHUSER%"=="" (
  echo.
  echo No username given - nothing to do.
  pause
  exit /b 1
)

echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0publish.ps1" -User "%GHUSER%"
echo.
pause
