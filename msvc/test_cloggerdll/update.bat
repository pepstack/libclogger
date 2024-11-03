@ECHO OFF
set LIBPROJECT=libclogger
set APPPROJECT=test_cloggerdll

for /f %%a in (%~dp0..\..\source\clogger\VERSION) do (
    echo VERSION: %%a
    set LIBVER=%%a
    goto :libver
)
:libver
echo depends: %LIBPROJECT%-%LIBVER%


setlocal enabledelayedexpansion
for /f %%a in (%~dp0..\..\source\apps\test_clogger\VERSION) do (
    echo VERSION: %%a
    set APPVER=%%a
    goto :appver
)
:appver
echo update for: %APPPROJECT%-%APPVER%

set DISTLIBDIR="%~dp0..\..\dist\%LIBPROJECT%-%LIBVER%"
set DISTAPPDIR="%~dp0..\..\dist\%APPPROJECT%-%APPVER%"

set x64AppDistDbgDir="%DISTAPPDIR%\win64-debug\bin"
set x64AppDistRelsDir="%DISTAPPDIR%\win64-release\bin"
set x86AppDistDbgDir="%DISTAPPDIR%\win86-debug\bin"
set x86AppDistRelsDir="%DISTAPPDIR%\win86-release\bin"


if "%1" == "x64_debug" (
	copy "%~dp0clogger.cfg" "%~dp0target\x64\Debug\"
	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x64\pthreadVC2.dll" "%~dp0target\x64\Debug\"
	copy "%DISTLIBDIR%\bin\win64\Debug\%LIBPROJECT%_dll.dll" "%~dp0target\x64\Debug\"

	copy "%~dp0target\x64\Debug\clogger.cfg" "%x64AppDistDbgDir%\"
	copy "%~dp0target\x64\Debug\pthreadVC2.dll" "%x64AppDistDbgDir%\"
	copy "%~dp0target\x64\Debug\%LIBPROJECT%_dll.dll" "%x64AppDistDbgDir%\"
    copy "%~dp0target\x64\Debug\%APPPROJECT%.exe" "%x64AppDistDbgDir%\"
)


if "%1" == "x64_release" (
	copy "%~dp0clogger.cfg" "%~dp0target\x64\Release\"
	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x64\pthreadVC2.dll" "%~dp0target\x64\Release\"
	copy "%DISTLIBDIR%\bin\win64\Release\%LIBPROJECT%_dll.dll" "%~dp0target\x64\Release\"

	copy "%~dp0target\x64\Release\clogger.cfg" "%x64AppDistRelsDir%\"
	copy "%~dp0target\x64\Release\pthreadVC2.dll" "%x64AppDistRelsDir%\"
	copy "%~dp0target\x64\Release\%LIBPROJECT%_dll.dll" "%x64AppDistRelsDir%\"
    copy "%~dp0target\x64\Release\%APPPROJECT%.exe" "%x64AppDistRelsDir%\"
)


if "%1" == "x86_debug" (
	copy "%~dp0clogger.cfg" "%~dp0target\Win32\Debug\"
	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x86\pthreadVC2.dll" "%~dp0target\Win32\Debug\"
	copy "%DISTLIBDIR%\bin\win86\Debug\%LIBPROJECT%_dll.dll" "%~dp0target\Win32\Debug\"

	copy "%~dp0target\Win32\Debug\clogger.cfg" "%x86AppDistDbgDir%\"
	copy "%~dp0target\Win32\Debug\pthreadVC2.dll" "%x86AppDistDbgDir%\"
	copy "%~dp0target\Win32\Debug\%LIBPROJECT%_dll.dll" "%x86AppDistDbgDir%\"
    copy "%~dp0target\Win32\Debug\%APPPROJECT%.exe" "%x86AppDistDbgDir%\"
)


if "%1" == "x86_release" (
	copy "%~dp0clogger.cfg" "%~dp0target\Win32\Release\"
	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x86\pthreadVC2.dll" "%~dp0target\Win32\Release\"
	copy "%DISTLIBDIR%\bin\win86\Release\%LIBPROJECT%_dll.dll" "%~dp0target\Win32\Release\"

	copy "%~dp0target\Win32\Release\clogger.cfg" "%x86AppDistRelsDir%\"
	copy "%~dp0target\Win32\Release\pthreadVC2.dll" "%x86AppDistRelsDir%\"
	copy "%~dp0target\Win32\Release\%LIBPROJECT%_dll.dll" "%x86AppDistRelsDir%\"
    copy "%~dp0target\Win32\Release\%APPPROJECT%.exe" "%x86AppDistRelsDir%\"
)