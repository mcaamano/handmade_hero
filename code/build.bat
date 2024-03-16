REM @echo off

REM foobar
REM call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir ..\build
pushd ..\build
cl /Zi ..\code\win32_handmade.c user32.lib
popd
