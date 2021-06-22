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

::include

set includeDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\clogger"
set commonDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\common"

::x64

set x64binDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win64"

set x64dbgDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win64\Debug"

set x64relsDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win64\Release"


::x86

set x86binDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win86"

set x86dbgDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win86\Debug"

set x86relsDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win86\Release"


::create %LIBPROJECT%-%LIBVER% Dirs

IF not exist %includeDir% (mkdir %includeDir%)

IF not exist %commonDir% (mkdir %commonDir%)

IF not exist %x64binDir% (mkdir %x64binDir%)

IF not exist %x64dbgDir% (mkdir %x64dbgDir%)

IF not exist %x64relsDir% (mkdir %x64relsDir%)

IF not exist %x86binDir% (mkdir %x86binDir%)

IF not exist %x86dbgDir% (mkdir %x86dbgDir%)

IF not exist %x86relsDir% (mkdir %x86relsDir%)