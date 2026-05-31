#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>

namespace ParryBar {
    bool IsGateCheckActive();
}

namespace WinRTCapture {
    HWND FindRobloxWindow();
}

namespace EditHUD {
    bool IsEditorActive();
}

namespace Keystrokes {

// Global variables
static HWND g_hWnd = nullptr;
static ID2D1Factory* g_d2dFactory = nullptr;
static ID2D1DCRenderTarget* g_dcRenderTarget = nullptr;
static IDWriteFactory* g_writeFactory = nullptr;
static IDWriteTextFormat* g_textFormat = nullptr;
static float g_lastFontSize = -1.0f;
static ID2D1SolidColorBrush* g_boxBrush = nullptr;
static ID2D1SolidColorBrush* g_pressedBrush = nullptr;
static ID2D1SolidColorBrush* g_textBrush = nullptr;
static HDC g_memDC = nullptr;
static HBITMAP g_hBitmap = nullptr;
static std::atomic<bool> g_running{ false };
static std::thread g_thread;
static std::atomic<bool> keysPressed[256]{ false };
static bool g_WindowShown = false;
static bool g_classRegistered = false;
static USHORT g_dodgeScanCode = 16;
static std::wstring g_dodgeKeyName = L"Q";
static USHORT g_parryScanCode = 33;
static std::wstring g_parryKeyName = L"F";
static int g_lastKeystrokesX = -1;
static int g_lastKeystrokesY = -1;
static int g_lastKeystrokesWidth = -1;
static int g_lastKeystrokesHeight = -1;
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

void SetKeyState(int vk, bool pressed) {
    if (vk >= 0 && vk < 256) {
        bool prev = keysPressed[vk].load();
        keysPressed[vk] = pressed;
        if (prev != pressed) {
            g_needsRender = true;
        }
    }
}

void SetDodgeScanCode(USHORT sc) {
    g_dodgeScanCode = sc;
}

void SetDodgeKeyName(const std::wstring& name) {
    g_dodgeKeyName = name;
}

void SetParryScanCode(USHORT sc) {
    g_parryScanCode = sc;
}

void SetParryKeyName(const std::wstring& name) {
    g_parryKeyName = name;
}

// UI Constants
const int WINDOW_WIDTH = 200;
const int WINDOW_HEIGHT = 220;
const int BOX_WIDTH = 42;
const int BOX_HEIGHT = 42;
const int WIDE_BOX_WIDTH = 135;
const int WIDE_BOX_HEIGHT = 35;
const int MOUSE_BOX_WIDTH = 65;
const int MOUSE_BOX_HEIGHT = 35;
const int BORDER_WIDTH = 1;
const int SPACING = 4;

// Colors
const COLORREF BOX_COLOR = RGB(40, 40, 40);
const COLORREF BORDER_COLOR = RGB(80, 80, 80);
const COLORREF TEXT_COLOR = RGB(255, 255, 255);
const COLORREF TRANSPARENT_COLOR = RGB(0, 0, 0);

// Utility

void DrawRoundedBox(int x, int y, int width, int height, const std::wstring& text, bool isPressed) {    
	D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
		D2D1::RectF((float)x, (float)y, (float)(x + width), (float)(y + height)),
		8.0f, 8.0f
	);
	ID2D1SolidColorBrush* brush = isPressed ? g_pressedBrush : g_boxBrush;
	g_dcRenderTarget->FillRoundedRectangle(roundedRect, brush);
	D2D1_RECT_F textRect = D2D1::RectF((float)x, (float)y, (float)(x + width), (float)(y + height));
	g_dcRenderTarget->DrawText(text.c_str(), text.length(), g_textFormat, textRect, g_textBrush);
}

void DrawWideBox(int x, int y, int width, int height, const std::wstring& text, bool isPressed) {
	D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
		D2D1::RectF((float)x, (float)y, (float)(x + width), (float)(y + height)),
		8.0f, 8.0f
	);
	ID2D1SolidColorBrush* brush = isPressed ? g_pressedBrush : g_boxBrush;
	g_dcRenderTarget->FillRoundedRectangle(roundedRect, brush);
	
	// Draw a straight line across the space bar instead of text
	if (text == L"---") {
		float centerY = (float)y + height / 2.0f;
		float startX = (float)x + 20.0f;
		float endX = (float)(x + width) - 20.0f;
		
		D2D1_POINT_2F startPoint = D2D1::Point2F(startX, centerY);
		D2D1_POINT_2F endPoint = D2D1::Point2F(endX, centerY);
		
		g_dcRenderTarget->DrawLine(startPoint, endPoint, g_textBrush, 2.0f);
	} else {
		D2D1_RECT_F textRect = D2D1::RectF((float)x, (float)y, (float)(x + width), (float)(y + height));
		g_dcRenderTarget->DrawText(text.c_str(), text.length(), g_textFormat, textRect, g_textBrush);
	}
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

void RenderKeystrokes() {
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
        g_writeFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_BOLD,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       desiredFont, L"en-us", &g_textFormat);
        g_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        g_lastFontSize = desiredFont;
    }
    int startY = (g_lastKeystrokesY >= 0 ? g_lastKeystrokesY : (int)(15 * s));
    int boxW = (int)(BOX_WIDTH * s);
    int boxH = (int)(BOX_HEIGHT * s);
    int space = (int)(SPACING * s);
    int wideW = (int)(WIDE_BOX_WIDTH * s);
    int wideH = (int)(WIDE_BOX_HEIGHT * s);
    int mouseW = (int)(MOUSE_BOX_WIDTH * s);
    int mouseH = (int)(MOUSE_BOX_HEIGHT * s);
    int totalTopWidth = boxW * 3 + space * 2;
    int qX = (g_lastKeystrokesX >= 0 ? (g_lastKeystrokesX + (g_lastKeystrokesWidth - totalTopWidth) / 2) : (width - totalTopWidth) / 2);
    int wX = qX + boxW + space;
    int fX = wX + boxW + space;
    
    DrawRoundedBox(qX, startY, boxW, boxH, g_dodgeKeyName.c_str(), keysPressed[g_dodgeScanCode]);
    DrawRoundedBox(wX, startY, boxW, boxH, L"W", keysPressed[17]);
    DrawRoundedBox(fX, startY, boxW, boxH, g_parryKeyName.c_str(), keysPressed[g_parryScanCode]);
    
    int middleY = startY + boxH + space;
    int sX = qX + boxW + space;
    int aX = qX;
    int dX = fX;
    DrawRoundedBox(aX, middleY, boxW, boxH, L"A", keysPressed[30]);
    DrawRoundedBox(sX, middleY, boxW, boxH, L"S", keysPressed[31]);
    DrawRoundedBox(dX, middleY, boxW, boxH, L"D", keysPressed[32]);
    
    int wideY = middleY + boxH + space;
    int wideX = qX + (totalTopWidth - wideW) / 2;
    DrawWideBox(wideX, wideY, wideW, wideH, L"---", keysPressed[57]);
    
    int mouseY = wideY + wideH + space;
    int totalMouseWidth = mouseW * 2 + space;
    int lmbX = qX + (totalTopWidth - totalMouseWidth) / 2;
    int rmbX = lmbX + mouseW + space;
    DrawRoundedBox(lmbX, mouseY, mouseW, mouseH, L"LMB", keysPressed[VK_LBUTTON]);
    DrawRoundedBox(rmbX, mouseY, mouseW, mouseH, L"RMB", keysPressed[VK_RBUTTON]);
    
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

void Keystrokes() {
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
            int boxW = (int)(BOX_WIDTH * scale);
            int boxH = (int)(BOX_HEIGHT * scale);
            int space = (int)(SPACING * scale);
            int wideH = (int)(WIDE_BOX_HEIGHT * scale);
            int mouseH = (int)(MOUSE_BOX_HEIGHT * scale);
            int scaledWidth = boxW * 3 + space * 2;
            int scaledHeight = boxH + space + boxH + space + wideH + space + mouseH;
            int robloxCenterX = topLeft.x + robloxW / 2;
            int robloxTopY = topLeft.y;
            int baseX = robloxCenterX - scaledWidth / 2;
            int baseY = robloxTopY + (int)(50 * scale);
            
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
            
            int keystrokesX = baseX;
            int keystrokesY = baseY;
            if (g_customOffsetEnabled.load()) {
                float oxu = g_offsetXUnits.load();
                float oyu = g_offsetYUnits.load();
                int availW = robloxW - scaledWidth;
                int availH = robloxH - scaledHeight;
                keystrokesX = topLeft.x + (availW > 0 ? (int)lroundf(clampf(oxu, 0.0f, 1.0f) * (float)availW) : 0);
                keystrokesY = topLeft.y + (availH > 0 ? (int)lroundf(clampf(oyu, 0.0f, 1.0f) * (float)availH) : 0);
            }
            int minX = topLeft.x;
            int minY = topLeft.y;
            int maxX = topLeft.x + robloxW - scaledWidth;
            int maxY = topLeft.y + robloxH - scaledHeight;
            if (maxX < minX) maxX = minX;
            if (maxY < minY) maxY = minY;
            keystrokesX = clampi(keystrokesX, minX, maxX);
            keystrokesY = clampi(keystrokesY, minY, maxY);
            if (keystrokesX != g_lastKeystrokesX || keystrokesY != g_lastKeystrokesY || scaledWidth != g_lastKeystrokesWidth || scaledHeight != g_lastKeystrokesHeight || oldScale != scale) {
                g_lastKeystrokesX = keystrokesX;
                g_lastKeystrokesY = keystrokesY;
                g_lastKeystrokesWidth = scaledWidth;
                g_lastKeystrokesHeight = scaledHeight;
                SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, screenW, screenH, SWP_NOACTIVATE | SWP_NOZORDER);
                g_needsRender = true;
            }
        }

        if (g_hWnd && g_WindowShown && IsWindow(g_hWnd) && g_needsRender) {
            RenderKeystrokes();
            g_needsRender = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void StartKeystrokes() {
    if (g_running) return;

    // Stop any existing thread first
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
        wc.lpszClassName = L"Keystrokes";

        if (!RegisterClassExW(&wc)) return;
        g_classRegistered = true;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        L"Keystrokes", L"Keystrokes",
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

    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x202020, 0.6f), &g_boxBrush);
    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x606060, 0.8f), &g_pressedBrush);
    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF, 1.0f), &g_textBrush);

    ShowWindow(g_hWnd, SW_HIDE);
    UpdateWindow(g_hWnd);

    g_WindowShown = false;
    g_running = true;
    g_thread = std::thread(Keystrokes);
}

void StopKeystrokes() {
    g_running = false;
    if (g_thread.joinable()) g_thread.join();
    g_WindowShown = false;
    if (g_hWnd) {
    DestroyWindow(g_hWnd);
        g_hWnd = nullptr;
    }
    if (g_textBrush) { g_textBrush->Release(); g_textBrush = nullptr; }
    if (g_pressedBrush) { g_pressedBrush->Release(); g_pressedBrush = nullptr; }
    if (g_boxBrush) { g_boxBrush->Release(); g_boxBrush = nullptr; }
    if (g_textFormat) { g_textFormat->Release(); g_textFormat = nullptr; }
    g_lastFontSize = -1.0f;
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

bool IsKeystrokesRunning() {
    return g_running;
}

void SetUserScale(float s) {
    g_userScale = clampf(s, 0.6f, 2.0f);
    g_needsRender = true;
}

float GetUserScale() {
    return g_userScale.load();
}

void GetKeystrokesPosition(int& x, int& y, int& width, int& height) {
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
    int boxW = (int)(BOX_WIDTH * scale);
    int boxH = (int)(BOX_HEIGHT * scale);
    int space = (int)(SPACING * scale);
    int wideH = (int)(WIDE_BOX_HEIGHT * scale);
    int mouseH = (int)(MOUSE_BOX_HEIGHT * scale);
    int scaledWidth = boxW * 3 + space * 2;
    int scaledHeight = boxH + space + boxH + space + wideH + space + mouseH;
    int robloxCenterX = topLeft.x + robloxW / 2;
    int robloxTopY = topLeft.y;
    int baseX = robloxCenterX - scaledWidth / 2;
    int baseY = robloxTopY + (int)(50 * scale);
    int keystrokesX = baseX;
    int keystrokesY = baseY;
    if (g_customOffsetEnabled.load()) {
        float oxu = g_offsetXUnits.load();
        float oyu = g_offsetYUnits.load();
        int availW = robloxW - scaledWidth;
        int availH = robloxH - scaledHeight;
        keystrokesX = topLeft.x + (availW > 0 ? (int)lroundf(clampf(oxu, 0.0f, 1.0f) * (float)availW) : 0);
        keystrokesY = topLeft.y + (availH > 0 ? (int)lroundf(clampf(oyu, 0.0f, 1.0f) * (float)availH) : 0);
    }
    int minX = topLeft.x;
    int minY = topLeft.y;
    int maxX = topLeft.x + robloxW - scaledWidth;
    int maxY = topLeft.y + robloxH - scaledHeight;
    if (maxX < minX) maxX = minX;
    if (maxY < minY) maxY = minY;
    keystrokesX = clampi(keystrokesX, minX, maxX);
    keystrokesY = clampi(keystrokesY, minY, maxY);
    g_lastKeystrokesX = keystrokesX;
    g_lastKeystrokesY = keystrokesY;
    g_lastKeystrokesWidth = scaledWidth;
    g_lastKeystrokesHeight = scaledHeight;
    x = keystrokesX;
    y = keystrokesY;
    width = scaledWidth;
    height = scaledHeight;
}

void SetKeystrokesPosition(int x, int y) {
    g_requestedX = x;
    g_requestedY = y;
    g_hasRequestedPosition = true;
    g_needsRender = true;
}

}