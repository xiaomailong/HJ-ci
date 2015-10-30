@echo off

rem transfers all files in all subdirectories of
xcopy C:\Users\hejh\Desktop\platform\platsoftware0.1 \\192.168.1.122\root\home\heinic\platformSoft /E /exclude:EXCLUDE.txt /y

if errorlevel 4 goto lowmemory

if errorlevel 2 goto abort

if errorlevel 0 goto exit

:lowmemory
echo Insufficient memory to copy files or

echo invalid drive or command-line syntax.

goto exit

:abort

echo You pressed CTRL+C to end the copy operation.

goto exit

:exit