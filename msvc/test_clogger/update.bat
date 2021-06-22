@ECHO OFF
set LIBPROJECT=libclogger
set APPPROJECT=test_clogger

setlocal enabledelayedexpansion
for /f %%a in (%~dp0..\..\src\apps\test_clogger\VERSION) do (
    echo VERSION: %%a
    set APPVER=%%a
    goto :APPVER
)
:APPVER
echo update for: %APPPROJECT%-%APPVER%

set x64AppDistDbgDir="%~dp0..\..\dist-apps\Debug\win64\%APPPROJECT%-%APPVER%\bin"
set x64AppDistRelsDir="%~dp0..\..\dist-apps\Release\win64\%APPPROJECT%-%APPVER%\bin"
set x86AppDistDbgDir="%~dp0..\..\dist-apps\Debug\win86\%APPPROJECT%-%APPVER%\bin"
set x86AppDistRelsDir="%~dp0..\..\dist-apps\Release\win86\%APPPROJECT%-%APPVER%\bin"


if "%1" == "x64_debug" (
	copy "%~dp0clogger.cfg" "%~dp0target\x64\Debug\"
	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x64\pthreadVC2.dll" "%~dp0target\x64\Debug\"

	copy "%~dp0target\x64\Debug\%APPPROJECT%.exe" "%x64AppDistDbgDir%\"
	copy "%~dp0target\x64\Debug\pthreadVC2.dll" "%x64AppDistDbgDir%\"
	copy "%~dp0target\x64\Debug\clogger.cfg" "%x64AppDistDbgDir%\"
)


if "%1" == "x64_release" (
	copy "%~dp0clogger.cfg" "%~dp0target\x64\Release\"
	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x64\pthreadVC2.dll" "%~dp0target\x64\Release\"

	copy "%~dp0target\x64\Release\%APPPROJECT%.exe" "%x64AppDistRelsDir%\"
	copy "%~dp0target\x64\Release\pthreadVC2.dll" "%x64AppDistRelsDir%\"
	copy "%~dp0target\x64\Release\clogger.cfg" "%x64AppDistRelsDir%\"
)


if "%1" == "x86_debug" (
	copy "%~dp0clogger.cfg" "%~dp0target\Win32\Debug\"
	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x86\pthreadVC2.dll" "%~dp0target\Win32\Debug\"

	copy "%~dp0target\Win32\Debug\%APPPROJECT%.exe" "%x86AppDistDbgDir%\"
	copy "%~dp0target\Win32\Debug\pthreadVC2.dll" "%x86AppDistDbgDir%\"
	copy "%~dp0target\Win32\Debug\clogger.cfg" "%x86AppDistDbgDir%\"
)


if "%1" == "x86_release" (
	copy "%~dp0clogger.cfg" "%~dp0target\Win32\Release\"
	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x86\pthreadVC2.dll" "%~dp0target\Win32\Release\"

	copy "%~dp0target\Win32\Release\%APPPROJECT%.exe" "%x86AppDistRelsDir%\"
	copy "%~dp0target\Win32\Release\pthreadVC2.dll" "%x86AppDistRelsDir%\"
	copy "%~dp0target\Win32\Release\clogger.cfg" "%x86AppDistRelsDir%\"
)