@echo off
setlocal

:: Load MSVC only if cl.exe is not already available
where cl.exe >nul 2>&1
if errorlevel 1 (
    if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    ) else (
        echo Could not find Visual Studio vcvars64.bat
        echo Open the Visual Studio Developer Command Prompt and run this again.
        pause
        exit /b 1
    )
)

:: Kill old client processes before compiling
taskkill /f /im main.exe >nul 2>&1
taskkill /f /im PCM.exe >nul 2>&1

echo.
echo Files in current directory:
dir /b

echo.
echo Deleting .log files...
del *.log >nul 2>&1
if %errorlevel% equ 0 (
    echo .log files deleted successfully
) else (
    echo No .log files found to delete
)

echo.
echo Compiling PCM.exe...

cl.exe ^
main.cpp app.res ^
/Fe:PCM.exe ^
/std:c++17 ^
/EHsc ^
/D_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS ^
/I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" ^
/I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\winrt" ^
user32.lib dwmapi.lib d2d1.lib dwrite.lib d3d11.lib dxgi.lib d3dcompiler.lib dcomp.lib windowsapp.lib gdi32.lib shell32.lib

echo.
if exist PCM.exe (
    echo Build successful: PCM.exe created.
) else (
    echo Build failed. Check the compiler errors above.
)

echo.
echo Press any key to exit...
pause >nul

