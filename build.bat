@echo off

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cls

set COMPILER_FLAGS="/DDEBUG_MODE=1"
set LINKWE_FALAGS=""

if not exist build mkdir build

pushd build
    cl /nologo %COMPILER_FLAGS% -Z7 ..\src\main_win32.cpp /link /INCREMENTAL:NO user32.lib Gdi32.lib
popd

