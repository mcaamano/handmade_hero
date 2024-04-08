@echo off

REM
REM Assumes shell.bat is called and this is already executed in the shell environment
REM call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
REM

cls
echo ==================
echo HandmadeHero build
echo ==================
echo.

IF NOT EXIST build mkdir build
IF NOT EXIST build mkdir data

pushd build

:: -wdXXXX specifically disable warning XXXX
:: -wd4201 nameless struct/union
:: -wd4100 unreferenced formal parameter / parameter not used in function
:: -wd4189 local variable is initialized but not referenced / variable not used - will clean up later

:: /Zi produces debug information
:: /Z7 user older method for debug information that plays nicer with multiprocessor builds

:: -Oi enable compiler instrinsics
:: -GR- turns off runtime type information C++
:: -EAa- turn off exception handling
:: -nologo turn off compiler logo/version print

:: Enable 32bit builds to run on WinXP
:: change shell.bat from x64 to x86 setting and also pass the 
:: /link -subsystem:windows,5.1 Build an executable that can run on WinXP

:: -MD use runtime c library as a DLL
:: -MT compile in the c runtime library as a static lib linked in

:: -Gmi disables incremental builds

:: -Od do not do any optimizations - for now allows us to look at dissassemly directly matching code

echo Building:
cl -nologo -MT -Gm- -GR- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32 /Zi /Fmwin32_handmade.map z:\handmade_hero\code\win32_handmade.c /link -opt:ref user32.lib Gdi32.lib Dsound.lib || echo "Build Failed" && popd && exit /B

echo.
echo ==================
echo Build Completed OK
echo ==================

dir

popd
