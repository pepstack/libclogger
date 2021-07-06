@ECHO OFF

set DEPPROJECT=libclogger_dll

set LIBPROJECT=clogger_jniwrapper

set JNILIBSDIR="%~dp0..\..\jni\libs"

set win64BinDbgDir="%JNILIBSDIR%\win64\Debug"
set win64BinRelsDir="%JNILIBSDIR%\win64\Release"

set win86BinDbgDir="%JNILIBSDIR%\win86\Debug"
set win86BinRelsDir="%JNILIBSDIR%\win86\Release"

echo update for: %JNILIBSDIR%


if "%1" == "x64_debug" (
	copy "%~dp0target\x64\Debug\%LIBPROJECT%.dll" "%win64BinDbgDir%\"
	copy "%~dp0..\%DEPPROJECT%\target\x64\Debug\%DEPPROJECT%.dll" "%win64BinDbgDir%\"
)


if "%1" == "x64_release" (
	copy "%~dp0target\x64\Release\%LIBPROJECT%.dll" "%win64BinRelsDir%\"
	copy "%~dp0..\%DEPPROJECT%\target\x64\Release\%DEPPROJECT%.dll" "%win64BinRelsDir%\"
)


if "%1" == "x86_debug" (
	copy "%~dp0target\Win32\Debug\%LIBPROJECT%.dll" "%win86BinDbgDir%\"
	copy "%~dp0..\%DEPPROJECT%\target\Win32\Debug\%DEPPROJECT%.dll" "%win86BinDbgDir%\"
)


if "%1" == "x86_release" (
	copy "%~dp0target\Win32\Release\%LIBPROJECT%.dll" "%win86BinRelsDir%\"
	copy "%~dp0..\%DEPPROJECT%\target\Win32\Release\%DEPPROJECT%.dll" "%win86BinRelsDir%\"
)