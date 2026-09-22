@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
 echo Falta CMake. Instala CMake y Visual Studio con Desarrollo para escritorio con C++.
 pause
 exit /b 1
)
cmake -S . -B build
if errorlevel 1 goto error
cmake --build build --config Release
if errorlevel 1 goto error
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto error
if exist "build\Release\Automatas.exe" (
 "build\Release\Automatas.exe"
) else (
 "build\Automatas.exe"
)
pause
exit /b 0
:error
 echo No se pudo compilar o verificar el proyecto. Revisa el mensaje anterior.
 pause
 exit /b 1
