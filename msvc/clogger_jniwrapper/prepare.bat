set LIBPROJECT=clogger_jniwrapper

::output %LIBPROJECT%

set JNILIBSDIR="%~dp0..\..\jni\libs"

::x64

set win64BinDbgDir="%JNILIBSDIR%\win64\Debug"
set win64BinRelsDir="%JNILIBSDIR%\win64\Release"

::x86

set win86BinDbgDir="%JNILIBSDIR%\win86\Debug"
set win86BinRelsDir="%JNILIBSDIR%\win86\Release"

::create %JNILIBSDIR% Dirs

IF not exist %win64BinDbgDir% (mkdir %win64BinDbgDir%)
IF not exist %win64BinRelsDir% (mkdir %win64BinRelsDir%)
IF not exist %win86BinDbgDir% (mkdir %win86BinDbgDir%)
IF not exist %win86BinRelsDir% (mkdir %win86BinRelsDir%)