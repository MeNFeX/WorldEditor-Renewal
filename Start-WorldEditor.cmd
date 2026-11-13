@echo off
setlocal
set "WORLD_EDITOR_DATA=D:\ymir work"
set "WORLD_EDITOR_EXE=%WORLD_EDITOR_DATA%\WorldEditor_WE-Release_Win32.exe"
if not exist "%WORLD_EDITOR_EXE%" (
    echo Compile WE-Release first with tools\build-worldeditor.ps1.
    pause
    exit /b 1
)
if not exist "%WORLD_EDITOR_DATA%\." (
    echo Game data directory does not exist: %WORLD_EDITOR_DATA%
    pause
    exit /b 1
)
start "" /D "%WORLD_EDITOR_DATA%" "%WORLD_EDITOR_EXE%"
