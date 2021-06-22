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

set win64BinDbgDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win64\Debug"
set win64BinRelsDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win64\Release"

set win64LibDbgDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win64\Debug"
set win64LibRelsDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win64\Release"


::x86

set win86BinDbgDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win86\Debug"
set win86BinRelsDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win86\Release"

set win86LibDbgDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win86\Debug"
set win86LibRelsDir="%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win86\Release"


::create %LIBPROJECT%-%LIBVER% Dirs

IF not exist %includeDir% (mkdir %includeDir%)
IF not exist %commonDir% (mkdir %commonDir%)

IF not exist %win64BinDbgDir% (mkdir %win64BinDbgDir%)
IF not exist %win64BinRelsDir% (mkdir %win64BinRelsDir%)
IF not exist %win64LibDbgDir% (mkdir %win64LibDbgDir%)
IF not exist %win64LibRelsDir% (mkdir %win64LibRelsDir%)

IF not exist %win86BinDbgDir% (mkdir %win86BinDbgDir%)
IF not exist %win86BinRelsDir% (mkdir %win86BinRelsDir%)
IF not exist %win86LibDbgDir% (mkdir %win86LibDbgDir%)
IF not exist %win86LibRelsDir% (mkdir %win86LibRelsDir%)