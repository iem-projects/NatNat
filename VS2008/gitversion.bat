@echo off

FOR /F "tokens=*" %%i in ('git describe --always') do SET GITREV=%%i

IF (%1)==() GOTO END

echo #define GITREVISION "%GITREV%" > %1

:END