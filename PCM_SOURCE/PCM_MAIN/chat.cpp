
// FOR NULLUT, hardcoded positions cause apparently roblox chat dosent scale?? only chat textbox horizontally scales under like 800x900 idk its weird check it out for urself

#ifndef CHAT_DETECTOR_H
#define CHAT_DETECTOR_H

#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <algorithm>
#include <d3dcompiler.h>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <wrl.h>
#include <d2d1.h>
#include <dwmapi.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwmapi.lib")

using namespace Microsoft::WRL;

namespace WinRTCapture {
    bool InitializeCapture(ID3D11Device* device, ID3D11DeviceContext* context);
    bool IsCaptureInitialized();
    uint32_t GetPixelColor(int x, int y);
    bool GetCurrentFrameTexture(ID3D11Texture2D** outTexture);
    int GetCaptureWidth();
    int GetCaptureHeight();
    HWND FindRobloxWindow();
}

namespace ChatDetector {

struct ChatDetectorApp
{
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;
};

static ChatDetectorApp g_app = {};
static std::thread g_detectorThread;
static std::atomic<bool> g_detectorRunning{ false };
static std::atomic<bool> g_chatActive{ false };
static std::atomic<bool> g_sendIconActive{ false };
static std::chrono::steady_clock::time_point g_lastActiveTime;

// Debug overlay variables
static HWND g_debugHWnd = nullptr;
static ID2D1Factory* g_d2dFactory = nullptr;
static ID2D1DCRenderTarget* g_dcRenderTarget = nullptr;
static ID2D1SolidColorBrush* g_redBrush = nullptr;
static HDC g_memDC = nullptr;
static HBITMAP g_hBitmap = nullptr;
static std::thread g_debugThread;
static std::atomic<bool> g_debugRunning{ false };
static int g_memWidth = 0;
static int g_memHeight = 0;
static bool g_debugClassRegistered = false;

// Base coordinates for 1920x1080 (fullscreen)
static const int BASE_CHAT_ICON_X = 130;
static const int BASE_CHAT_ICON_Y = 35;
static const int BASE_BLINKER_X = 34;
static const int BASE_BLINKER_Y = 90;
static const int BASE_ACTIVE_ICON_X = 456;
static const int BASE_ACTIVE_ICON_Y = 91;

// Dynamic active icon tracking
static int g_activeIconX = BASE_ACTIVE_ICON_X;
static int g_activeIconY = BASE_ACTIVE_ICON_Y;
static const int DEFAULT_ACTIVE_ICON_X = BASE_ACTIVE_ICON_X;
static const int DEFAULT_ACTIVE_ICON_Y = BASE_ACTIVE_ICON_Y;

// Target colors
static const uint32_t CHAT_ICON_COLOR = 0xF7F7F8;
static const uint32_t BLINKER_COLOR = 0xB2B2B2;
static const uint32_t ACTIVE_ICON_COLOR = 0xFFFFFF;

// Debounce time in milliseconds
static const int DEBOUNCE_TIME_MS = 440;

bool InitializeDirectX()
{
    if (g_app.device && g_app.deviceContext) {
        return true;
    }

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &g_app.device,
        nullptr,
        &g_app.deviceContext
    );

    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool InitializeRobloxCapture()
{
    if (!g_app.device) {
        return false;
    }
    return WinRTCapture::InitializeCapture(g_app.device, g_app.deviceContext);
}

bool IsColorMatch(uint32_t color, uint32_t target) {
    // Extract RGB components
    uint8_t r1 = (color >> 16) & 0xFF;
    uint8_t g1 = (color >> 8) & 0xFF;
    uint8_t b1 = color & 0xFF;

    uint8_t r2 = (target >> 16) & 0xFF;
    uint8_t g2 = (target >> 8) & 0xFF;
    uint8_t b2 = target & 0xFF;

    // Exact match
    return (r1 == r2 && g1 == g2 && b1 == b2);
}

void DetectChatActive()
{
    if (!WinRTCapture::IsCaptureInitialized()) {
        return;
    }

    int capWidth = WinRTCapture::GetCaptureWidth();
    int capHeight = WinRTCapture::GetCaptureHeight();
    if (capWidth <= 0 || capHeight <= 0) {
        return;
    }

    // Chat UI doesn't scale - use fixed coordinates
    int chatIconX = BASE_CHAT_ICON_X;
    int chatIconY = BASE_CHAT_ICON_Y;

    int blinkerX = BASE_BLINKER_X;
    int blinkerY = BASE_BLINKER_Y;

    int activeIconX = BASE_ACTIVE_ICON_X;
    int activeIconY = BASE_ACTIVE_ICON_Y;

    // Bounds checking
    if (chatIconX < 0 || chatIconX >= capWidth || chatIconY < 0 || chatIconY >= capHeight) {
        return;
    }
    if (blinkerX < 0 || blinkerX >= capWidth || blinkerY < 0 || blinkerY >= capHeight) {
        return;
    }
    if (activeIconX < 0 || activeIconX >= capWidth || activeIconY < 0 || activeIconY >= capHeight) {
        return;
    }

    // Get pixel colors
    uint32_t chatColor = WinRTCapture::GetPixelColor(chatIconX, chatIconY);
    uint32_t blinkerColor = WinRTCapture::GetPixelColor(blinkerX, blinkerY);
    uint32_t activeColor = WinRTCapture::GetPixelColor(activeIconX, activeIconY);

    // Check if conditions are met
    // Chat icon must be 0xF7F7F8 AND (blinker is 0xB2B2B2 OR active icon is 0xFFFFFF)
    bool conditionsMet = false;
    bool blinkerActive = false;
    bool activeIconActive = false;

    if (IsColorMatch(chatColor, CHAT_ICON_COLOR)) {
        if (IsColorMatch(blinkerColor, BLINKER_COLOR)) {
            blinkerActive = true;
            conditionsMet = true;
        }
        if (IsColorMatch(activeColor, ACTIVE_ICON_COLOR)) {
            activeIconActive = true;
            conditionsMet = true;
        }
    }

    // If active icon is not detected but blinker isn't active either, try to track the icon
    if (!conditionsMet && g_chatActive.load()) {
        // Active icon not found at current position, try to find it
        if (!blinkerActive) {
            bool found = false;

            // Try each pixel down from 1 to 8
            for (int offset = 1; offset <= 8; ++offset) {
                int checkY = activeIconY + offset;
                int checkX = activeIconX;

                if (checkY >= 0 && checkY < capHeight && checkX >= 0 && checkX < capWidth) {
                    uint32_t colorDown = WinRTCapture::GetPixelColor(checkX, checkY);
                    if (IsColorMatch(colorDown, ACTIVE_ICON_COLOR) && IsColorMatch(chatColor, CHAT_ICON_COLOR)) {
                        // Y coordinates don't scale, so offset is already in base space
                        g_activeIconY += offset;
                        conditionsMet = true;
                        activeIconActive = true;
                        found = true;
                        break;
                    }
                }
            }

            // If not found down, try each pixel up from 1 to 8
            if (!found) {
                for (int offset = 1; offset <= 8; ++offset) {
                    int checkY = activeIconY - offset;
                    int checkX = activeIconX;

                    if (checkY >= 0 && checkY < capHeight && checkX >= 0 && checkX < capWidth) {
                        uint32_t colorUp = WinRTCapture::GetPixelColor(checkX, checkY);
                        if (IsColorMatch(colorUp, ACTIVE_ICON_COLOR) && IsColorMatch(chatColor, CHAT_ICON_COLOR)) {
                            // Y coordinates don't scale, so offset is already in base space
                            g_activeIconY -= offset;
                            conditionsMet = true;
                            activeIconActive = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Debounce logic - wait 440ms before deactivating
    auto now = std::chrono::steady_clock::now();

    if (conditionsMet) {
        // Conditions are met, set active and update last active time
        g_chatActive.store(true);
        g_sendIconActive.store(activeIconActive);
        g_lastActiveTime = now;
    } else {
        // Conditions not met, check if we should wait before deactivating
        if (g_chatActive.load()) {
            // Currently active, check if 440ms has passed since last active
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastActiveTime);
            if (elapsed.count() >= DEBOUNCE_TIME_MS) {
                g_chatActive.store(false);
                g_sendIconActive.store(false);
                // Reset active icon position to default when chat becomes inactive
                g_activeIconX = DEFAULT_ACTIVE_ICON_X;
                g_activeIconY = DEFAULT_ACTIVE_ICON_Y;
            }
        }
    }
}

void StartChatDetection()
{
    if (g_detectorThread.joinable()) {
        g_detectorRunning.store(false);
        g_detectorThread.join();
    }

    if (!InitializeDirectX()) {
        return;
    }

    g_detectorRunning.store(true);
    g_chatActive.store(false);
    g_sendIconActive.store(false);
    g_activeIconX = DEFAULT_ACTIVE_ICON_X;
    g_activeIconY = DEFAULT_ACTIVE_ICON_Y;
    g_lastActiveTime = std::chrono::steady_clock::now();

    g_detectorThread = std::thread([]() {
        while (g_detectorRunning.load()) {
            HWND robloxWindow = WinRTCapture::FindRobloxWindow();
            if (robloxWindow) {
                if (!WinRTCapture::IsCaptureInitialized()) {
                    if (!InitializeRobloxCapture()) {
                        Sleep(1000);
                        continue;
                    }
                }

                DetectChatActive();
            } else {
                g_chatActive.store(false);
                g_sendIconActive.store(false);
                Sleep(1000);
            }

            Sleep(16); // Update every 16ms (60fps)
        }
    });
}

void StopChatDetection()
{
    g_detectorRunning.store(false);
    if (g_detectorThread.joinable()) {
        g_detectorThread.join();
    }

    g_chatActive.store(false);
    g_sendIconActive.store(false);

    if (g_app.deviceContext) {
        g_app.deviceContext->Release();
        g_app.deviceContext = nullptr;
    }

    if (g_app.device) {
        g_app.device->Release();
        g_app.device = nullptr;
    }
}

bool IsChatActive()
{
    return g_chatActive.load();
}

bool IsSendIconActive()
{
    // Return true only if chat is active AND the send/active icon was detected
    // Return false if chat is active but only blinker was detected
    if (!g_chatActive.load()) {
        return false;
    }
    return g_sendIconActive.load();
}

bool IsChatDetectionRunning()
{
    return g_detectorRunning.load();
}

// Debug overlay functions

LRESULT CALLBACK DebugWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        ValidateRect(hwnd, nullptr);
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void RenderDebugOverlay(int chatIconX, int chatIconY, int blinkerX, int blinkerY, int activeIconX, int activeIconY) {
    if (!g_debugHWnd || !IsWindow(g_debugHWnd)) return;

    RECT clientRect;
    GetClientRect(g_debugHWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    if (width != g_memWidth || height != g_memHeight) {
        if (g_hBitmap) { DeleteObject(g_hBitmap); g_hBitmap = nullptr; }
        if (g_memDC) { DeleteDC(g_memDC); g_memDC = nullptr; }
        g_memDC = CreateCompatibleDC(nullptr);
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        g_hBitmap = CreateDIBSection(g_memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        SelectObject(g_memDC, g_hBitmap);
        g_memWidth = width;
        g_memHeight = height;
    }

    g_dcRenderTarget->BindDC(g_memDC, &clientRect);
    g_dcRenderTarget->BeginDraw();
    g_dcRenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));

    // Draw red rectangles at each detection point (5x5 pixel boxes)
    float boxSize = 5.0f;

    // Chat icon
    D2D1_RECT_F chatRect = D2D1::RectF(
        (float)chatIconX - boxSize/2, (float)chatIconY - boxSize/2,
        (float)chatIconX + boxSize/2, (float)chatIconY + boxSize/2
    );
    g_dcRenderTarget->FillRectangle(chatRect, g_redBrush);

    // Blinker
    D2D1_RECT_F blinkerRect = D2D1::RectF(
        (float)blinkerX - boxSize/2, (float)blinkerY - boxSize/2,
        (float)blinkerX + boxSize/2, (float)blinkerY + boxSize/2
    );
    g_dcRenderTarget->FillRectangle(blinkerRect, g_redBrush);

    // Active icon
    D2D1_RECT_F activeRect = D2D1::RectF(
        (float)activeIconX - boxSize/2, (float)activeIconY - boxSize/2,
        (float)activeIconX + boxSize/2, (float)activeIconY + boxSize/2
    );
    g_dcRenderTarget->FillRectangle(activeRect, g_redBrush);

    g_dcRenderTarget->EndDraw();

    HDC hdcScreen = GetDC(nullptr);
    POINT ptSrc = { 0, 0 };
    RECT wr;
    GetWindowRect(g_debugHWnd, &wr);
    POINT ptDst = { wr.left, wr.top };
    SIZE size = { width, height };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(g_debugHWnd, hdcScreen, &ptDst, &size, g_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, hdcScreen);
}

void DebugOverlayThread() {
    while (g_debugRunning.load()) {
        HWND robloxWindow = WinRTCapture::FindRobloxWindow();

        if (g_debugHWnd && robloxWindow && IsWindow(g_debugHWnd)) {
            RECT robloxClientRect;
            GetClientRect(robloxWindow, &robloxClientRect);
            POINT topLeft = {robloxClientRect.left, robloxClientRect.top};
            ClientToScreen(robloxWindow, &topLeft);

            int capWidth = WinRTCapture::GetCaptureWidth();
            int capHeight = WinRTCapture::GetCaptureHeight();

            if (capWidth > 0 && capHeight > 0) {
                // Chat UI doesn't scale - use fixed coordinates
                int chatIconX = BASE_CHAT_ICON_X;
                int chatIconY = BASE_CHAT_ICON_Y;

                int blinkerX = BASE_BLINKER_X;
                int blinkerY = BASE_BLINKER_Y;

                int activeIconX = BASE_ACTIVE_ICON_X;
                int activeIconY = BASE_ACTIVE_ICON_Y;

                // Convert to screen coordinates
                chatIconX += topLeft.x;
                chatIconY += topLeft.y;
                blinkerX += topLeft.x;
                blinkerY += topLeft.y;
                activeIconX += topLeft.x;
                activeIconY += topLeft.y;

                RenderDebugOverlay(chatIconX, chatIconY, blinkerX, blinkerY, activeIconX, activeIconY);
            }
        }

        Sleep(16);
    }
}

void StartDebugOverlay() {
    if (g_debugRunning.load()) return;

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory);

    if (!g_debugClassRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DebugWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"ChatDebugOverlay";

        if (!RegisterClassExW(&wc)) return;
        g_debugClassRegistered = true;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    g_debugHWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        L"ChatDebugOverlay", L"ChatDebugOverlay",
        WS_POPUP, 0, 0, screenWidth, screenHeight,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );

    if (!g_debugHWnd) return;

    RECT clientRect;
    GetClientRect(g_debugHWnd, &clientRect);
    g_memDC = CreateCompatibleDC(nullptr);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = clientRect.right - clientRect.left;
    bmi.bmiHeader.biHeight = -(clientRect.bottom - clientRect.top);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    g_hBitmap = CreateDIBSection(g_memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    SelectObject(g_memDC, g_hBitmap);

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT
    );
    g_d2dFactory->CreateDCRenderTarget(&props, &g_dcRenderTarget);
    g_dcRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    // Create red brush
    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 1.0f), &g_redBrush);

    ShowWindow(g_debugHWnd, SW_SHOW);
    UpdateWindow(g_debugHWnd);

    g_debugRunning.store(true);
    g_debugThread = std::thread(DebugOverlayThread);
}

void StopDebugOverlay() {
    g_debugRunning.store(false);
    if (g_debugThread.joinable()) {
        g_debugThread.join();
    }

    if (g_debugHWnd) {
        DestroyWindow(g_debugHWnd);
        g_debugHWnd = nullptr;
    }
    if (g_redBrush) { g_redBrush->Release(); g_redBrush = nullptr; }
    if (g_dcRenderTarget) { g_dcRenderTarget->Release(); g_dcRenderTarget = nullptr; }
    if (g_d2dFactory) { g_d2dFactory->Release(); g_d2dFactory = nullptr; }
    if (g_hBitmap) { DeleteObject(g_hBitmap); g_hBitmap = nullptr; }
    if (g_memDC) { DeleteDC(g_memDC); g_memDC = nullptr; }
}

} // namespace ChatDetector

#endif // CHAT_DETECTOR_H
