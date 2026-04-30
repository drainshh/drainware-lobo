@echo off
setlocal
set "MSBUILD_EXE="

for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD_EXE=%%i"

if not defined MSBUILD_EXE set "MSBUILD_EXE=Y:\visual\MSBuild\Current\Bin\MSBuild.exe"

"%MSBUILD_EXE%" csgo-sdk.sln /p:Configuration=Debug /p:Platform=x86
pause 
