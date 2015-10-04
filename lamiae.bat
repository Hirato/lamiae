@ECHO OFF

set LAMIAE_BIN=w32

IF EXIST bin\w64\lamiae.exe (
    IF /I "%PROCESSOR_ARCHITECTURE%" == "amd64" (
        set LAMIAE_BIN=w64
    )
    IF /I "%PROCESSOR_ARCHITEW6432%" == "amd64" (
        set LAMIAE_BIN=w64
    )
)

start bin\%LAMIAE_BIN%\lamiae.exe "-u$HOME\My Games\Lamiae" -gstdout.txt %*
