@ECHO OFF

if "%1" == "x64_debug" (
  set Platform=x64
  set Configuration=Debug
)


if "%1" == "x64_release" (
  set Platform=x64
  set Configuration=Release
)


if "%1" == "x86_debug" (
  set Platform=Win32
  set Configuration=Debug
)


if "%1" == "x86_release" (
  set Platform=Win32
  set Configuration=Release
)

set JNIWRAPPERLIB=%~dp0..\clogger_jniwrapper\target\%Platform%\%Configuration%\clogger_jniwrapper.dll
set DEPPROJECTLIB=%~dp0..\libclogger_dll\target\%Platform%\%Configuration%\libclogger_dll.dll

echo "%JNIWRAPPERLIB%"
echo "%DEPPROJECTLIB%"

copy "%~dp0..\..\deps\pthreads-w32\Pre-built.2\dll\x64\pthreadVC2.dll" "%~dp0target\%Platform%\%Configuration%\"

copy "%DEPPROJECTLIB%" "%~dp0target\%Platform%\%Configuration%\"
copy "%JNIWRAPPERLIB%" "%~dp0target\%Platform%\%Configuration%\"