@echo off
cd /d "%~dp0"

set "EXE=%~dp0build_windows_auto\Release\Automatas.exe"

if not exist "%EXE%" (
    echo [ERROR] No se encontro Automatas.exe.
    echo Primero ejecuta compilar_y_ejecutar.bat para compilar el proyecto.
    pause
    exit /b 1
)

"%EXE%"
