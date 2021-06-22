@ECHO OFF
set LIBPROJECT=libclogger

setlocal enabledelayedexpansion
for /f %%a in (%~dp0..\..\src\clogger\VERSION) do (
    echo VERSION: %%a
    set LIBVER=%%a
    goto :libver
)
:libver
echo update for: %LIBPROJECT%-%LIBVER%


if "%1" == "x64_debug" (
	copy "%~dp0..\..\src\clogger\logger_api.h" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\clogger\"
	copy "%~dp0..\..\src\clogger\clogger.h" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\clogger\"
	
	copy "%~dp0target\x64\Debug\%LIBPROJECT%_dll.lib" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win64\Debug\"
	copy "%~dp0target\x64\Debug\%LIBPROJECT%_dll.pdb" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win64\Debug\"
	copy "%~dp0target\x64\Debug\%LIBPROJECT%_dll.dll" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win64\Debug\"
)


if "%1" == "x64_release" (
	copy "%~dp0..\..\src\clogger\logger_api.h" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\clogger\"
	copy "%~dp0..\..\src\clogger\clogger.h" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\clogger\"
	
	copy "%~dp0target\x64\Release\%LIBPROJECT%_dll.lib" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win64\Release\"
	copy "%~dp0target\x64\Release\%LIBPROJECT%_dll.dll" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win64\Release\"
)


if "%1" == "x86_debug" (
	copy "%~dp0..\..\src\clogger\logger_api.h" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\clogger\"
	copy "%~dp0..\..\src\clogger\clogger.h" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\clogger\"
	
	copy "%~dp0target\Win32\Debug\%LIBPROJECT%_dll.lib" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win86\Debug\"
	copy "%~dp0target\Win32\Debug\%LIBPROJECT%_dll.pdb" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win86\Debug\"
	copy "%~dp0target\Win32\Debug\%LIBPROJECT%_dll.dll" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win86\Debug\"
)


if "%1" == "x86_release" (
	copy "%~dp0..\..\src\clogger\logger_api.h" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\clogger\"
	copy "%~dp0..\..\src\clogger\clogger.h" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\include\clogger\"
	
	copy "%~dp0target\Win32\Release\%LIBPROJECT%_dll.lib" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\lib\win86\Release\"
	copy "%~dp0target\Win32\Release\%LIBPROJECT%_dll.dll" "%~dp0..\..\%LIBPROJECT%-%LIBVER%\bin\win86\Release\"
)