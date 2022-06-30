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

set DISTLIBDIR="%~dp0..\..\dist\%LIBPROJECT%-%LIBVER%"


if "%1" == "x64_debug" (
	echo update public head files to: %DISTLIBDIR%\include\clogger\
	copy "%~dp0..\..\src\clogger\clogger_api.h" "%DISTLIBDIR%\include\clogger\"
	copy "%~dp0..\..\src\clogger\clogger_def.h" "%DISTLIBDIR%\include\clogger\"
	copy "%~dp0..\..\src\clogger\logger_helper.h" "%DISTLIBDIR%\include\clogger\"

	echo update lib files to: %DISTLIBDIR%\lib\win64\Debug\
	copy "%~dp0target\x64\Debug\%LIBPROJECT%.lib" "%DISTLIBDIR%\lib\win64\Debug\"
	copy "%~dp0target\x64\Debug\%LIBPROJECT%.pdb" "%DISTLIBDIR%\lib\win64\Debug\"

	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x64\pthreadVC2.dll" "%DISTLIBDIR%\bin\win64\Debug\"
)


if "%1" == "x64_release" (
	echo update public head files to: %DISTLIBDIR%\include\clogger\
	copy "%~dp0..\..\src\clogger\clogger_api.h" "%DISTLIBDIR%\include\clogger\"
	copy "%~dp0..\..\src\clogger\clogger_def.h" "%DISTLIBDIR%\include\clogger\"
	copy "%~dp0..\..\src\clogger\logger_helper.h" "%DISTLIBDIR%\include\clogger\"

	echo update lib files to: %DISTLIBDIR%\lib\win64\Release\
	copy "%~dp0target\x64\Release\%LIBPROJECT%.lib" "%DISTLIBDIR%\lib\win64\Release\"

	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x64\pthreadVC2.dll" "%DISTLIBDIR%\bin\win64\Release\"
)


if "%1" == "x86_debug" (
	echo update public head files to: %DISTLIBDIR%\include\clogger\
	copy "%~dp0..\..\src\clogger\clogger_api.h" "%DISTLIBDIR%\include\clogger\"
	copy "%~dp0..\..\src\clogger\clogger_def.h" "%DISTLIBDIR%\include\clogger\"
	copy "%~dp0..\..\src\clogger\logger_helper.h" "%DISTLIBDIR%\include\clogger\"

	echo update lib files to: %DISTLIBDIR%\lib\win86\Debug\
	copy "%~dp0target\Win32\Debug\%LIBPROJECT%.lib" "%DISTLIBDIR%\lib\win86\Debug\"
	copy "%~dp0target\Win32\Debug\%LIBPROJECT%.pdb" "%DISTLIBDIR%\lib\win86\Debug\"

	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x86\pthreadVC2.dll" "%DISTLIBDIR%\bin\win86\Debug\"
)


if "%1" == "x86_release" (
	echo update public head files to: %DISTLIBDIR%\include\clogger\
	copy "%~dp0..\..\src\clogger\clogger_api.h" "%DISTLIBDIR%\include\clogger\"
	copy "%~dp0..\..\src\clogger\clogger_def.h" "%DISTLIBDIR%\include\clogger\"
	copy "%~dp0..\..\src\clogger\logger_helper.h" "%DISTLIBDIR%\include\clogger\"

	echo update lib files to: %DISTLIBDIR%\lib\win86\Release\
	copy "%~dp0target\Win32\Release\%LIBPROJECT%.lib" "%DISTLIBDIR%\lib\win86\Release\"

	copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x86\pthreadVC2.dll" "%DISTLIBDIR%\bin\win86\Release\"
)