#ifndef CROSSHAIR_H
#define CROSSHAIR_H

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

namespace WinRTCapture {
    HWND FindRobloxWindow();
}

namespace Crosshair {

// Forward declarations
void CreateCrosshairWindow();
void CrosshairLoop();
void CleanupOverlayResources();
void UpdateCrosshairImage(const std::wstring& imagePath);
void UpdateCrosshairScale(float scale);
void SetCameraLockTimeWindow(bool active, DWORD startTime);
bool IsCameraLockTimeWindowActive();
void TriggerRestoration();
void StartShiftLockCheck();
void StopShiftLockCheck();
bool GetShiftLockActive();
void SetWindowJustHidden(bool hidden);

// Simple state variables
static std::atomic<bool> g_running{false};
static std::atomic<bool> g_enabled{false};
static std::atomic<bool> g_isOffScreen{false};
static std::atomic<bool> g_shouldRestore{false};
static std::atomic<bool> g_forceCenter{false};
static std::atomic<DWORD> g_forceCenterStartTime{0};
static std::atomic<DWORD> g_robloxThreadId{0};
static RECT g_originalClipRect = {};
static POINT g_originalCursorPos = {};
static RECT g_offScreenRect = {};
static std::thread g_monitorThread;
static std::atomic<bool> g_cameraLockTimeWindowActive{false};
static std::atomic<DWORD> g_cameraLockTimeWindowStart{0};
static std::thread g_shiftLockCheckThread;
static std::atomic<bool> g_shiftLockCheckThreadRunning{false};
static std::atomic<bool> g_shiftLockActive{false};
static std::atomic<DWORD> g_windowHideTime{0};

// Image overlay variables
static HWND g_hWnd = nullptr;
static bool g_classRegistered = false;
static bool g_WindowShown = false;
static bool g_needsRender = false;
static HDC g_memDC = nullptr;
static HBITMAP g_memBitmap = nullptr;
static HBITMAP g_oldBitmap = nullptr;
static int g_lastCrosshairX = -1;
static int g_lastCrosshairY = -1;
static int g_lastCrosshairWidth = -1;
static int g_lastCrosshairHeight = -1;
static int g_lastRobloxWidth = -1;
static int g_lastRobloxHeight = -1;
static float g_lastScale = 1.0f;
static float g_lastCalculatedScale = -1.0f;
static std::wstring g_currentImagePath;
static HBITMAP g_crosshairBitmap = nullptr;
static ULONG_PTR g_gdiplusToken = 0;
static std::wstring g_persistentImagePath;
static Gdiplus::Image* g_crosshairImage = nullptr;
static bool g_isAnimated = false;
static std::atomic<bool> g_animationActive{false};
static std::thread g_animationThread;
static std::atomic<bool> g_animationThreadRunning{false};
static UINT g_frameCount = 0;
static UINT g_currentFrame = 0;
static Gdiplus::PropertyItem* g_frameDelays = nullptr;
static GUID g_frameDimensionID;

// Simple helper functions
void ClipCursorRect(const RECT* rect) {
    ClipCursor(rect);
}

void SetCursorPosition(int x, int y) {
    SetCursorPos(x, y);
}

RECT GetClipCursorRect() {
    RECT rect = {};
    GetClipCursor(&rect);
    return rect;
}

bool IsShiftLock() {
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (!robloxWindow) {
        return false;
    }

    HWND foregroundWindow = GetForegroundWindow();
    if (foregroundWindow != robloxWindow && GetParent(foregroundWindow) != robloxWindow) {
        return false;
    }

    RECT clipRect = GetClipCursorRect();
    RECT clientRect = {};
    GetClientRect(robloxWindow, &clientRect);

    int clipWidth = clipRect.right - clipRect.left;
    int clipHeight = clipRect.bottom - clipRect.top;

    if (clipWidth != 1 || clipHeight != 1) {
        return false;
    }

    RECT windowRect = {};
    GetWindowRect(robloxWindow, &windowRect);

    int centerX = windowRect.left + (clientRect.right - clientRect.left) / 2;
    int centerY = windowRect.top + (clientRect.bottom - clientRect.top) / 2;

    int tolerance = 50;
    bool isNearCenter = (abs(clipRect.left - centerX) <= tolerance &&
                        abs(clipRect.top - centerY) <= tolerance);

    if (isNearCenter) {
        return true;
    }

    if (g_enabled.load() && g_isOffScreen.load()) {
        POINT topLeft = {clientRect.left, clientRect.top};
        ClientToScreen(robloxWindow, &topLeft);
        bool isAtTopLeft = (abs(clipRect.left - topLeft.x) <= tolerance &&
                          abs(clipRect.top - topLeft.y) <= tolerance);
        return isAtTopLeft;
    }

    return false;
}

void CheckShiftLock() {
    g_shiftLockActive.store(IsShiftLock());
}

bool GetShiftLockActive() {
    return g_shiftLockActive.load();
}

void MoveCursorToTopLeft() {
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (!robloxWindow) return;
    
    DWORD robloxThread = g_robloxThreadId.load();
    DWORD myThread = GetCurrentThreadId();
    
    if (robloxThread != 0) {
        RECT robloxClientRect;
        GetClientRect(robloxWindow, &robloxClientRect);
        POINT topLeft = {robloxClientRect.left, robloxClientRect.top};
        ClientToScreen(robloxWindow, &topLeft);
        
        AttachThreadInput(myThread, robloxThread, TRUE);
        SetCursorPosition(topLeft.x, topLeft.y);
        
        RECT offScreenRect = {topLeft.x, topLeft.y, topLeft.x + 1, topLeft.y + 1};
        ClipCursorRect(&offScreenRect);
        AttachThreadInput(myThread, robloxThread, FALSE);
    }
}

void RestoreCursor() {
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (!robloxWindow) return;
    
    DWORD robloxThread = g_robloxThreadId.load();
    DWORD myThread = GetCurrentThreadId();
    
    if (robloxThread != 0) {
        RECT robloxClientRect;
        GetClientRect(robloxWindow, &robloxClientRect);
        POINT topLeft = {robloxClientRect.left, robloxClientRect.top};
        ClientToScreen(robloxWindow, &topLeft);
        
        int centerX = topLeft.x + (robloxClientRect.right - robloxClientRect.left) / 2;
        int centerY = topLeft.y + (robloxClientRect.bottom - robloxClientRect.top) / 2;
        
        AttachThreadInput(myThread, robloxThread, TRUE);
        ClipCursorRect(&g_originalClipRect);
        SetCursorPosition(centerX, centerY);
        AttachThreadInput(myThread, robloxThread, FALSE);
        
        g_forceCenter.store(true);
        g_forceCenterStartTime.store(GetTickCount());
    }
}


LRESULT CALLBACK CrosshairWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (g_memDC && g_memBitmap) {
                BitBlt(hdc, 0, 0, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
                       g_memDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void AnimationThread() {
    while (g_animationThreadRunning.load()) {
        if (g_animationActive.load() && g_crosshairImage && g_isAnimated && g_frameCount > 1) {
            UINT delay = ((UINT*)g_frameDelays->value)[g_currentFrame] * 10;
            
            g_currentFrame = (g_currentFrame + 1) % g_frameCount;
            g_crosshairImage->SelectActiveFrame(&g_frameDimensionID, g_currentFrame);
            g_needsRender = true;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
    }
}

bool LoadCrosshairImage(const std::wstring& filePath) {
    if (g_crosshairBitmap) {
        DeleteObject(g_crosshairBitmap);
        g_crosshairBitmap = nullptr;
    }
    
    if (g_crosshairImage) {
        if (g_frameDelays) {
            free(g_frameDelays);
            g_frameDelays = nullptr;
        }
        delete g_crosshairImage;
        g_crosshairImage = nullptr;
    }
    
    if (filePath.empty() || filePath == L"No file selected") {
        return false;
    }
    
    g_crosshairImage = Gdiplus::Image::FromFile(filePath.c_str());
    if (!g_crosshairImage || g_crosshairImage->GetLastStatus() != Gdiplus::Ok) {
        delete g_crosshairImage;
        g_crosshairImage = nullptr;
        return false;
    }
    
    UINT count = g_crosshairImage->GetFrameDimensionsCount();
    GUID* dimensionIDs = new GUID[count];
    g_crosshairImage->GetFrameDimensionsList(dimensionIDs, count);
    
    g_frameDimensionID = dimensionIDs[0];
    g_frameCount = g_crosshairImage->GetFrameCount(&g_frameDimensionID);
    g_isAnimated = (g_frameCount > 1);
    
    if (g_isAnimated) {
        UINT totalBuffer = g_crosshairImage->GetPropertyItemSize(PropertyTagFrameDelay);
        if (totalBuffer > 0) {
            g_frameDelays = (Gdiplus::PropertyItem*)malloc(totalBuffer);
            g_crosshairImage->GetPropertyItem(PropertyTagFrameDelay, totalBuffer, g_frameDelays);
        }
        g_currentFrame = 0;
        g_crosshairImage->SelectActiveFrame(&g_frameDimensionID, g_currentFrame);
        g_animationActive.store(true);
    } else {
        g_animationActive.store(false);
    }
    
    delete[] dimensionIDs;
    
    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(filePath.c_str());
    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {
        delete bitmap;
        return false;
    }
    
    HBITMAP hBitmap;
    bitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap);
    delete bitmap;
    
    g_crosshairBitmap = hBitmap;
    return g_crosshairBitmap != nullptr;
}

void RenderCrosshair() {
    if (!g_memDC || !g_memBitmap) return;
    
    RECT clientRect;
    GetClientRect(g_hWnd, &clientRect);
    HBRUSH clearBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(g_memDC, &clientRect, clearBrush);
    DeleteObject(clearBrush);
    
    if (!g_isOffScreen.load() || (!g_crosshairBitmap && !g_crosshairImage)) return;
    
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (!robloxWindow) return;
    
    RECT robloxClientRect;
    GetClientRect(robloxWindow, &robloxClientRect);
    POINT topLeft = {robloxClientRect.left, robloxClientRect.top};
    ClientToScreen(robloxWindow, &topLeft);
    
    int robloxW = robloxClientRect.right - robloxClientRect.left;
    int robloxH = robloxClientRect.bottom - robloxClientRect.top;
    
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    float scaleX = (float)robloxW / (float)screenW;
    float scaleY = (float)robloxH / (float)screenH;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    
    int imageWidth, imageHeight;
    
    if (g_crosshairImage) {
        imageWidth = g_crosshairImage->GetWidth();
        imageHeight = g_crosshairImage->GetHeight();
    } else {
        BITMAP bm;
        GetObject(g_crosshairBitmap, sizeof(BITMAP), &bm);
        imageWidth = bm.bmWidth;
        imageHeight = bm.bmHeight;
    }
    
    float userScale = g_lastScale;
    float finalScale = userScale * scale;
    
    int scaledWidth = (int)(imageWidth * finalScale);
    int scaledHeight = (int)(imageHeight * finalScale);
    
    int robloxCenterX = topLeft.x + robloxW / 2;
    int robloxCenterY = topLeft.y + robloxH / 2;
    int crosshairX = robloxCenterX - scaledWidth / 2;
    int crosshairY = robloxCenterY - scaledHeight / 2;
    
    if (g_crosshairImage && g_isAnimated) {
        Gdiplus::Graphics graphics(g_memDC);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.DrawImage(g_crosshairImage, crosshairX, crosshairY, scaledWidth, scaledHeight);
    } else if (g_crosshairBitmap) {
        HDC bitmapDC = CreateCompatibleDC(g_memDC);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(bitmapDC, g_crosshairBitmap);
        
        BLENDFUNCTION blendFunc = {};
        blendFunc.BlendOp = AC_SRC_OVER;
        blendFunc.BlendFlags = 0;
        blendFunc.SourceConstantAlpha = 255;
        blendFunc.AlphaFormat = AC_SRC_ALPHA;
        
        AlphaBlend(g_memDC, crosshairX, crosshairY, scaledWidth, scaledHeight,
                   bitmapDC, 0, 0, imageWidth, imageHeight, blendFunc);
        
        SelectObject(bitmapDC, oldBitmap);
        DeleteDC(bitmapDC);
    }
    
    InvalidateRect(g_hWnd, nullptr, FALSE);
}

void CreateCrosshairWindow() {
    if (!g_classRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = CrosshairWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"CrosshairOverlay";
        
        if (!RegisterClassExW(&wc)) return;
        g_classRegistered = true;
    }
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        L"CrosshairOverlay", L"CrosshairOverlay",
        WS_POPUP, 0, 0, screenWidth, screenHeight,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    if (!g_hWnd) return;
    
    SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 255, LWA_COLORKEY);
    
    RECT clientRect;
    GetClientRect(g_hWnd, &clientRect);
    g_memDC = CreateCompatibleDC(nullptr);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = clientRect.right - clientRect.left;
    bmi.bmiHeader.biHeight = -(clientRect.bottom - clientRect.top);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    g_memBitmap = CreateDIBSection(g_memDC, &bmi, DIB_RGB_COLORS, nullptr, nullptr, 0);
    g_oldBitmap = (HBITMAP)SelectObject(g_memDC, g_memBitmap);
}

void CleanupOverlayResources() {
    g_animationThreadRunning.store(false);
    g_animationActive.store(false);
    
    if (g_animationThread.joinable()) {
        g_animationThread.join();
    }
    
    if (g_crosshairImage) {
        delete g_crosshairImage;
        g_crosshairImage = nullptr;
    }
    
    if (g_frameDelays) {
        free(g_frameDelays);
        g_frameDelays = nullptr;
    }
    
    if (g_crosshairBitmap) {
        DeleteObject(g_crosshairBitmap);
        g_crosshairBitmap = nullptr;
    }
    
    if (g_oldBitmap) {
        SelectObject(g_memDC, g_oldBitmap);
        g_oldBitmap = nullptr;
    }
    
    if (g_memBitmap) {
        DeleteObject(g_memBitmap);
        g_memBitmap = nullptr;
    }
    
    if (g_memDC) {
        DeleteDC(g_memDC);
        g_memDC = nullptr;
    }
    
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = nullptr;
    }
    
    if (g_classRegistered) {
        UnregisterClassW(L"CrosshairOverlay", GetModuleHandle(nullptr));
        g_classRegistered = false;
    }
}

void CrosshairLoop() {
    while (g_running) {
        HWND robloxWindow = WinRTCapture::FindRobloxWindow();
        HWND fg = GetForegroundWindow();
        bool isRobloxActive = (robloxWindow && (fg == robloxWindow || GetParent(fg) == robloxWindow));
        bool shouldShow = isRobloxActive && g_enabled.load() && g_isOffScreen.load();
        
        if (shouldShow != g_WindowShown && g_hWnd && IsWindow(g_hWnd)) {
            if (shouldShow) {
                ShowWindow(g_hWnd, SW_SHOW);
                SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                g_needsRender = true;
            } else {
                SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                ShowWindow(g_hWnd, SW_HIDE);
            }
            g_WindowShown = shouldShow;
        }
        
        if (g_hWnd && robloxWindow && IsWindow(g_hWnd)) {
            RECT robloxClientRect;
            GetClientRect(robloxWindow, &robloxClientRect);
            POINT topLeft = {robloxClientRect.left, robloxClientRect.top};
            ClientToScreen(robloxWindow, &topLeft);
            
            int robloxW = robloxClientRect.right - robloxClientRect.left;
            int robloxH = robloxClientRect.bottom - robloxClientRect.top;
            
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            float scaleX = (float)robloxW / (float)screenW;
            float scaleY = (float)robloxH / (float)screenH;
            float scale = (scaleX < scaleY) ? scaleX : scaleY;
            if (scale < 0.6f) scale = 0.6f;
            
            if (g_lastCrosshairX != 0 || g_lastCrosshairY != 0 || g_lastCrosshairWidth != screenW || g_lastCrosshairHeight != screenH || 
                g_lastRobloxWidth != robloxW || g_lastRobloxHeight != robloxH || g_lastCalculatedScale != scale) {
                g_lastCrosshairX = 0;
                g_lastCrosshairY = 0;
                g_lastCrosshairWidth = screenW;
                g_lastCrosshairHeight = screenH;
                g_lastRobloxWidth = robloxW;
                g_lastRobloxHeight = robloxH;
                g_lastCalculatedScale = scale;
                SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, screenW, screenH, SWP_NOACTIVATE | SWP_NOZORDER);
                g_needsRender = true;
            }
        }
        
        if (g_hWnd && g_WindowShown && IsWindow(g_hWnd) && g_needsRender) {
            RenderCrosshair();
            g_needsRender = false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void MonitorThread() {
    while (g_running.load()) {
        if (g_enabled.load()) {
            HWND robloxWindow = WinRTCapture::FindRobloxWindow();
            HWND foregroundWindow = GetForegroundWindow();
            bool isRobloxForeground = (robloxWindow && (foregroundWindow == robloxWindow || GetParent(foregroundWindow) == robloxWindow));
            
            if (isRobloxForeground) {
                if (g_shouldRestore.load()) {
                    g_forceCenter.store(true);
                    g_forceCenterStartTime.store(GetTickCount());
                    g_isOffScreen.store(false);
                    g_shouldRestore.store(false);
                    g_cameraLockTimeWindowActive.store(false);
                    g_cameraLockTimeWindowStart.store(0);
                } else if (g_isOffScreen.load()) {
                    bool shiftLockActive = IsShiftLock();
                    if (shiftLockActive) {
                        DWORD currentTime = GetTickCount();
                        DWORD windowHideTime = g_windowHideTime.load();
                        bool inGracePeriod = false;
                        
                        if (windowHideTime != 0) {
                            DWORD elapsedSinceHide = currentTime - windowHideTime;
                            if (elapsedSinceHide <= 440) {
                                inGracePeriod = true;
                            } else {
                                g_windowHideTime.store(0);
                            }
                        }
                        
                        bool shouldMoveToTopLeft = false;
                        if (inGracePeriod) {
                            shouldMoveToTopLeft = true;
                        } else {
                            if (IsCameraLockTimeWindowActive()) {
                                shouldMoveToTopLeft = true;
                            }
                        }
                        
                        if (shouldMoveToTopLeft) {
                            MoveCursorToTopLeft();
                        } else {
                            g_isOffScreen.store(false);
                        }
                    } else {
                        g_isOffScreen.store(false);
                    }
                } else {
                    bool shiftLockActive = IsShiftLock();
                    if (shiftLockActive) {
                        DWORD currentTime = GetTickCount();
                        DWORD windowHideTime = g_windowHideTime.load();
                        bool inGracePeriod = false;
                        
                        if (windowHideTime != 0) {
                            DWORD elapsedSinceHide = currentTime - windowHideTime;
                            if (elapsedSinceHide <= 440) {
                                inGracePeriod = true;
                            } else {
                                g_windowHideTime.store(0);
                            }
                        }
                        
                        bool shouldMoveToTopLeft = false;
                        if (inGracePeriod) {
                            shouldMoveToTopLeft = true;
                        } else {
                            if (g_cameraLockTimeWindowActive.load()) {
                                DWORD elapsed = currentTime - g_cameraLockTimeWindowStart.load();
                                if (elapsed <= 440) {
                                    shouldMoveToTopLeft = true;
                                }
                            }
                        }
                        
                        if (shouldMoveToTopLeft) {
                            g_isOffScreen.store(true);
                        }
                    } else {
                        if (g_isOffScreen.load()) {
                            g_isOffScreen.store(false);
                        }
                    }
                }
            }   
            
            if (g_forceCenter.load()) {
                DWORD currentTime = GetTickCount();
                DWORD elapsed = currentTime - g_forceCenterStartTime.load();
                
                if (elapsed < 1440) {
                    DWORD robloxThread = g_robloxThreadId.load();
                    DWORD myThread = GetCurrentThreadId();
                    
                    if (robloxThread != 0) {
                        HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                        if (robloxWindow) {
                            RECT robloxClientRect;
                            GetClientRect(robloxWindow, &robloxClientRect);
                            POINT topLeft = {robloxClientRect.left, robloxClientRect.top};
                            ClientToScreen(robloxWindow, &topLeft);
                            
                            int centerX = topLeft.x + (robloxClientRect.right - robloxClientRect.left) / 2;
                            int centerY = topLeft.y + (robloxClientRect.bottom - robloxClientRect.top) / 2;
                            
                            POINT currentPos;
                            GetCursorPos(&currentPos);
                            
                            int tolerance = 50;
                            bool isAtTopLeft = (abs(currentPos.x - topLeft.x) <= tolerance && 
                                               abs(currentPos.y - topLeft.y) <= tolerance);
                            
                            if (isAtTopLeft) {
                                AttachThreadInput(myThread, robloxThread, TRUE);
                                SetCursorPosition(centerX, centerY);
                                AttachThreadInput(myThread, robloxThread, FALSE);
                            } else {
                                g_forceCenter.store(false);
                            }
                        }
                    }
                } else {
                    g_forceCenter.store(false);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void StartCrosshair() {
    if (g_running.load()) return;
    
    CleanupOverlayResources();
    if (g_gdiplusToken != 0) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
    
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
    
    g_running.store(true);
    g_enabled.store(true);
    g_isOffScreen.store(false);
    g_shouldRestore.store(false);
    g_forceCenter.store(false);
    g_forceCenterStartTime.store(0);
    g_cameraLockTimeWindowActive.store(false);
    g_cameraLockTimeWindowStart.store(0);
    g_WindowShown = false;
    g_needsRender = false;
    g_lastCrosshairX = -1;
    g_lastCrosshairY = -1;
    g_lastCrosshairWidth = -1;
    g_lastCrosshairHeight = -1;
    g_lastRobloxWidth = -1;
    g_lastRobloxHeight = -1;
    g_lastScale = 1.0f;
    g_lastCalculatedScale = -1.0f;
    g_crosshairBitmap = nullptr;
    g_crosshairImage = nullptr;
    g_isAnimated = false;
    g_animationActive.store(false);
    g_animationThreadRunning.store(false);
    g_frameCount = 0;
    g_currentFrame = 0;
    g_frameDelays = nullptr;
    g_currentImagePath.clear();
    
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (robloxWindow) {
        DWORD threadId = GetWindowThreadProcessId(robloxWindow, nullptr);
        g_robloxThreadId.store(threadId);
    }
    
    g_originalClipRect = GetClipCursorRect();
    GetCursorPos(&g_originalCursorPos);
    
    CreateCrosshairWindow();
    g_monitorThread = std::thread(MonitorThread);
    std::thread(CrosshairLoop).detach();
    
    g_animationThreadRunning.store(true);
    g_animationThread = std::thread(AnimationThread);
    
    if (!g_persistentImagePath.empty()) {
        LoadCrosshairImage(g_persistentImagePath);
        g_currentImagePath = g_persistentImagePath;
        g_needsRender = true;
    }
}

void StopCrosshair() {
    if (!g_running.load()) return;
    
    g_running.store(false);
    g_enabled.store(false);
    g_isOffScreen.store(false);
    g_shouldRestore.store(false);
    g_forceCenter.store(false);
    g_forceCenterStartTime.store(0);
    g_cameraLockTimeWindowActive.store(false);
    g_cameraLockTimeWindowStart.store(0);
    g_WindowShown = false;
    g_needsRender = false;
    g_lastCrosshairX = -1;
    g_lastCrosshairY = -1;
    g_lastCrosshairWidth = -1;
    g_lastCrosshairHeight = -1;
    g_lastRobloxWidth = -1;
    g_lastRobloxHeight = -1;
    g_lastScale = 1.0f;
    g_lastCalculatedScale = -1.0f;
    
    g_animationThreadRunning.store(false);
    g_animationActive.store(false);
    
    if (g_crosshairImage) {
        delete g_crosshairImage;
        g_crosshairImage = nullptr;
    }
    
    if (g_frameDelays) {
        free(g_frameDelays);
        g_frameDelays = nullptr;
    }
    
    if (g_crosshairBitmap) {
        DeleteObject(g_crosshairBitmap);
        g_crosshairBitmap = nullptr;
    }
    g_currentImagePath.clear();
    
    ClipCursorRect(&g_originalClipRect);
    
    if (g_monitorThread.joinable()) {
        g_monitorThread.join();
    }
    
    
    CleanupOverlayResources();
    
    if (g_gdiplusToken != 0) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

void SetEnabled(bool enabled) {
    g_enabled.store(enabled);
}

bool IsRunning() {
    return g_running.load();
}

bool IsEnabled() {
    return g_enabled.load();
}

void UpdateCrosshairScale(float scale) {
    g_lastScale = scale;
    g_needsRender = true;
}

void UpdateCrosshairImage(const std::wstring& imagePath) {
    if (imagePath != g_currentImagePath) {
        LoadCrosshairImage(imagePath);
        g_currentImagePath = imagePath;
        g_persistentImagePath = imagePath;
        g_needsRender = true;
    }
}

void SetCameraLockTimeWindow(bool active, DWORD startTime) {
    g_cameraLockTimeWindowActive.store(active);
    g_cameraLockTimeWindowStart.store(startTime);
}

bool IsCameraLockTimeWindowActive() {
    if (!g_cameraLockTimeWindowActive.load()) return false;
    
    DWORD currentTime = GetTickCount();
    DWORD elapsed = currentTime - g_cameraLockTimeWindowStart.load();
    
    if (elapsed >= 1440) {
        g_cameraLockTimeWindowActive.store(false);
        return false;
    }
    
    return true;
}

void TriggerRestoration() {
    if (g_isOffScreen.load()) {
        g_shouldRestore.store(true);
    }
}

void StartShiftLockCheck() {
    if (g_shiftLockCheckThread.joinable()) {
        g_shiftLockCheckThreadRunning = false;
        g_shiftLockCheckThread.join();
    }
    
    g_shiftLockCheckThreadRunning = true;
    g_shiftLockCheckThread = std::thread([]() {
        while (g_shiftLockCheckThreadRunning) {
            HWND robloxWindow = WinRTCapture::FindRobloxWindow();
            if (robloxWindow) {
                CheckShiftLock();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            } else {
                g_shiftLockActive.store(false);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    });
}

void StopShiftLockCheck() {
    g_shiftLockCheckThreadRunning = false;
    if (g_shiftLockCheckThread.joinable()) {
        g_shiftLockCheckThread.join();
    }
    g_shiftLockActive.store(false);
}

void SetWindowJustHidden(bool hidden) {
    if (hidden) {
        g_windowHideTime.store(GetTickCount());
    } else {
        g_windowHideTime.store(0);
    }
}

}

#endif