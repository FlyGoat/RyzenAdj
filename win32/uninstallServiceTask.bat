@echo off
NET FILE 1>NUL 2>NUL
if %errorlevel% NEQ 0 (
	echo Deinstallation need to be run as Administrator to delete your Scheduled Task
	pause
	exit /B 0
)

reg query HKCU\Software\HWiNFO64\Sensors\Custom\RyzenAdj 2>NUL
if %errorlevel% EQU 0 reg delete HKCU\Software\HWiNFO64\Sensors\Custom\RyzenAdj

SCHTASKS /query /TN "AMD\RyzenAdj" 2>NUL

if %errorlevel% NEQ 0 (
	echo RyzenAdj Service Task is not installed
	pause
	exit /B 0
)

SCHTASKS /delete /TN "AMD\RyzenAdj" 2>NUL

pause
exit /B 0
