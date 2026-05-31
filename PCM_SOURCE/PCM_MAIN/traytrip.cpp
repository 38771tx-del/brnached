#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include <string>
#include <thread>
#include <tlhelp32.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

namespace Traytip {

const int BASE_WINDOW_WIDTH = 280;
const int BASE_WINDOW_HEIGHT = 120;
const int BORDER_RADIUS = 12;
const int PADDING = 16;
const int TITLE_HEIGHT = 24;
const int DEFAULT_DURATION_MS = 5000;

struct TraytipInstance {
    HWND hWnd = nullptr;
    ID2D1DCRenderTarget* dcRenderTarget = nullptr;
    IDWriteTextFormat* textFormat = nullptr;
    IDWriteTextFormat* titleFormat = nullptr;
    ID2D1SolidColorBrush* backgroundBrush = nullptr;
    ID2D1SolidColorBrush* textBrush = nullptr;
    ID2D1SolidColorBrush* titleBrush = nullptr;
    ID2D1SolidColorBrush* borderBrush = nullptr;
    HDC memDC = nullptr;
    HBITMAP hBitmap = nullptr;
    std::chrono::steady_clock::time_point showTime;
    bool shouldDestroy = false;
    float alpha = 0.0f;
    float scale = 0.8f;
    bool animating = false;
    std::chrono::steady_clock::time_point animationStart;
    std::wstring titleText;
    std::wstring contentText;
    int durationMs = DEFAULT_DURATION_MS;
    bool isScrollable = false;
    float scrollOffset = 0.0f;
    float maxScrollOffset = 0.0f;
    bool isBeingInteractedWith = false;
    std::chrono::steady_clock::time_point lastInteractionTime;
    bool isRepositioning = false;
    std::chrono::steady_clock::time_point repositionStart;
    int startY = 0;
    int targetY = 0;
    int currentY = 0;
    int calculatedHeight = BASE_WINDOW_HEIGHT;
    bool paused = false;
    std::chrono::steady_clock::time_point pauseStart;
};

static HWND g_hWnd = nullptr;
static ID2D1Factory* g_d2dFactory = nullptr;
static ID2D1DCRenderTarget* g_dcRenderTarget = nullptr;
static IDWriteFactory* g_writeFactory = nullptr;
static IDWriteTextFormat* g_textFormat = nullptr;
static IDWriteTextFormat* g_titleFormat = nullptr;
static ID2D1SolidColorBrush* g_backgroundBrush = nullptr;
static ID2D1SolidColorBrush* g_textBrush = nullptr;
static ID2D1SolidColorBrush* g_titleBrush = nullptr;
static ID2D1SolidColorBrush* g_borderBrush = nullptr;
static HDC g_memDC = nullptr;
static HBITMAP g_hBitmap = nullptr;
static std::atomic<bool> g_running{ false };
static std::thread g_thread;
static bool g_classRegistered = false;
static bool g_WindowShown = false;
static int g_lastTraytipX = -1;
static int g_lastTraytipY = -1;
static int g_lastTraytipWidth = -1;
static int g_lastTraytipHeight = -1;
static float g_currentScale = 1.0f;
static int g_memWidth = 0;
static int g_memHeight = 0;
static std::atomic<bool> g_needsRender{ false };
static std::chrono::steady_clock::time_point g_lastRenderTime;
static std::vector<TraytipInstance*> g_traytipInstances;
static std::mutex g_instancesMutex;
static HWND g_cachedRobloxWindow = nullptr;
static DWORD g_lastRobloxWindowCheckMs = 0;

static HWND GetRobloxWindowCached() {
    DWORD now = GetTickCount();
    if (g_cachedRobloxWindow && !IsWindow(g_cachedRobloxWindow)) {
        g_cachedRobloxWindow = nullptr;
    }
    if (!g_cachedRobloxWindow || (now - g_lastRobloxWindowCheckMs) > 500) {
        g_cachedRobloxWindow = WinRTCapture::FindRobloxWindow();
        g_lastRobloxWindowCheckMs = now;
    }
    return g_cachedRobloxWindow;
}

static bool IsRobloxForeground(HWND robloxWindow) {
    if (!robloxWindow) return false;
    HWND fg = GetForegroundWindow();
    return (fg == robloxWindow) || (GetParent(fg) == robloxWindow);
}

static void PauseInstance(TraytipInstance* instance, std::chrono::steady_clock::time_point now) {
    if (!instance || instance->paused) return;
    instance->paused = true;
    instance->pauseStart = now;
}

static void ResumeInstance(TraytipInstance* instance, std::chrono::steady_clock::time_point now) {
    if (!instance || !instance->paused) return;
    auto delta = now - instance->pauseStart;
    instance->showTime += delta;
    if (instance->animating) instance->animationStart += delta;
    if (instance->isRepositioning) instance->repositionStart += delta;
    if (instance->isBeingInteractedWith) instance->lastInteractionTime += delta;
    instance->paused = false;
}

// Forward declarations
void RenderTraytips();
void UpdateTraytipInstances();
void RepositionTraytips();



bool IsPointOverCloseButton(POINT pt, TraytipInstance* instance) {
    if (!instance || instance->shouldDestroy) return false;
    
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (!robloxWindow) return false;
    
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
    
    int scaledWindowWidth = (int)(BASE_WINDOW_WIDTH * scale);
    int scaledWindowHeight = (int)(instance->calculatedHeight * scale);
    int windowX = topLeft.x + robloxW - scaledWindowWidth - 20;
    int windowY = instance->currentY;
    
    float closeButtonSize = 16.0f * scale;
    float closeButtonX = windowX + scaledWindowWidth - closeButtonSize - 8.0f * scale;
    float closeButtonY = windowY + PADDING * scale + 2.0f * scale;
    
    return (pt.x >= closeButtonX && pt.x <= closeButtonX + closeButtonSize &&
            pt.y >= closeButtonY && pt.y <= closeButtonY + closeButtonSize);
}

bool IsPointOverTraytip(POINT pt) {
    std::lock_guard<std::mutex> lock(g_instancesMutex);
    
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (!robloxWindow) return false;
    
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
    
    int scaledWindowWidth = (int)(BASE_WINDOW_WIDTH * scale);
    int windowX = topLeft.x + robloxW - scaledWindowWidth - 20;
    
    for (size_t i = 0; i < g_traytipInstances.size(); i++) {
        TraytipInstance* instance = g_traytipInstances[i];
        if (instance->shouldDestroy) continue;
        
        int scaledWindowHeight = (int)(instance->calculatedHeight * scale);
        int windowY = instance->currentY;
        
        if (pt.x >= windowX && pt.x <= windowX + scaledWindowWidth &&
            pt.y >= windowY && pt.y <= windowY + scaledWindowHeight) {
            return true;
        }
    }
    
    return false;
}

LRESULT CALLBACK TraytipWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SETCURSOR:
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            return TRUE;
        case WM_NCHITTEST: {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            
            if (IsPointOverTraytip(pt)) {
                return HTCLIENT;
            }
            return HTTRANSPARENT;
        }
        case WM_MOUSEACTIVATE:
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_CAPTURECHANGED:
        case WM_MOUSEMOVE:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_ERASEBKGND:
            return 0;
        case WM_LBUTTONDOWN: {
            POINT pt;
            GetCursorPos(&pt);
            
            std::lock_guard<std::mutex> lock(g_instancesMutex);
            for (auto it = g_traytipInstances.begin(); it != g_traytipInstances.end(); ++it) {
                TraytipInstance* instance = *it;
                if (IsPointOverCloseButton(pt, instance)) {
                    instance->shouldDestroy = true;
                    instance->animating = true;
                    instance->animationStart = std::chrono::steady_clock::now();
                    g_needsRender = true;
                    break;
                }
            }
            return 0;
        }
        case WM_LBUTTONUP:
            return 0;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

TraytipInstance* CreateTraytipInstance(const std::wstring& title, const std::wstring& content, int durationMs = DEFAULT_DURATION_MS, bool scrollable = false) {
    TraytipInstance* instance = new TraytipInstance();
    
    instance->showTime = std::chrono::steady_clock::now();
    instance->animating = true;
    instance->animationStart = std::chrono::steady_clock::now();
    instance->alpha = 0.0f;
    instance->scale = 0.8f;
    
    instance->titleText = title;
    instance->contentText = content;
    instance->durationMs = durationMs;
    instance->isScrollable = scrollable;
    
    // Initialize positioning for single-window system
    instance->currentY = 0;
    instance->targetY = 0;
    
    // Calculate dynamic height based on text content
    if (g_writeFactory && g_textFormat) {
        IDWriteTextLayout* textLayout = nullptr;
        float maxWidth = (float)(BASE_WINDOW_WIDTH - PADDING * 2 - 20);
        g_writeFactory->CreateTextLayout(content.c_str(), (UINT32)content.length(), g_textFormat, maxWidth, 1000.0f, &textLayout);
        
        if (textLayout) {
            DWRITE_TEXT_METRICS textMetrics;
            textLayout->GetMetrics(&textMetrics);
            
            float contentHeight = textMetrics.height;
            float minHeight = PADDING + TITLE_HEIGHT + 8 + PADDING;
            float calculatedHeight = PADDING + TITLE_HEIGHT + 8 + contentHeight + PADDING;
            
            if (calculatedHeight < minHeight) {
                calculatedHeight = minHeight;
            }
            
            if (calculatedHeight > BASE_WINDOW_HEIGHT * 2) {
                instance->isScrollable = true;
                instance->calculatedHeight = BASE_WINDOW_HEIGHT;
                float availableHeight = (float)(BASE_WINDOW_HEIGHT - PADDING - TITLE_HEIGHT - 8 - PADDING);
                instance->maxScrollOffset = max(0.0f, contentHeight - availableHeight);
            } else {
                instance->calculatedHeight = (int)calculatedHeight;
            }
            
            textLayout->Release();
        } else {
            instance->calculatedHeight = BASE_WINDOW_HEIGHT;
        }
    } else {
        instance->calculatedHeight = BASE_WINDOW_HEIGHT;
    }
    
    return instance;
}

void UpdateAnimation(TraytipInstance* instance) {
    if (!instance->animating) {
        if (instance->shouldDestroy) {
            instance->alpha = 0.0f;
            instance->scale = 0.8f;
        } else {
            instance->alpha = 1.0f;
            instance->scale = 1.0f;
        }
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - instance->animationStart).count();
    
    const float fadeInDuration = 300.0f;
    const float fadeOutDuration = 150.0f; // Faster fade out
    float animationDuration = instance->shouldDestroy ? fadeOutDuration : fadeInDuration;
    float progress = min(1.0f, (float)elapsed / animationDuration);
    
    progress = 1.0f - (1.0f - progress) * (1.0f - progress);
    
    if (instance->shouldDestroy) {
        instance->alpha = max(0.0f, 1.0f - progress);
        instance->scale = max(0.8f, 1.0f - (0.2f * progress));
    } else {
        instance->alpha = progress;
        instance->scale = 0.8f + (0.2f * progress);
    }
    
    if (progress >= 1.0f) {
        instance->animating = false;
        if (instance->shouldDestroy) {
            instance->alpha = 0.0f;
            instance->scale = 0.8f;
        } else {
            instance->alpha = 1.0f;
            instance->scale = 1.0f;
        }
    }
    
    // Trigger render during animation
    if (instance->animating) {
        g_needsRender = true;
    }
}

void UpdateRepositioningAnimation(TraytipInstance* instance) {
    if (!instance->isRepositioning) {
        return;
    }
    
    // Trigger render when repositioning animation is active
    g_needsRender = true;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - instance->repositionStart).count();
    
    const float repositionDuration = 150.0f; // 150ms for smooth repositioning
    float progress = min(1.0f, (float)elapsed / repositionDuration);
    
    // Smooth easing function (ease-out) for natural drop motion
    progress = 1.0f - (1.0f - progress) * (1.0f - progress);
    
    // Interpolate Y position - update the currentY that's used during rendering
    int newY = instance->startY + (int)((instance->targetY - instance->startY) * progress);
    
    // Update the current Y position used for rendering
    instance->currentY = newY;
    
    // Check if animation is complete
    if (progress >= 1.0f) {
        instance->isRepositioning = false;
        instance->currentY = instance->targetY; // Ensure final position is exact
    }
}

void DrawTraytip(TraytipInstance* instance, int x, int y, int width, int height) {
    if (instance->alpha < 0.01f) return;
    
    D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Scale(instance->scale, instance->scale, D2D1::Point2F(x + width/2.0f, y + height/2.0f));
    g_dcRenderTarget->SetTransform(transform);
    
    float clampedAlpha = max(0.0f, min(1.0f, instance->alpha));
    g_backgroundBrush->SetOpacity(clampedAlpha * 0.75f);
    g_borderBrush->SetOpacity(clampedAlpha * 0.15f);
    g_titleBrush->SetOpacity(clampedAlpha);
    g_textBrush->SetOpacity(clampedAlpha);
    
    D2D1_ROUNDED_RECT backgroundRect = D2D1::RoundedRect(
        D2D1::RectF((float)x, (float)y, (float)(x + width), (float)(y + height)),
        (float)BORDER_RADIUS, (float)BORDER_RADIUS
    );
    
    g_dcRenderTarget->FillRoundedRectangle(backgroundRect, g_backgroundBrush);
    g_dcRenderTarget->DrawRoundedRectangle(backgroundRect, g_borderBrush, 1.0f);
    
    float closeButtonSize = 16.0f;
    float closeButtonX = (float)(x + width - closeButtonSize - 8);
    float closeButtonY = (float)(y + PADDING + 2);
    
    ID2D1SolidColorBrush* closeButtonBrush = nullptr;
    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xCCCCCC, clampedAlpha), &closeButtonBrush);
    
    float lineThickness = 2.0f;
    float padding = 3.0f;
    
    g_dcRenderTarget->DrawLine(
        D2D1::Point2F(closeButtonX + padding, closeButtonY + padding),
        D2D1::Point2F(closeButtonX + closeButtonSize - padding, closeButtonY + closeButtonSize - padding),
        closeButtonBrush,
        lineThickness
    );
    
    g_dcRenderTarget->DrawLine(
        D2D1::Point2F(closeButtonX + closeButtonSize - padding, closeButtonY + padding),
        D2D1::Point2F(closeButtonX + padding, closeButtonY + closeButtonSize - padding),
        closeButtonBrush,
        lineThickness
    );
    
    closeButtonBrush->Release();
    
    D2D1_RECT_F titleRect = D2D1::RectF(
        (float)(x + PADDING), 
        (float)(y + PADDING), 
        (float)(x + width - PADDING - 20),
        (float)(y + PADDING + TITLE_HEIGHT)
    );
    
    D2D1_RECT_F contentRect = D2D1::RectF(
        (float)(x + PADDING), 
        (float)(y + PADDING + TITLE_HEIGHT + 8), 
        (float)(x + width - PADDING - (instance->isScrollable ? 12 : 0)),
        (float)(y + height - PADDING)
    );
    
    if (instance->isScrollable) {
        titleRect.top -= instance->scrollOffset;
        titleRect.bottom -= instance->scrollOffset;
        contentRect.top -= instance->scrollOffset;
        contentRect.bottom -= instance->scrollOffset;
    }
    
    g_dcRenderTarget->DrawText(instance->titleText.c_str(), (UINT32)instance->titleText.length(), g_titleFormat, titleRect, g_titleBrush);
    g_dcRenderTarget->DrawText(instance->contentText.c_str(), (UINT32)instance->contentText.length(), g_textFormat, contentRect, g_textBrush);
    
    if (instance->isScrollable && instance->maxScrollOffset > 0) {
        float scrollbarWidth = 6.0f;
        float scrollbarX = (float)(x + width - scrollbarWidth - 4);
        float scrollbarTop = (float)(y + PADDING + TITLE_HEIGHT + 8);
        float scrollbarBottom = (float)(y + height - PADDING);
        float scrollbarHeight = scrollbarBottom - scrollbarTop;
        
        float thumbHeight = scrollbarHeight * 0.3f;
        float thumbY = scrollbarTop + (instance->scrollOffset / instance->maxScrollOffset) * (scrollbarHeight - thumbHeight);
        
        D2D1_RECT_F scrollbarThumb = D2D1::RectF(
            scrollbarX, 
            thumbY, 
            scrollbarX + scrollbarWidth, 
            thumbY + thumbHeight
        );
        
        ID2D1SolidColorBrush* thumbBrush = nullptr;
        g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x404040, clampedAlpha * 0.9f), &thumbBrush);
        g_dcRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(scrollbarThumb, 2.0f, 2.0f), thumbBrush);
        
        thumbBrush->Release();
    }
}

void RenderTraytips() {
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
    
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (robloxWindow) {
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
        
        int scaledWindowWidth = (int)(BASE_WINDOW_WIDTH * scale);
        
        std::lock_guard<std::mutex> lock(g_instancesMutex);
        for (size_t i = 0; i < g_traytipInstances.size(); i++) {
            TraytipInstance* instance = g_traytipInstances[i];
            int scaledWindowHeight = (int)(instance->calculatedHeight * scale);
            int windowX = topLeft.x + robloxW - scaledWindowWidth - 20;
            
            // Use currentY for positioning (will be animated)
            int windowY = instance->currentY;
            
            DrawTraytip(instance, windowX, windowY, scaledWindowWidth, scaledWindowHeight);
        }
    }
    
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

void RepositionTraytips() {
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (!robloxWindow) {
        return;
    }
    
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
    
    int scaledWindowWidth = (int)(BASE_WINDOW_WIDTH * scale);
    
    int accumulatedOffset = 0;
    for (size_t i = 0; i < g_traytipInstances.size(); i++) {
        TraytipInstance* instance = g_traytipInstances[i];
        int scaledWindowHeight = (int)(instance->calculatedHeight * scale);
        int targetY = topLeft.y + robloxH - scaledWindowHeight - 100 - accumulatedOffset;
        accumulatedOffset += scaledWindowHeight + 10;
        
        // Always update target position for all instances
        if (instance->targetY != targetY) {
            // Only animate if this instance was already positioned (not initial setup)
            if (instance->targetY != 0) {
                instance->isRepositioning = true;
                instance->repositionStart = std::chrono::steady_clock::now();
                instance->startY = instance->currentY; // Use current position as start
                instance->targetY = targetY;
                g_needsRender = true;
            } else {
                // Initial setup - set position immediately
                instance->currentY = targetY;
                instance->targetY = targetY;
            }
        }
    }
}

void UpdateTraytipInstances() {
    std::lock_guard<std::mutex> lock(g_instancesMutex);
    
    auto now = std::chrono::steady_clock::now();
    bool needsRepositioning = false;
    
    for (auto it = g_traytipInstances.begin(); it != g_traytipInstances.end();) {
        TraytipInstance* instance = *it;
        if (instance->paused) {
            ++it;
            continue;
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - instance->showTime).count();
        
        if (instance->isBeingInteractedWith) {
            auto timeSinceInteraction = std::chrono::duration_cast<std::chrono::milliseconds>(now - instance->lastInteractionTime).count();
            if (timeSinceInteraction > 1000) {
                instance->isBeingInteractedWith = false;
                instance->showTime = now;
            }
        }
        
        if (!instance->isBeingInteractedWith && elapsed >= instance->durationMs && !instance->shouldDestroy) {
            instance->shouldDestroy = true;
            instance->animating = true;
            instance->animationStart = now;
        }
        
        UpdateAnimation(instance);
        
        // Update repositioning animation
        UpdateRepositioningAnimation(instance);
        
        if (instance->shouldDestroy && !instance->animating) {
            delete instance;
            it = g_traytipInstances.erase(it);
            g_needsRender = true;
            needsRepositioning = true; // Mark that we need to reposition
        } else if (instance->shouldDestroy && instance->animating) {
            auto animationElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - instance->animationStart).count();
            if (animationElapsed > 2000) {
                delete instance;
                it = g_traytipInstances.erase(it);
                g_needsRender = true;
                needsRepositioning = true; // Mark that we need to reposition
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    
    // Only reposition if notifications were actually removed
    if (needsRepositioning) {
        RepositionTraytips();
    }
}

void TraytipLoop() {
    while (g_running) {
        HWND robloxWindow = GetRobloxWindowCached();
        bool haveRoblox = (robloxWindow != nullptr);
        bool allowDisplay = !haveRoblox || IsRobloxForeground(robloxWindow);
        bool shouldShow = allowDisplay && !g_traytipInstances.empty(); // Show if there are notifications and Roblox is foreground (if present)
        
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

        // Pause/resume all traytips when Roblox is not foreground (if present)
        {
            std::lock_guard<std::mutex> lock(g_instancesMutex);
            auto now = std::chrono::steady_clock::now();
            if (!allowDisplay && haveRoblox) {
                for (auto* instance : g_traytipInstances) PauseInstance(instance, now);
            } else {
                for (auto* instance : g_traytipInstances) ResumeInstance(instance, now);
            }
        }

        if (g_hWnd && robloxWindow && IsWindow(g_hWnd) && allowDisplay) {
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
            float oldScale = g_currentScale;
            g_currentScale = scale;
            
            if (oldScale != scale) {
                g_needsRender = true;
            }
        } else {
            // Use default scale when Roblox is not running
            g_currentScale = 1.0f;
        }

        if (allowDisplay) {
            if (robloxWindow) {
                RepositionTraytips();
            }
            UpdateTraytipInstances();
        }

        // Check if any animations are active (fade-in, fade-out, or repositioning)
        bool hasActiveAnimations = false;
        {
            std::lock_guard<std::mutex> lock(g_instancesMutex);
            for (auto* instance : g_traytipInstances) {
                if (instance->animating || instance->isRepositioning) {
                    hasActiveAnimations = true;
                    break;
                }
            }
        }

        // Render when needed OR when any animations are active
        if (g_hWnd && g_WindowShown && IsWindow(g_hWnd) && (g_needsRender || hasActiveAnimations)) {
            RenderTraytips();
            g_needsRender = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void ShowNotification(const std::wstring& title, const std::wstring& content, int durationMs = DEFAULT_DURATION_MS, bool scrollable = false) {
    TraytipInstance* newInstance = CreateTraytipInstance(title, content, durationMs, scrollable);
    if (newInstance) {
        std::lock_guard<std::mutex> lock(g_instancesMutex);
        g_traytipInstances.push_back(newInstance);
        g_needsRender = true;

        // If Roblox exists but is not foreground, pause this notification immediately.
        HWND robloxWindow = GetRobloxWindowCached();
        bool haveRoblox = (robloxWindow != nullptr);
        bool allowDisplay = !haveRoblox || IsRobloxForeground(robloxWindow);
        if (!allowDisplay && haveRoblox) {
            PauseInstance(newInstance, std::chrono::steady_clock::now());
        } else {
            // Set initial positions for all notifications when we're allowed to display.
            RepositionTraytips();
        }
    }
}

void traytrip(const char* header, const char* description, int durationMs = DEFAULT_DURATION_MS) {
    int headerLen = MultiByteToWideChar(CP_UTF8, 0, header, -1, nullptr, 0);
    int descLen = MultiByteToWideChar(CP_UTF8, 0, description, -1, nullptr, 0);
    
    std::wstring wHeader(headerLen - 1, L'\0');
    std::wstring wDesc(descLen - 1, L'\0');
    
    MultiByteToWideChar(CP_UTF8, 0, header, -1, &wHeader[0], headerLen);
    MultiByteToWideChar(CP_UTF8, 0, description, -1, &wDesc[0], descLen);
    
    bool needsScrolling = false;
    int lineCount = 1;
    for (wchar_t c : wDesc) {
        if (c == L'\n') lineCount++;
    }
    if (lineCount > 4) {
        needsScrolling = true;
    }
    
    ShowNotification(wHeader, wDesc, durationMs, needsScrolling);
}

void traytrip(const wchar_t* header, const wchar_t* description, int durationMs = DEFAULT_DURATION_MS) {
    std::wstring wHeader(header);
    std::wstring wDesc(description);
    
    bool needsScrolling = false;
    int lineCount = 1;
    for (wchar_t c : wDesc) {
        if (c == L'\n') lineCount++;
    }
    if (lineCount > 4) {
        needsScrolling = true;
    }
    
    ShowNotification(wHeader, wDesc, durationMs, needsScrolling);
}

void StartTraytip() {
    if (g_running) return;

    if (g_thread.joinable()) {
        g_running = false;
        g_thread.join();
    }

    g_running = true;
    g_lastRenderTime = std::chrono::steady_clock::now();
    
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(g_writeFactory), 
                       reinterpret_cast<IUnknown**>(&g_writeFactory));
    
    if (!g_classRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = TraytipWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"TraytipNotification";

        if (!RegisterClassExW(&wc)) return;
        g_classRegistered = true;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        L"TraytipNotification", L"TraytipNotification",
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

    g_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                   DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                   16.0f, L"en-us", &g_titleFormat);
    g_titleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    g_titleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    g_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                   DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                   12.0f, L"en-us", &g_textFormat);
    g_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    g_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0A0A0B, 0.75f), &g_backgroundBrush);
    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x404040, 0.15f), &g_borderBrush);
    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF, 1.0f), &g_titleBrush);
    g_dcRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xCCCCCC, 1.0f), &g_textBrush);

    ShowWindow(g_hWnd, SW_SHOW);
    SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    UpdateWindow(g_hWnd);

    g_WindowShown = true;
    g_running = true;
    g_thread = std::thread(TraytipLoop);
}

void StopTraytip() {
    g_running = false;
    if (g_thread.joinable()) g_thread.join();
    g_WindowShown = false;
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = nullptr;
    }
    
    std::lock_guard<std::mutex> lock(g_instancesMutex);
    for (auto* instance : g_traytipInstances) {
        delete instance;
    }
    g_traytipInstances.clear();
    
    if (g_textBrush) { g_textBrush->Release(); g_textBrush = nullptr; }
    if (g_titleBrush) { g_titleBrush->Release(); g_titleBrush = nullptr; }
    if (g_borderBrush) { g_borderBrush->Release(); g_borderBrush = nullptr; }
    if (g_backgroundBrush) { g_backgroundBrush->Release(); g_backgroundBrush = nullptr; }
    if (g_textFormat) { g_textFormat->Release(); g_textFormat = nullptr; }
    if (g_titleFormat) { g_titleFormat->Release(); g_titleFormat = nullptr; }
    if (g_dcRenderTarget) { g_dcRenderTarget->Release(); g_dcRenderTarget = nullptr; }
    if (g_writeFactory) { g_writeFactory->Release(); g_writeFactory = nullptr; }
    if (g_d2dFactory) { g_d2dFactory->Release(); g_d2dFactory = nullptr; }
    if (g_hBitmap) { DeleteObject(g_hBitmap); g_hBitmap = nullptr; }
    if (g_memDC) { DeleteDC(g_memDC); g_memDC = nullptr; }
}

bool IsTraytipRunning() {
    return g_running;
}

}