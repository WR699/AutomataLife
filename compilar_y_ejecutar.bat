@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
 echo Falta CMake. Instala CMake y Visual Studio con Desarrollo para escritorio con C++.
 pause
 exit /b 1
)
rem Directorio nuevo para evitar los proyectos anteriores con nombres en conflicto.
set "AUTOMATAS_BUILD_DIR=build_windows_v3"
cmake -S . -B "%AUTOMATAS_BUILD_DIR%"
if errorlevel 1 goto error
cmake --build "%AUTOMATAS_BUILD_DIR%" --config Release
if errorlevel 1 goto error
ctest --test-dir "%AUTOMATAS_BUILD_DIR%" -C Release --output-on-failure
if errorlevel 1 goto error
if exist "%AUTOMATAS_BUILD_DIR%\Release\Automatas.exe" (
 "%AUTOMATAS_BUILD_DIR%\Release\Automatas.exe"
) else (
 "%AUTOMATAS_BUILD_DIR%\Automatas.exe"
)
pause
exit /b 0
:error
 echo No se pudo compilar o verificar el proyecto. Revisa el mensaje anterior.
 pause
 exit /b 1
