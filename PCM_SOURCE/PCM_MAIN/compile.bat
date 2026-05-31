@echo off
:: Set up the environment for x64 build with VS2022
call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

:: Kill all main.exe processes before compiling
taskkill /f /im main.exe >nul 2>&1

:: List all files in current directory
echo.
echo Files in current directory:
dir /b

:: Delete all .log files
echo.
echo Deleting .log files...
del *.log >nul 2>&1
if %errorlevel% equ 0 (
    echo .log files deleted successfully
) else (
    echo No .log files found to delete
)

:: Your compile command
cl.exe main.cpp app.res /std:c++17 /EHsc /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\winrt" user32.lib dwmapi.lib d2d1.lib dwrite.lib d3d11.lib dxgi.lib d3dcompiler.lib dcomp.lib windowsapp.lib gdi32.lib shell32.lib

:: Wait for key press so you can see errors
echo.
echo Press any key to exit...
pause >nul