@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
taskkill /f /im launcher.exe >nul 2>&1
cl.exe launcher.cpp app.res /std:c++17 /EHsc /MT /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\winrt" /link user32.lib dwmapi.lib d2d1.lib gdi32.lib ole32.lib windowscodecs.lib dwrite.lib shell32.lib advapi32.lib winhttp.lib Comdlg32.lib /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup
