@echo off
NET FILE 1>NUL 2>NUL
if %errorlevel% NEQ 0 (
	echo Installation need be run as Administrator to install a Task
	pause
	exit /B 0
)

cd /D "%~dp0" 
choice /C YN /M "Do you want to install Service based on directory %~dp0? It can not be changed after installation."
if %ERRORLEVEL% NEQ 1 exit /B 1

for %%f in (RyzenAdjServiceTask.xml.template readjustService.ps1 libryzenadj.dll WinRing0x64.dll WinRing0x64.sys inpoutx64.dll) do (
   if not exist %%f echo %%f is missing && goto failed
)

echo Please configure RyzenAdjService by adding your prefered values in the top section of the powershell script. 
timeout /t 2 > NUL
notepad "%~dp0\readjustService.ps1"

powershell -Command "(gc '%~dp0RyzenAdjServiceTask.xml.template') -replace '###SCRIPTPATH###', '%~dp0readjustService.ps1' | Out-File -encoding ASCII '%~dp0RyzenAdjServiceTask.xml'"

SCHTASKS /Create /TN "AMD\RyzenAdj" /XML "%~dp0RyzenAdjServiceTask.xml" /F || goto failed

SCHTASKS /run /TN "AMD\RyzenAdj" || goto failed

timeout /t 2 > NUL

SCHTASKS /query /TN "AMD\RyzenAdj" || goto failed

echo.
echo Installation successfull
pause
exit /B 0

:FAILED
echo Installation failed
pause
exit /B 1
