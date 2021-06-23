set APPPROJECT=test_clogger

::output %APPPROJECT%

setlocal enabledelayedexpansion
for /f %%a in (%~dp0..\..\src\apps\test_clogger\VERSION) do (
    echo VERSION: %%a
    set APPVER=%%a
    goto :APPVER
)
:APPVER
echo prepare for: %APPPROJECT%-%APPVER%

set DISTAPPDIR="%~dp0..\..\dist\%APPPROJECT%-%APPVER%"

set x64AppDistDbgDir="%DISTAPPDIR%\win64-debug\bin"
set x64AppDistRelsDir="%DISTAPPDIR%\win64-release\bin"
set x86AppDistDbgDir="%DISTAPPDIR%\win86-debug\bin"
set x86AppDistRelsDir="%DISTAPPDIR%\win86-release\bin"

IF not exist %x64AppDistDbgDir% (mkdir %x64AppDistDbgDir%)
IF not exist %x64AppDistRelsDir% (mkdir %x64AppDistRelsDir%)
IF not exist %x86AppDistDbgDir% (mkdir %x86AppDistDbgDir%)
IF not exist %x86AppDistRelsDir% (mkdir %x86AppDistRelsDir%)