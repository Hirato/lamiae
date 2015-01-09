@ECHO OFF

set LAMIAE_BIN=binw32

IF EXIST binw64\lamiae.exe (
    IF /I "%PROCESSOR_ARCHITECTURE%" == "amd64" (
        set LAMIAE_BIN=binw64
    )
    IF /I "%PROCESSOR_ARCHITEW6432%" == "amd64" (
        set LAMIAE_BIN=binw64
    )
)

start %LAMIAE_BIN%\lamiae.exe "-u$HOME\My Games\Lamiae" -gstdout.txt %*
