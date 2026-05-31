#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>

namespace ParryBar {
    bool IsGateCheckActive();
}

namespace WinRTCapture {
    HWND FindRobloxWindow();
}

namespace EditHUD {
    bool IsEditorActive();
}

namespace CPS {

static HWND g_hWnd = nullptr;
static ID2D1Factory* g_d2dFactory = nullptr;
static ID2D1DCRenderTarget* g_dcRenderTarget = nullptr;
static IDWriteFactory* g_writeFactory = nullptr;
static IDWriteTextFormat* g_textFormat = nullptr;
static IDWriteTextFormat* g_textFormatLeft = nullptr;
static IDWriteTextFormat* g_textFormatRight = nullptr;
static float g_lastFontSize = -1.0f;
static ID2D1SolidColorBrush* g_boxBrush = nullptr;
static ID2D1SolidColorBrush* g_textBrush = nullptr;
static HDC g_memDC = nullptr;
static HBITMAP g_hBitmap = nullptr;
static std::atomic<bool> g_running{ false };
static std::thread g_thread;
static bool g_WindowShown = false;
static bool g_classRegistered = false;

static std::vector<ULONGLONG> lmbClickTimes;
static std::vector<ULONGLONG> rmbClickTimes;
static std::mutex lmbMutex;
static std::mutex rmbMutex;
static int g_lastCPSX = -1;
static int g_lastCPSY = -1;
static int g_lastCPSWidth = -1;
static int g_lastCPSHeight = -1;
static float g_currentScale = 1.0f;
static std::atomic<float> g_userScale{ 1.0f };
static int g_memWidth = 0;
static int g_memHeight = 0;
static std::atomic<bool> g_needsRender{ false };
static std::atomic<bool> g_hasRequestedPosition{ false };
static std::atomic<int> g_requestedX{ -1 };
static std::atomic<int> g_requestedY{ -1 };
static std::atomic<bool> g_customOffsetEnabled{ false };
static std::atomic<float> g_offsetXUnits{ 0.0f };
static std::atomic<float> g_offsetYUnits{ 0.0f };

static inline int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static inline float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

const int WINDOW_WIDTH = 135;
const int WINDOW_HEIGHT = 40;
const int BOX_WIDTH = 135;
const int BOX_HEIGHT = 25;
const int SPACING = 4;

static inline ULONGLONG nowMs() { return GetTickCount64(); }

static void addLMB() {
    std::lock_guard<std::mutex> lock(lmbMutex);
    lmbClickTimes.push_back(nowMs());
}

static void addRMB() {
    std::lock_guard<std::mutex> lock(rmbMutex);
    rmbClickTimes.push_back(nowMs());
}

void RegisterLeftClick() {
    addLMB();
    g_needsRender = true;
}

void RegisterRightClick() {
    addRMB();
    g_needsRender = true;
}

static int calculateCPS(std::vector<ULONGLONG>& clickTimes, std::mutex& mutex) {
    std::lock_guard<std::mutex> lock(mutex);
    ULONGLONG now = nowMs();
    ULONGLONG oneSecondAgo = now - 1000;
    
    clickTimes.erase(
        std::remove_if(clickTimes.begin(), clickTimes.end(),
            [oneSecondAgo](ULONGLONG time) { return time < oneSecondAgo; }),
        clickTimes.end()
    );
    
    return static_cast<int>(clickTimes.size());
}

void DrawCPSBox(int x, int y, int width, int height, const std::wstring& text) {
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
        D2D1::RectF((float)x, (float)y, (float)(x + width), (float)(y + height)),
        8.0f, 8.0f
    );
    g_dcRenderTarget->FillRoundedRectangle(roundedRect, g_boxBrush);
    D2D1_RECT_F textRect = D2D1::RectF((float)x, (float)y, (float)(x + width), (float)(y + height));
    g_dcRenderTarget->DrawText(text.c_str(), text.length(), g_textFormat, textRect, g_textBrush);
}

static void MeasureText(const std::wstring& text, float& outWidth, float& outHeight) {
    IDWriteTextLayout* layout = nullptr;
    g_writeFactory->CreateTextLayout(text.c_str(), (UINT32)text.length(), g_textFormat, 1000.0f, 1000.0f, &layout);
    if (layout) layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    DWRITE_TEXT_METRICS m = {};
    if (layout) layout->GetMetrics(&m);
    if (layout) layout->Release();
    outWidth = m.width;
    outHeight = m.height;
}

static void DrawCpsTextWithBracketOffset(int x, int y, int width, int height, const std::wstring& innerText, float bracketOffset) {
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
        D2D1::RectF((float)x, (float)y, (float)(x + width), (float)(y + height)),
        8.0f, 8.0f
    );
    g_dcRenderTarget->FillRoundedRectangle(roundedRect, g_boxBrush);

    float innerW = 0.0f, innerH = 0.0f;
    float leftBW = 0.0f, leftBH = 0.0f;
    float rightBW = 0.0f, rightBH = 0.0f;
    MeasureText(innerText, innerW, innerH);
    MeasureText(L"[", leftBW, leftBH);
    MeasureText(L"]", rightBW, rightBH);
    float bracketW = ceilf((leftBW > rightBW ? leftBW : rightBW));
    float innerWPad = ceilf(innerW) + 1.0f;
    float totalW = bracketW + innerWPad + bracketW;
    float startX = floorf((float)x + ((float)width - totalW) * 0.5f + 0.5f);
    float baseY = floorf((float)y + ((float)height - innerH) * 0.5f + 0.5f);
    if (baseY < (float)y) baseY = (float)y;

    D2D1_RECT_F leftRect = D2D1::RectF(floorf(startX), floorf(baseY - bracketOffset), floorf(startX + bracketW), floorf(baseY - bracketOffset + innerH));
    g_dcRenderTarget->DrawText(L"[", 1, g_textFormatLeft ? g_textFormatLeft : g_textFormat, leftRect, g_textBrush);

    D2D1_RECT_F innerRect = D2D1::RectF(floorf(startX + bracketW), floorf(baseY), floorf(startX + bracketW + innerWPad), floorf(baseY + innerH));
    g_dcRenderTarget->DrawText(innerText.c_str(), (UINT32)innerText.length(), g_textFormat, innerRect, g_textBrush);

    D2D1_RECT_F rightRect = D2D1::RectF(floorf(startX + bracketW + innerWPad), floorf(baseY - bracketOffset), floorf(startX + bracketW + innerWPad + bracketW), floorf(baseY - bracketOffset + innerH));
    g_dcRenderTarget->DrawText(L"]", 1, g_textFormatRight ? g_textFormatRight : g_textFormat, rightRect, g_textBrush);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

void RenderCPS() {
    if (!g_hWnd || !IsWindow(g_hWnd)) return;
    
    RECT clientRect;
    GetClientRect(g_hWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    
    if (width != g_memWidth || height != g_memHeight) {
        if (g_hBitmap) { DeleteObject(g_hBitmap); g_hBitmap = nullptr; }
        if (g_memDC) { DeleteDC(g_memDC); g_memDC = nullptr; }
        g_memDC = CreateCompatibleDC(nullptr);
        BITMAPINFO bmiResize = {};
        bmiResize.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmiResize.bmiHeader.biWidth = width;
        bmiResize.bmiHeader.biHeight = -height;
        bmiResize.bmiHeader.biPlanes = 1;
        bmiResize.bmiHeader.biBitCount = 32;
        bmiResize.bmiHeader.biCompression = BI_RGB;
        void* bitsResize = nullptr;
        g_hBitmap = CreateDIBSection(g_memDC, &bmiResize, DIB_RGB_COLORS, &bitsResize, nullptr, 0);
        SelectObject(g_memDC, g_hBitmap);
        g_memWidth = width;
        g_memHeight = height;
    }
    
    g_dcRenderTarget->BindDC(g_memDC, &clientRect);
    g_dcRenderTarget->BeginDraw();
    g_dcRenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
    
    float s = g_currentScale;
    if (s < 0.6f) s = 0.6f;
    float desiredFont = 14.0f * s;
    if (fabsf(desiredFont - g_lastFontSize) > 0.01f) {
        if (g_textFormat) { g_textFormat->Release(); g_textFormat = nullptr; }
        if (g_textFormatLeft) { g_textFormatLeft->Release(); g_textFormatLeft = nullptr; }
        if (g_textFormatRight) { g_textFormatRight->Release(); g_textFormatRight = nullptr; }
        g_writeFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_BOLD,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       desiredFont, L"en-us", &g_textFormat);
        g_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        g_writeFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_BOLD,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       desiredFont, L"en-us", &g_textFormatLeft);
        g_textFormatLeft->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        g_textFormatLeft->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        g_writeFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_BOLD,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       desiredFont, L"en-us", &g_textFormatRight);
        g_textFormatRight->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        g_textFormatRight->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        g_lastFontSize = desiredFont;
    }
    int startY = (g_lastCPSY >= 0 ? g_lastCPSY : (int)(8 * s));
    int boxW = (int)(BOX_WIDTH * s);
    int boxH = (int)(BOX_HEIGHT * s);
    
    int centerX = (g_lastCPSX >= 0 ? (g_lastCPSX + g_lastCPSWidth / 2) : width / 2);
    
    int lmbCps = calculateCPS(lmbClickTimes, lmbMutex);
    int rmbCps = calculateCPS(rmbClickTimes, rmbMutex);
    
    std::wstring innerText = std::to_wstring(lmbCps) + L" | " + std::to_wstring(rmbCps) + L" CPS";
    
    int cpsX = centerX - boxW / 2;
    float bracketOffset = 0.9f * s;
    DrawCpsTextWithBracketOffset(cpsX, startY, boxW, boxH, innerText, bracketOffset);
    
    g_dcRenderTarget->EndDraw();
    
    HDC hdcScreen = GetDC(nullptr);
    POINT ptSrc = { 0, 0 };
    RECT wr;
    GetWindowRect(g_hWnd, &wr);
    POINT ptDst = { wr.left, wr.top };
    SIZE size = { width, height };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(g_hWnd, hdcScreen, &ptDst, &size, g_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, hdcScreen);
}

void CPS() {
    while (g_running) {
        HWND robloxWindow = WinRTCapture::FindRobloxWindow();
        HWND fg = GetForegroundWindow();
        bool isRobloxActive = (robloxWindow && (fg == robloxWindow || GetParent(fg) == robloxWindow));
        bool gateCheckActive = ParryBar::IsGateCheckActive();
        bool shouldShow = EditHUD::IsEditorActive() || (isRobloxActive && gateCheckActive);
        
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
            bool isFullscreen = (robloxW == screenW && robloxH == screenH);
            float scaleX = (float)robloxW / (float)screenW;
            float scaleY = (float)robloxH / (float)screenH;
            float baseScale = (scaleX < scaleY) ? scaleX : scaleY;
            if (baseScale < 0.6f) baseScale = 0.6f;
            float user = clampf(g_userScale.load(), 0.6f, 2.0f);
            float scale = baseScale * user;
            float maxScale = baseScale * 2.0f;
            if (scale < 0.6f) scale = 0.6f;
            if (scale > maxScale) scale = maxScale;
            float oldScale = g_currentScale;
            g_currentScale = scale;
            int scaledWidth = (int)(BOX_WIDTH * scale);
            int scaledHeight = (int)(BOX_HEIGHT * scale);
            int robloxCenterX = topLeft.x + robloxW / 2;
            int robloxTopY = topLeft.y;
            int baseX = robloxCenterX - scaledWidth / 2;
            int baseY = robloxTopY + (int)(228 * scale);
            
            if (g_hasRequestedPosition.exchange(false)) {
                int rx = g_requestedX.load();
                int ry = g_requestedY.load();
                if (rx >= 0 && ry >= 0) {
                    int availW = robloxW - scaledWidth;
                    int availH = robloxH - scaledHeight;
                    float nx = 0.0f;
                    float ny = 0.0f;
                    if (availW > 0) nx = (float)(rx - topLeft.x) / (float)availW;
                    if (availH > 0) ny = (float)(ry - topLeft.y) / (float)availH;
                    g_offsetXUnits = clampf(nx, 0.0f, 1.0f);
                    g_offsetYUnits = clampf(ny, 0.0f, 1.0f);
                    g_customOffsetEnabled = true;
                }
            }
            
            int cpsX = baseX;
            int cpsY = baseY;
            if (g_customOffsetEnabled.load()) {
                float oxu = g_offsetXUnits.load();
                float oyu = g_offsetYUnits.load();
                int availW = robloxW - scaledWidth;
                int availH = robloxH - scaledHeight;
                cpsX = topLeft.x + (availW > 0 ? (int)lroundf(clampf(oxu, 0.0f, 1.0f) * (float)availW) : 0);
                cpsY = topLeft.y + (availH > 0 ? (int)lroundf(clampf(oyu, 0.0f, 1.0f) * (float)availH) : 0);
            }
            int minX = topLeft.x;
            int minY = topLeft.y;
            int maxX = topLeft.x + robloxW - scaledWidth;
            int maxY = topLeft.y + robloxH - scaledHeight;
            if (maxX < minX) maxX = minX;
            if (maxY < minY) maxY = minY;
            cpsX = clampi(cpsX, minX, maxX);
            cpsY = clampi(cpsY, minY, maxY);
            if (cpsX != g_lastCPSX || cpsY != g_lastCPSY || scaledWidth != g_lastCPSWidth || scaledHeight != g_lastCPSHeight || oldScale != scale) {
                g_lastCPSX = cpsX;
                g_lastCPSY = cpsY;
                g_lastCPSWidth = scaledWidth;
                g_lastCPSHeight = scaledHeight;
                SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, screenW, screenH, SWP_NOACTIVATE | SWP_NOZORDER);
                g_needsRender = true;
            }
        }

        if (g_hWnd && g_WindowShown && IsWindow(g_hWnd)) {
            RenderCPS();
            g_needsRender = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void StartCPS() {
    if (g_running) return;

    if (g_thread.joinable()) {
        g_running = false;
        g_thread.join();
    }

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(g_writeFactory), 
                       reinterpret_cast<IUnknown**>(&g_writeFactory));
    
    if (!g_classRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"CPS";

        if (!RegisterClassExW(&wc)) return;
        g_classRegistered = true;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        L"CPS", L"CPS",
        WS_POPUP, 0, 0, screenWidth, screenHeight,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );

    if (!g_hWnd) return;

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
    g_dcRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    g_dcRenderTarget->SetDpi(96.0f, 96.0f);

    

    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x202020, 0.6f), &g_boxBrush);
    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF, 1.0f), &g_textBrush);

    ShowWindow(g_hWnd, SW_HIDE);
    UpdateWindow(g_hWnd);

    g_WindowShown = false;
    g_running = true;
    g_thread = std::thread(CPS);
}

void StopCPS() {
    g_running = false;
    if (g_thread.joinable()) g_thread.join();
    g_WindowShown = false;
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = nullptr;
    }
    if (g_textFormatLeft) { g_textFormatLeft->Release(); g_textFormatLeft = nullptr; }
    if (g_textFormatRight) { g_textFormatRight->Release(); g_textFormatRight = nullptr; }
    if (g_textBrush) { g_textBrush->Release(); g_textBrush = nullptr; }
    if (g_boxBrush) { g_boxBrush->Release(); g_boxBrush = nullptr; }
    if (g_textFormat) { g_textFormat->Release(); g_textFormat = nullptr; }
    if (g_dcRenderTarget) { g_dcRenderTarget->Release(); g_dcRenderTarget = nullptr; }
    if (g_writeFactory) { g_writeFactory->Release(); g_writeFactory = nullptr; }
    if (g_d2dFactory) { g_d2dFactory->Release(); g_d2dFactory = nullptr; }
    if (g_hBitmap) { DeleteObject(g_hBitmap); g_hBitmap = nullptr; }
    if (g_memDC) { DeleteDC(g_memDC); g_memDC = nullptr; }
}

void SetEnabled(bool enabled) {
}

bool IsEnabled() {
    return g_WindowShown;
}

bool IsCPSRunning() {
    return g_running;
}

void SetUserScale(float s) {
    g_userScale = clampf(s, 0.6f, 2.0f);
    g_needsRender = true;
}

float GetUserScale() {
    return g_userScale.load();
}

void GetCPSPosition(int& x, int& y, int& width, int& height) {
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    float baseScale = 1.0f;
    POINT topLeft = { 0, 0 };
    int robloxW = screenW;
    int robloxH = screenH;
    if (robloxWindow) {
        RECT robloxClientRect;
        GetClientRect(robloxWindow, &robloxClientRect);
        topLeft = { robloxClientRect.left, robloxClientRect.top };
        ClientToScreen(robloxWindow, &topLeft);
        robloxW = robloxClientRect.right - robloxClientRect.left;
        robloxH = robloxClientRect.bottom - robloxClientRect.top;
        float scaleX = (float)robloxW / (float)screenW;
        float scaleY = (float)robloxH / (float)screenH;
        baseScale = (scaleX < scaleY) ? scaleX : scaleY;
        if (baseScale < 0.6f) baseScale = 0.6f;
    }
    float user = clampf(g_userScale.load(), 0.6f, 2.0f);
    float scale = baseScale * user;
    float maxScale = baseScale * 2.0f;
    if (scale < 0.6f) scale = 0.6f;
    if (scale > maxScale) scale = maxScale;
    int scaledWidth = (int)(BOX_WIDTH * scale);
    int scaledHeight = (int)(BOX_HEIGHT * scale);
    int robloxCenterX = topLeft.x + robloxW / 2;
    int robloxTopY = topLeft.y;
    int baseX = robloxCenterX - scaledWidth / 2;
    int baseY = robloxTopY + (int)(228 * scale);
    int cpsX = baseX;
    int cpsY = baseY;
    if (g_customOffsetEnabled.load()) {
        float oxu = g_offsetXUnits.load();
        float oyu = g_offsetYUnits.load();
        int availW = robloxW - scaledWidth;
        int availH = robloxH - scaledHeight;
        cpsX = topLeft.x + (availW > 0 ? (int)lroundf(clampf(oxu, 0.0f, 1.0f) * (float)availW) : 0);
        cpsY = topLeft.y + (availH > 0 ? (int)lroundf(clampf(oyu, 0.0f, 1.0f) * (float)availH) : 0);
    }
    int minX = topLeft.x;
    int minY = topLeft.y;
    int maxX = topLeft.x + robloxW - scaledWidth;
    int maxY = topLeft.y + robloxH - scaledHeight;
    if (maxX < minX) maxX = minX;
    if (maxY < minY) maxY = minY;
    cpsX = clampi(cpsX, minX, maxX);
    cpsY = clampi(cpsY, minY, maxY);
    g_lastCPSX = cpsX;
    g_lastCPSY = cpsY;
    g_lastCPSWidth = scaledWidth;
    g_lastCPSHeight = scaledHeight;
    x = cpsX;
    y = cpsY;
    width = scaledWidth;
    height = scaledHeight;
}

void SetCPSPosition(int x, int y) {
    g_requestedX = x;
    g_requestedY = y;
    g_hasRequestedPosition = true;
    g_needsRender = true;
}

}
