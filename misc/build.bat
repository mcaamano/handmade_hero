REM @echo off

REM foobar
REM call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir z:\handmade_hero\build
pushd z:\handmade_hero\build
cl /Zi z:\handmade_hero\code\win32_handmade.c user32.lib
popd
