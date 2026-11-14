@echo off
rem Close the Unreal Editor before running this.
rem Deletes the compiled module so the editor rebuilds the latest C++ code on open.
if exist "%~dp0Binaries" rmdir /s /q "%~dp0Binaries"
if exist "%~dp0Binaries" (
  echo Could not delete the Binaries folder. Close the Unreal Editor and run this again.
  pause
  exit /b 1
)
start "" "%~dp0Isekai.uproject"
