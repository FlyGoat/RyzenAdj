@echo off
NET FILE 1>NUL 2>NUL
if %errorlevel% NEQ 0 (
	echo Deinstallation need to be run as Administrator to delete your Scheduled Task
	pause
	exit /B 0
)

SCHTASKS /query /TN "AMD\RyzenAdj" 2>NUL

if %errorlevel% NEQ 0 (
	echo RyzenAdj Service Task is not installed
	pause
	exit /B 0
)

:delete
SCHTASKS /delete /TN "AMD\RyzenAdj" 2>NUL

if %errorlevel% NEQ 0 goto delete

pause
exit /B 0
