@echo off
setlocal
cd /d "%~dp0"

echo ============================================================
echo  STRESS CONFIGURABLE / 6 hilos
echo ============================================================
echo.
REM CODIGO VIEJO: el stress antes era fijo en 9.999.900.000 comparaciones.
REM echo Este test hace 9.999.900.000 comparaciones de pares.
REM echo Puede tardar MUCHAS horas o dias segun el equipo.
echo El test pedira un maximo de comparaciones y ajustara el reparto automaticamente.
echo.
echo.

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0instalar_y_compilar.ps1" -Stress200k
set "RC=%ERRORLEVEL%"

echo.
if not "%RC%"=="0" echo El stress o la compilacion fallaron.
pause
exit /b %RC%
