set LIBPROJECT=libclogger

::output %LIBPROJECT%

setlocal enabledelayedexpansion
for /f %%a in (%~dp0..\..\src\clogger\VERSION) do (
    echo VERSION: %%a
    set LIBVER=%%a
    goto :libver
)
:libver
echo prepare for: %LIBPROJECT%-%LIBVER%

set DISTLIBDIR="%~dp0..\..\dist\%LIBPROJECT%-%LIBVER%"

::include

set includeDir="%DISTLIBDIR%\include\clogger"
set commonDir="%DISTLIBDIR%\include\common"

::x64

set x64binDir="%DISTLIBDIR%\bin\win64"

set x64dbgDir="%DISTLIBDIR%\lib\win64\Debug"

set x64relsDir="%DISTLIBDIR%\lib\win64\Release"


::x86

set x86binDir="%DISTLIBDIR%\bin\win86"

set x86dbgDir="%DISTLIBDIR%\lib\win86\Debug"

set x86relsDir="%DISTLIBDIR%\lib\win86\Release"


::create %LIBPROJECT%-%LIBVER% Dirs

IF not exist %includeDir% (mkdir %includeDir%)

IF not exist %commonDir% (mkdir %commonDir%)

IF not exist %x64binDir% (mkdir %x64binDir%)

IF not exist %x64dbgDir% (mkdir %x64dbgDir%)

IF not exist %x64relsDir% (mkdir %x64relsDir%)

IF not exist %x86binDir% (mkdir %x86binDir%)

IF not exist %x86dbgDir% (mkdir %x86dbgDir%)

IF not exist %x86relsDir% (mkdir %x86relsDir%)