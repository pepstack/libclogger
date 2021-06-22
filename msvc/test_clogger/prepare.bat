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


set x64AppDistDbgDir="%~dp0..\..\dist-apps\Debug\win64\%APPPROJECT%-%APPVER%\bin"
set x64AppDistRelsDir="%~dp0..\..\dist-apps\Release\win64\%APPPROJECT%-%APPVER%\bin"
set x86AppDistDbgDir="%~dp0..\..\dist-apps\Debug\win86\%APPPROJECT%-%APPVER%\bin"
set x86AppDistRelsDir="%~dp0..\..\dist-apps\Release\win86\%APPPROJECT%-%APPVER%\bin"

IF not exist %x64AppDistDbgDir% (mkdir %x64AppDistDbgDir%)
IF not exist %x64AppDistRelsDir% (mkdir %x64AppDistRelsDir%)
IF not exist %x86AppDistDbgDir% (mkdir %x86AppDistDbgDir%)
IF not exist %x86AppDistRelsDir% (mkdir %x86AppDistRelsDir%)