@echo off

REM
REM Assumes shell.bat is called and this is already executed in the shell environment
REM call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
REM

cls

mkdir z:\handmade_hero\build
pushd z:\handmade_hero\build
cl /Zi z:\handmade_hero\code\win32_handmade.c user32.lib Gdi32.lib Dsound.lib
popd
