#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace Keystrokes {
    bool IsKeystrokesRunning();
    void GetKeystrokesPosition(int& x, int& y, int& width, int& height);
    void SetKeystrokesPosition(int x, int y);
    void SetUserScale(float s);
    float GetUserScale();
}

namespace CPS {
    bool IsCPSRunning();
    void GetCPSPosition(int& x, int& y, int& width, int& height);
    void SetCPSPosition(int x, int y);
    void SetUserScale(float s);
    float GetUserScale();
}

namespace ParryBar {
    bool IsParryBarRunning();
    void GetParryBarPosition(int& x, int& y, int& width, int& height);
    void SetParryBarPosition(int x, int y);
    void SetUserScale(float s);
    float GetUserScale();
}

namespace WinRTCapture {
    HWND FindRobloxWindow();
    bool IsCaptureInitialized();
    int GetCaptureWidth();
    int GetCaptureHeight();
}

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

namespace EditHUD {

static HWND g_hWnd = nullptr;
static ID2D1Factory* g_d2dFactory = nullptr;
static ID2D1DCRenderTarget* g_renderTarget = nullptr;
static IDWriteFactory* g_writeFactory = nullptr;
static IDWriteTextFormat* g_textFormat = nullptr;
static ID2D1SolidColorBrush* g_backgroundBrush = nullptr;
static ID2D1SolidColorBrush* g_rectBrush = nullptr;
static ID2D1SolidColorBrush* g_textBrush = nullptr;
static ID2D1SolidColorBrush* g_iconOutlineBrush = nullptr;
static HDC g_memDC = nullptr;
static HBITMAP g_hBitmap = nullptr;
static int g_memWidth = 0;
static int g_memHeight = 0;
static bool g_classRegistered = false;
static bool g_editorActive = false;
static UINT_PTR g_focusTimer = 0;
static UINT_PTR g_positionTimer = 0;

static int g_keystrokesX = -1;
static int g_keystrokesY = -1;
static int g_keystrokesWidth = 200;
static int g_keystrokesHeight = 220;
static bool g_draggingKeystrokes = false;
static int g_dragOffsetX = 0;
static int g_dragOffsetY = 0;
static int g_resizeTarget = 0;
static int g_resizeStartMouseX = 0;
static int g_resizeStartMouseY = 0;
static float g_resizeStartScale = 1.0f;
static int g_resizeAnchorX = 0;
static int g_resizeAnchorY = 0;
static int g_resizeStartW = 1;
static int g_resizeStartH = 1;

static int g_cpsX = -1;
static int g_cpsY = -1;
static int g_cpsWidth = 135;
static int g_cpsHeight = 40;
static bool g_draggingCPS = false;

static int g_parryBarX = -1;
static int g_parryBarY = -1;
static int g_parryBarWidth = 380;
static int g_parryBarHeight = 22;
static bool g_draggingParryBar = false;

static int g_snapThresholdPx = 12;
static int g_snapGapPx = 6;

static bool GetRobloxClientBounds(RECT& outBounds) {
    HWND roblox = WinRTCapture::FindRobloxWindow();
    if (!roblox) return false;
    RECT cr;
    if (!GetClientRect(roblox, &cr)) return false;
    POINT tl = { cr.left, cr.top };
    if (!ClientToScreen(roblox, &tl)) return false;
    outBounds.left = tl.x;
    outBounds.top = tl.y;
    outBounds.right = tl.x + (cr.right - cr.left);
    outBounds.bottom = tl.y + (cr.bottom - cr.top);
    return true;
}

static void ClampToBounds(int& x, int& y, int w, int h, const RECT& b) {
    int minX = b.left;
    int minY = b.top;
    int maxX = b.right - w;
    int maxY = b.bottom - h;
    if (maxX < minX) maxX = minX;
    if (maxY < minY) maxY = minY;
    if (x < minX) x = minX;
    if (y < minY) y = minY;
    if (x > maxX) x = maxX;
    if (y > maxY) y = maxY;
}

static void ApplySnapping(int& x, int& y, int w, int h, const RECT& bounds, const std::vector<RECT>& others) {
    if (GetAsyncKeyState(VK_MENU) & 0x8000) return;
    int bestDx = g_snapThresholdPx + 1;
    int bestDy = g_snapThresholdPx + 1;
    int snapX = x;
    int snapY = y;
    auto considerX = [&](int candidate) {
        int d = std::abs(x - candidate);
        if (d < bestDx) {
            bestDx = d;
            snapX = candidate;
        }
    };
    auto considerY = [&](int candidate) {
        int d = std::abs(y - candidate);
        if (d < bestDy) {
            bestDy = d;
            snapY = candidate;
        }
    };
    considerX(bounds.left);
    considerX(bounds.right - w);
    considerX((bounds.left + bounds.right) / 2 - w / 2);
    considerY(bounds.top);
    considerY(bounds.bottom - h);
    considerY((bounds.top + bounds.bottom) / 2 - h / 2);
    for (const auto& r : others) {
        int rW = r.right - r.left;
        int rH = r.bottom - r.top;
        if (rW <= 0 || rH <= 0) continue;
        considerX(r.left);
        considerX(r.right - w);
        considerX((r.left + r.right) / 2 - w / 2);
        considerX(r.left - w - g_snapGapPx);
        considerX(r.right + g_snapGapPx);
        considerY(r.top);
        considerY(r.bottom - h);
        considerY((r.top + r.bottom) / 2 - h / 2);
        considerY(r.top - h - g_snapGapPx);
        considerY(r.bottom + g_snapGapPx);
    }
    if (bestDx <= g_snapThresholdPx) x = snapX;
    if (bestDy <= g_snapThresholdPx) y = snapY;
}

static void Cleanup() {
    if (g_textBrush) { g_textBrush->Release(); g_textBrush = nullptr; }
    if (g_rectBrush) { g_rectBrush->Release(); g_rectBrush = nullptr; }
    if (g_backgroundBrush) { g_backgroundBrush->Release(); g_backgroundBrush = nullptr; }
    if (g_iconOutlineBrush) { g_iconOutlineBrush->Release(); g_iconOutlineBrush = nullptr; }
    if (g_textFormat) { g_textFormat->Release(); g_textFormat = nullptr; }
    if (g_renderTarget) { g_renderTarget->Release(); g_renderTarget = nullptr; }
    if (g_writeFactory) { g_writeFactory->Release(); g_writeFactory = nullptr; }
    if (g_d2dFactory) { g_d2dFactory->Release(); g_d2dFactory = nullptr; }
    if (g_hBitmap) { DeleteObject(g_hBitmap); g_hBitmap = nullptr; }
    if (g_memDC) { DeleteDC(g_memDC); g_memDC = nullptr; }
    g_hWnd = nullptr;
}

bool IsPointInRect(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight) {
    return x >= rectX && x <= rectX + rectWidth && y >= rectY && y <= rectY + rectHeight;
}

static bool IsRobloxFullscreen() {
    if (!WinRTCapture::IsCaptureInitialized()) return false;
    int clientWidth = WinRTCapture::GetCaptureWidth();
    int clientHeight = WinRTCapture::GetCaptureHeight();
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    return clientWidth == screenWidth && clientHeight == screenHeight;
}

static void EnsureTopMost(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

static bool IsPointInCornerIcon(int px, int py, int x, int y, int w, int h) {
    float r = 10.0f;
    float cx = std::floor((float)(x + w) - 1.0f) + 0.5f;
    float cy = std::floor((float)(y + h) - 1.0f) + 0.5f;
    float dx = (float)px - cx;
    float dy = (float)py - cy;
    return dx * dx + dy * dy <= r * r;
}

static void DrawCornerButton(const D2D1_RECT_F& rect) {
    if (!g_renderTarget) return;
    float r = 10.0f;
    float cx = std::floor(rect.right - 1.0f) + 0.5f;
    float cy = std::floor(rect.bottom - 1.0f) + 0.5f;
    if (!g_textBrush) return;
    float iconSize = 12.0f;
    float s = iconSize / 24.0f;
    float iconMinX = 3.0f;
    float iconMinY = 3.0f;
    float iconMaxX = 15.0f;
    float iconMaxY = 15.0f;
    float iconCenterX = (iconMinX + iconMaxX) * 0.5f;
    float iconCenterY = (iconMinY + iconMaxY) * 0.5f;
    float ox = cx - iconCenterX * s;
    float oy = cy - iconCenterY * s;
    ox = std::round(ox);
    oy = std::round(oy);
    auto p = [&](float x, float y) { return D2D1::Point2F(ox + x * s, oy + y * s); };
    float sw = 2.0f * s;
    if (sw < 1.0f) sw = 1.0f;
    float ow = sw + 1.5f;
    if (g_iconOutlineBrush) {
        g_renderTarget->DrawLine(p(3, 3), p(15, 15), g_iconOutlineBrush, ow);
        g_renderTarget->DrawLine(p(10, 15), p(15, 15), g_iconOutlineBrush, ow);
        g_renderTarget->DrawLine(p(15, 15), p(15, 10), g_iconOutlineBrush, ow);
        g_renderTarget->DrawLine(p(8, 3), p(3, 3), g_iconOutlineBrush, ow);
        g_renderTarget->DrawLine(p(3, 3), p(3, 8), g_iconOutlineBrush, ow);
    }
    g_renderTarget->DrawLine(p(3, 3), p(15, 15), g_textBrush, sw);
    g_renderTarget->DrawLine(p(10, 15), p(15, 15), g_textBrush, sw);
    g_renderTarget->DrawLine(p(15, 15), p(15, 10), g_textBrush, sw);
    g_renderTarget->DrawLine(p(8, 3), p(3, 3), g_textBrush, sw);
    g_renderTarget->DrawLine(p(3, 3), p(3, 8), g_textBrush, sw);
}

static void SaveAndExit(HWND hwnd) {
    RECT bounds;
    bool haveBounds = GetRobloxClientBounds(bounds);
    if (!haveBounds) {
        GetClientRect(hwnd, &bounds);
    }
    if (Keystrokes::IsKeystrokesRunning() && g_keystrokesX >= 0 && g_keystrokesY >= 0) {
        int x = g_keystrokesX;
        int y = g_keystrokesY;
        ClampToBounds(x, y, g_keystrokesWidth, g_keystrokesHeight, bounds);
        g_keystrokesX = x;
        g_keystrokesY = y;
        Keystrokes::SetKeystrokesPosition(x, y);
        HudLayoutSaveKeystrokes(x, y, Keystrokes::GetUserScale(), true);
    }
    if (CPS::IsCPSRunning() && g_cpsX >= 0 && g_cpsY >= 0) {
        int x = g_cpsX;
        int y = g_cpsY;
        ClampToBounds(x, y, g_cpsWidth, g_cpsHeight, bounds);
        g_cpsX = x;
        g_cpsY = y;
        CPS::SetCPSPosition(x, y);
        HudLayoutSaveCPS(x, y, CPS::GetUserScale(), true);
    }
    if (ParryBar::IsParryBarRunning() && g_parryBarX >= 0 && g_parryBarY >= 0) {
        int x = g_parryBarX;
        int y = g_parryBarY;
        ClampToBounds(x, y, g_parryBarWidth, g_parryBarHeight, bounds);
        g_parryBarX = x;
        g_parryBarY = y;
        ParryBar::SetParryBarPosition(x, y);
        HudLayoutSaveParryBar(x, y, ParryBar::GetUserScale(), true);
    }
    g_editorActive = false;
    DestroyWindow(hwnd);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_NCHITTEST:
        return HTCLIENT;
    case WM_MOUSEACTIVATE:
        return MA_ACTIVATE;
    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            HCURSOR cur = LoadCursor(nullptr, IDC_ARROW);
            if (Keystrokes::IsKeystrokesRunning() && g_keystrokesX >= 0 && g_keystrokesY >= 0 && g_keystrokesWidth > 0 && g_keystrokesHeight > 0) {
                if (IsPointInCornerIcon(pt.x, pt.y, g_keystrokesX, g_keystrokesY, g_keystrokesWidth, g_keystrokesHeight)) {
                    cur = LoadCursor(nullptr, IDC_SIZENWSE);
                } else if (IsPointInRect(pt.x, pt.y, g_keystrokesX, g_keystrokesY, g_keystrokesWidth, g_keystrokesHeight)) {
                    cur = LoadCursor(nullptr, IDC_SIZEALL);
                }
            }
            if (cur == LoadCursor(nullptr, IDC_ARROW) && CPS::IsCPSRunning() && g_cpsX >= 0 && g_cpsY >= 0 && g_cpsWidth > 0 && g_cpsHeight > 0) {
                if (IsPointInCornerIcon(pt.x, pt.y, g_cpsX, g_cpsY, g_cpsWidth, g_cpsHeight)) {
                    cur = LoadCursor(nullptr, IDC_SIZENWSE);
                } else if (IsPointInRect(pt.x, pt.y, g_cpsX, g_cpsY, g_cpsWidth, g_cpsHeight)) {
                    cur = LoadCursor(nullptr, IDC_SIZEALL);
                }
            }
            if (cur == LoadCursor(nullptr, IDC_ARROW) && ParryBar::IsParryBarRunning() && g_parryBarX >= 0 && g_parryBarY >= 0 && g_parryBarWidth > 0 && g_parryBarHeight > 0) {
                if (IsPointInCornerIcon(pt.x, pt.y, g_parryBarX, g_parryBarY, g_parryBarWidth, g_parryBarHeight)) {
                    cur = LoadCursor(nullptr, IDC_SIZENWSE);
                } else if (IsPointInRect(pt.x, pt.y, g_parryBarX, g_parryBarY, g_parryBarWidth, g_parryBarHeight)) {
                    cur = LoadCursor(nullptr, IDC_SIZEALL);
                }
            }
            SetCursor(cur);
            return TRUE;
        }
        break;
    }
    case WM_ACTIVATE:
        if (wParam == WA_INACTIVE) {
            SaveAndExit(hwnd);
            return 0;
        }
        SetFocus(hwnd);
        return 0;
    case WM_TIMER:
        if (wParam == 1) {
            HWND fg = GetForegroundWindow();
            if (fg != hwnd) {
                BringWindowToTop(hwnd);
                SetForegroundWindow(hwnd);
                SetActiveWindow(hwnd);
                SetFocus(hwnd);
            } else {
                KillTimer(hwnd, 1);
                g_focusTimer = 0;
            }
        } else if (wParam == 2) {
            EnsureTopMost(hwnd);
            if (!IsRobloxFullscreen()) {
                SaveAndExit(hwnd);
                return 0;
            }
            RECT bounds;
            bool haveBounds = GetRobloxClientBounds(bounds);
            if (!haveBounds) {
                GetClientRect(hwnd, &bounds);
            }
            bool changed = false;
            if (!Keystrokes::IsKeystrokesRunning()) {
                if (g_keystrokesX >= 0 || g_keystrokesY >= 0 || g_keystrokesWidth > 0 || g_keystrokesHeight > 0) {
                    g_keystrokesX = -1;
                    g_keystrokesY = -1;
                    changed = true;
                }
            }
            if (Keystrokes::IsKeystrokesRunning() && !g_draggingKeystrokes && g_resizeTarget != 1) {
                int x = -1, y = -1, w = 0, h = 0;
                Keystrokes::GetKeystrokesPosition(x, y, w, h);
                if (x >= 0 && y >= 0 && w > 0 && h > 0) {
                    int cx = x;
                    int cy = y;
                    ClampToBounds(cx, cy, w, h, bounds);
                    if (cx != x || cy != y) {
                        Keystrokes::SetKeystrokesPosition(cx, cy);
                        x = cx;
                        y = cy;
                    }
                    if (g_keystrokesX != x || g_keystrokesY != y || g_keystrokesWidth != w || g_keystrokesHeight != h) {
                        g_keystrokesX = x;
                        g_keystrokesY = y;
                        g_keystrokesWidth = w;
                        g_keystrokesHeight = h;
                        changed = true;
                    }
                }
            }
            if (!CPS::IsCPSRunning()) {
                if (g_cpsX >= 0 || g_cpsY >= 0 || g_cpsWidth > 0 || g_cpsHeight > 0) {
                    g_cpsX = -1;
                    g_cpsY = -1;
                    changed = true;
                }
            }
            if (CPS::IsCPSRunning() && !g_draggingCPS && g_resizeTarget != 2) {
                int x = -1, y = -1, w = 0, h = 0;
                CPS::GetCPSPosition(x, y, w, h);
                if (x >= 0 && y >= 0 && w > 0 && h > 0) {
                    int cx = x;
                    int cy = y;
                    ClampToBounds(cx, cy, w, h, bounds);
                    if (cx != x || cy != y) {
                        CPS::SetCPSPosition(cx, cy);
                        x = cx;
                        y = cy;
                    }
                    if (g_cpsX != x || g_cpsY != y || g_cpsWidth != w || g_cpsHeight != h) {
                        g_cpsX = x;
                        g_cpsY = y;
                        g_cpsWidth = w;
                        g_cpsHeight = h;
                        changed = true;
                    }
                }
            }
            if (!ParryBar::IsParryBarRunning()) {
                if (g_parryBarX >= 0 || g_parryBarY >= 0 || g_parryBarWidth > 0 || g_parryBarHeight > 0) {
                    g_parryBarX = -1;
                    g_parryBarY = -1;
                    changed = true;
                }
            }
            if (ParryBar::IsParryBarRunning() && !g_draggingParryBar && g_resizeTarget != 3) {
                int x = -1, y = -1, w = 0, h = 0;
                ParryBar::GetParryBarPosition(x, y, w, h);
                if (x >= 0 && y >= 0 && w > 0 && h > 0) {
                    int cx = x;
                    int cy = y;
                    ClampToBounds(cx, cy, w, h, bounds);
                    if (cx != x || cy != y) {
                        ParryBar::SetParryBarPosition(cx, cy);
                        x = cx;
                        y = cy;
                    }
                    if (g_parryBarX != x || g_parryBarY != y || g_parryBarWidth != w || g_parryBarHeight != h) {
                        g_parryBarX = x;
                        g_parryBarY = y;
                        g_parryBarWidth = w;
                        g_parryBarHeight = h;
                        changed = true;
                    }
                }
            }
            if (changed) InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_DESTROY:
        if (g_focusTimer) {
            KillTimer(hwnd, g_focusTimer);
            g_focusTimer = 0;
        }
        if (g_positionTimer) {
            KillTimer(hwnd, g_positionTimer);
            g_positionTimer = 0;
        }
        g_editorActive = false;
        Cleanup();
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            SaveAndExit(hwnd);
            return 0;
        }
        break;
    case WM_LBUTTONDOWN: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        if (Keystrokes::IsKeystrokesRunning() && g_keystrokesX >= 0 && g_keystrokesY >= 0 && g_keystrokesWidth > 0 && g_keystrokesHeight > 0 &&
            IsPointInCornerIcon(pt.x, pt.y, g_keystrokesX, g_keystrokesY, g_keystrokesWidth, g_keystrokesHeight)) {
            g_resizeTarget = 1;
            g_resizeStartMouseX = pt.x;
            g_resizeStartMouseY = pt.y;
            g_resizeStartScale = Keystrokes::GetUserScale();
            g_resizeAnchorX = g_keystrokesX;
            g_resizeAnchorY = g_keystrokesY;
            g_resizeStartW = g_keystrokesWidth > 0 ? g_keystrokesWidth : 1;
            g_resizeStartH = g_keystrokesHeight > 0 ? g_keystrokesHeight : 1;
            SetCapture(hwnd);
            return 0;
        }
        if (CPS::IsCPSRunning() && g_cpsX >= 0 && g_cpsY >= 0 && g_cpsWidth > 0 && g_cpsHeight > 0 &&
            IsPointInCornerIcon(pt.x, pt.y, g_cpsX, g_cpsY, g_cpsWidth, g_cpsHeight)) {
            g_resizeTarget = 2;
            g_resizeStartMouseX = pt.x;
            g_resizeStartMouseY = pt.y;
            g_resizeStartScale = CPS::GetUserScale();
            g_resizeAnchorX = g_cpsX;
            g_resizeAnchorY = g_cpsY;
            g_resizeStartW = g_cpsWidth > 0 ? g_cpsWidth : 1;
            g_resizeStartH = g_cpsHeight > 0 ? g_cpsHeight : 1;
            SetCapture(hwnd);
            return 0;
        }
        if (ParryBar::IsParryBarRunning() && g_parryBarX >= 0 && g_parryBarY >= 0 && g_parryBarWidth > 0 && g_parryBarHeight > 0 &&
            IsPointInCornerIcon(pt.x, pt.y, g_parryBarX, g_parryBarY, g_parryBarWidth, g_parryBarHeight)) {
            g_resizeTarget = 3;
            g_resizeStartMouseX = pt.x;
            g_resizeStartMouseY = pt.y;
            g_resizeStartScale = ParryBar::GetUserScale();
            g_resizeAnchorX = g_parryBarX;
            g_resizeAnchorY = g_parryBarY;
            g_resizeStartW = g_parryBarWidth > 0 ? g_parryBarWidth : 1;
            g_resizeStartH = g_parryBarHeight > 0 ? g_parryBarHeight : 1;
            SetCapture(hwnd);
            return 0;
        }
        if (Keystrokes::IsKeystrokesRunning() && g_keystrokesX >= 0 && g_keystrokesY >= 0 && IsPointInRect(pt.x, pt.y, g_keystrokesX, g_keystrokesY, g_keystrokesWidth, g_keystrokesHeight)) {
            g_draggingKeystrokes = true;
            g_dragOffsetX = pt.x - g_keystrokesX;
            g_dragOffsetY = pt.y - g_keystrokesY;
            SetCapture(hwnd);
            return 0;
        }
        if (CPS::IsCPSRunning() && g_cpsX >= 0 && g_cpsY >= 0 && IsPointInRect(pt.x, pt.y, g_cpsX, g_cpsY, g_cpsWidth, g_cpsHeight)) {
            g_draggingCPS = true;
            g_dragOffsetX = pt.x - g_cpsX;
            g_dragOffsetY = pt.y - g_cpsY;
            SetCapture(hwnd);
            return 0;
        }
        if (ParryBar::IsParryBarRunning() && g_parryBarX >= 0 && g_parryBarY >= 0 && IsPointInRect(pt.x, pt.y, g_parryBarX, g_parryBarY, g_parryBarWidth, g_parryBarHeight)) {
            g_draggingParryBar = true;
            g_dragOffsetX = pt.x - g_parryBarX;
            g_dragOffsetY = pt.y - g_parryBarY;
            SetCapture(hwnd);
            return 0;
        }
        break;
    }
    case WM_LBUTTONUP:
        g_resizeTarget = 0;
        g_draggingKeystrokes = false;
        g_draggingCPS = false;
        g_draggingParryBar = false;
        ReleaseCapture();
        return 0;
    case WM_MOUSEMOVE: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        RECT bounds;
        bool haveBounds = GetRobloxClientBounds(bounds);
        if (!haveBounds) {
            GetClientRect(hwnd, &bounds);
        }
        if (g_resizeTarget != 0) {
            EnsureTopMost(hwnd);
            float dx = (float)(pt.x - g_resizeAnchorX);
            float dy = (float)(pt.y - g_resizeAnchorY);
            if (dx < 1.0f) dx = 1.0f;
            if (dy < 1.0f) dy = 1.0f;
            float vw = (float)g_resizeStartW;
            float vh = (float)g_resizeStartH;
            float denom = vw * vw + vh * vh;
            if (denom < 1.0f) denom = 1.0f;
            float factor = (dx * vw + dy * vh) / denom;
            if (factor < 0.05f) factor = 0.05f;
            float newScale = g_resizeStartScale * factor;
            if (newScale < 0.6f) newScale = 0.6f;
            if (newScale > 2.0f) newScale = 2.0f;
            if (g_resizeTarget == 1) {
                Keystrokes::SetUserScale(newScale);
                Keystrokes::SetKeystrokesPosition(g_resizeAnchorX, g_resizeAnchorY);
                int x = -1, y = -1, w = 0, h = 0;
                Keystrokes::GetKeystrokesPosition(x, y, w, h);
                if (w > 0 && h > 0) {
                    if (x >= 0 && y >= 0) {
                        g_keystrokesX = x;
                        g_keystrokesY = y;
                        g_resizeAnchorX = x;
                        g_resizeAnchorY = y;
                    }
                    g_keystrokesWidth = w;
                    g_keystrokesHeight = h;
                }
            } else if (g_resizeTarget == 2) {
                CPS::SetUserScale(newScale);
                CPS::SetCPSPosition(g_resizeAnchorX, g_resizeAnchorY);
                int x = -1, y = -1, w = 0, h = 0;
                CPS::GetCPSPosition(x, y, w, h);
                if (w > 0 && h > 0) {
                    if (x >= 0 && y >= 0) {
                        g_cpsX = x;
                        g_cpsY = y;
                        g_resizeAnchorX = x;
                        g_resizeAnchorY = y;
                    }
                    g_cpsWidth = w;
                    g_cpsHeight = h;
                }
            } else if (g_resizeTarget == 3) {
                ParryBar::SetUserScale(newScale);
                ParryBar::SetParryBarPosition(g_resizeAnchorX, g_resizeAnchorY);
                int x = -1, y = -1, w = 0, h = 0;
                ParryBar::GetParryBarPosition(x, y, w, h);
                if (w > 0 && h > 0) {
                    if (x >= 0 && y >= 0) {
                        g_parryBarX = x;
                        g_parryBarY = y;
                        g_resizeAnchorX = x;
                        g_resizeAnchorY = y;
                    }
                    g_parryBarWidth = w;
                    g_parryBarHeight = h;
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_draggingKeystrokes) {
            EnsureTopMost(hwnd);
            int x = pt.x - g_dragOffsetX;
            int y = pt.y - g_dragOffsetY;
            std::vector<RECT> others;
            if (CPS::IsCPSRunning() && g_cpsX >= 0 && g_cpsY >= 0 && g_cpsWidth > 0 && g_cpsHeight > 0) {
                others.push_back({ g_cpsX, g_cpsY, g_cpsX + g_cpsWidth, g_cpsY + g_cpsHeight });
            }
            if (ParryBar::IsParryBarRunning() && g_parryBarX >= 0 && g_parryBarY >= 0 && g_parryBarWidth > 0 && g_parryBarHeight > 0) {
                others.push_back({ g_parryBarX, g_parryBarY, g_parryBarX + g_parryBarWidth, g_parryBarY + g_parryBarHeight });
            }
            ApplySnapping(x, y, g_keystrokesWidth, g_keystrokesHeight, bounds, others);
            ClampToBounds(x, y, g_keystrokesWidth, g_keystrokesHeight, bounds);
            g_keystrokesX = x;
            g_keystrokesY = y;
            Keystrokes::SetKeystrokesPosition(x, y);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_draggingCPS) {
            EnsureTopMost(hwnd);
            int x = pt.x - g_dragOffsetX;
            int y = pt.y - g_dragOffsetY;
            std::vector<RECT> others;
            if (Keystrokes::IsKeystrokesRunning() && g_keystrokesX >= 0 && g_keystrokesY >= 0 && g_keystrokesWidth > 0 && g_keystrokesHeight > 0) {
                others.push_back({ g_keystrokesX, g_keystrokesY, g_keystrokesX + g_keystrokesWidth, g_keystrokesY + g_keystrokesHeight });
            }
            if (ParryBar::IsParryBarRunning() && g_parryBarX >= 0 && g_parryBarY >= 0 && g_parryBarWidth > 0 && g_parryBarHeight > 0) {
                others.push_back({ g_parryBarX, g_parryBarY, g_parryBarX + g_parryBarWidth, g_parryBarY + g_parryBarHeight });
            }
            ApplySnapping(x, y, g_cpsWidth, g_cpsHeight, bounds, others);
            ClampToBounds(x, y, g_cpsWidth, g_cpsHeight, bounds);
            g_cpsX = x;
            g_cpsY = y;
            CPS::SetCPSPosition(x, y);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (g_draggingParryBar) {
            EnsureTopMost(hwnd);
            int x = pt.x - g_dragOffsetX;
            int y = pt.y - g_dragOffsetY;
            std::vector<RECT> others;
            if (Keystrokes::IsKeystrokesRunning() && g_keystrokesX >= 0 && g_keystrokesY >= 0 && g_keystrokesWidth > 0 && g_keystrokesHeight > 0) {
                others.push_back({ g_keystrokesX, g_keystrokesY, g_keystrokesX + g_keystrokesWidth, g_keystrokesY + g_keystrokesHeight });
            }
            if (CPS::IsCPSRunning() && g_cpsX >= 0 && g_cpsY >= 0 && g_cpsWidth > 0 && g_cpsHeight > 0) {
                others.push_back({ g_cpsX, g_cpsY, g_cpsX + g_cpsWidth, g_cpsY + g_cpsHeight });
            }
            ApplySnapping(x, y, g_parryBarWidth, g_parryBarHeight, bounds, others);
            ClampToBounds(x, y, g_parryBarWidth, g_parryBarHeight, bounds);
            g_parryBarX = x;
            g_parryBarY = y;
            ParryBar::SetParryBarPosition(x, y);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }
    case WM_PAINT: {
        ValidateRect(hwnd, nullptr);
        
        if (!g_renderTarget) return 0;
        
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
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
        
        g_renderTarget->BindDC(g_memDC, &clientRect);
        g_renderTarget->BeginDraw();
        g_renderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.004f));
        
        if (g_textFormat && g_textBrush) {
            D2D1_RECT_F topTextRect = D2D1::RectF(0.0f, 6.0f, (float)width, 30.0f);
            g_renderTarget->DrawText(L"[Press ESC To Exit]", 19, g_textFormat, topTextRect, g_textBrush);
            D2D1_RECT_F snapTextRect = D2D1::RectF(0.0f, 26.0f, (float)width, 50.0f);
            g_renderTarget->DrawText(L"[Hold ALT To Disable Snapping]", 30, g_textFormat, snapTextRect, g_textBrush);
        }
        
        if (g_rectBrush && Keystrokes::IsKeystrokesRunning() && g_keystrokesX >= 0 && g_keystrokesY >= 0 && g_keystrokesWidth > 0 && g_keystrokesHeight > 0) {
            D2D1_RECT_F rect = D2D1::RectF((float)g_keystrokesX, (float)g_keystrokesY, 
                                           (float)(g_keystrokesX + g_keystrokesWidth), 
                                           (float)(g_keystrokesY + g_keystrokesHeight));
            g_renderTarget->DrawRectangle(rect, g_rectBrush, 2.0f);
            DrawCornerButton(rect);
            
            if (g_textFormat && g_textBrush) {
                D2D1_RECT_F textRect = D2D1::RectF((float)g_keystrokesX, (float)(g_keystrokesY - 20), 
                                                   (float)(g_keystrokesX + g_keystrokesWidth), (float)g_keystrokesY);
                g_renderTarget->DrawText(L"Keystrokes", 10, g_textFormat, textRect, g_textBrush);
            }
        }
        
        if (g_rectBrush && CPS::IsCPSRunning() && g_cpsX >= 0 && g_cpsY >= 0 && g_cpsWidth > 0 && g_cpsHeight > 0) {
            D2D1_RECT_F rect = D2D1::RectF((float)g_cpsX, (float)g_cpsY, 
                                           (float)(g_cpsX + g_cpsWidth), 
                                           (float)(g_cpsY + g_cpsHeight));
            g_renderTarget->DrawRectangle(rect, g_rectBrush, 2.0f);
            DrawCornerButton(rect);
            
            if (g_textFormat && g_textBrush) {
                D2D1_RECT_F textRect = D2D1::RectF((float)g_cpsX, (float)(g_cpsY - 20), 
                                                   (float)(g_cpsX + g_cpsWidth), (float)g_cpsY);
                g_renderTarget->DrawText(L"CPS", 3, g_textFormat, textRect, g_textBrush);
            }
        }
        
        if (g_rectBrush && ParryBar::IsParryBarRunning() && g_parryBarX >= 0 && g_parryBarY >= 0 && g_parryBarWidth > 0 && g_parryBarHeight > 0) {
            D2D1_RECT_F rect = D2D1::RectF((float)g_parryBarX, (float)g_parryBarY, 
                                           (float)(g_parryBarX + g_parryBarWidth), 
                                           (float)(g_parryBarY + g_parryBarHeight));
            g_renderTarget->DrawRectangle(rect, g_rectBrush, 2.0f);
            DrawCornerButton(rect);
            
            if (g_textFormat && g_textBrush) {
                D2D1_RECT_F textRect = D2D1::RectF((float)g_parryBarX, (float)(g_parryBarY - 20), 
                                                   (float)(g_parryBarX + g_parryBarWidth), (float)g_parryBarY);
                g_renderTarget->DrawText(L"Parry Bar", 9, g_textFormat, textRect, g_textBrush);
            }
        }
        
        g_renderTarget->EndDraw();
        
        HDC hdcScreen = GetDC(nullptr);
        POINT ptSrc = { 0, 0 };
        RECT wr;
        GetWindowRect(hwnd, &wr);
        POINT ptDst = { wr.left, wr.top };
        SIZE size = { width, height };
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        UpdateLayeredWindow(hwnd, hdcScreen, &ptDst, &size, g_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
        ReleaseDC(nullptr, hdcScreen);
        
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void StartEditor() {
    if (g_editorActive) return;
    if (!IsRobloxFullscreen()) return;
    
    g_keystrokesX = -1;
    g_keystrokesY = -1;
    g_cpsX = -1;
    g_cpsY = -1;
    g_parryBarX = -1;
    g_parryBarY = -1;
    
    if (Keystrokes::IsKeystrokesRunning()) {
        Keystrokes::GetKeystrokesPosition(g_keystrokesX, g_keystrokesY, g_keystrokesWidth, g_keystrokesHeight);
    }
    if (CPS::IsCPSRunning()) {
        CPS::GetCPSPosition(g_cpsX, g_cpsY, g_cpsWidth, g_cpsHeight);
    }
    if (ParryBar::IsParryBarRunning()) {
        ParryBar::GetParryBarPosition(g_parryBarX, g_parryBarY, g_parryBarWidth, g_parryBarHeight);
    }
    
    Cleanup();
    
    HRESULT hrD2D = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory);
    if (FAILED(hrD2D) || !g_d2dFactory) return;
    HRESULT hrDW = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(g_writeFactory), reinterpret_cast<IUnknown**>(&g_writeFactory));
    if (FAILED(hrDW) || !g_writeFactory) return;
    
    if (!g_classRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"EditHUD";
        
        if (!RegisterClassExW(&wc)) { Cleanup(); return; }
        g_classRegistered = true;
    }
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    g_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"EditHUD", L"Edit HUD Layout",
        WS_POPUP, 0, 0, screenWidth, screenHeight,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    if (!g_hWnd) { Cleanup(); return; }
    
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
    g_memWidth = clientRect.right - clientRect.left;
    g_memHeight = clientRect.bottom - clientRect.top;
    
    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT
    );
    HRESULT hrRT = g_d2dFactory->CreateDCRenderTarget(&rtProps, &g_renderTarget);
    if (FAILED(hrRT) || !g_renderTarget) { DestroyWindow(g_hWnd); Cleanup(); return; }
    g_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    g_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    
    HRESULT hrBrush1 = g_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &g_rectBrush);
    if (FAILED(hrBrush1)) g_rectBrush = nullptr;
    HRESULT hrBrush2 = g_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &g_textBrush);
    if (FAILED(hrBrush2)) g_textBrush = nullptr;
    HRESULT hrBrush3 = g_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.95f), &g_iconOutlineBrush);
    if (FAILED(hrBrush3)) g_iconOutlineBrush = nullptr;
    
    HRESULT hrTF = g_writeFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                   DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                   14.0f, L"en-us", &g_textFormat);
    if (FAILED(hrTF)) g_textFormat = nullptr;
    if (g_textFormat) {
        g_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        g_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    
    ShowWindow(g_hWnd, SW_SHOW);
    SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (robloxWindow) {
        DWORD robloxThread = GetWindowThreadProcessId(robloxWindow, nullptr);
        DWORD myThread = GetCurrentThreadId();
        if (robloxThread != myThread) {
            AttachThreadInput(robloxThread, myThread, TRUE);
        }
        BringWindowToTop(g_hWnd);
        SetForegroundWindow(g_hWnd);
        SetActiveWindow(g_hWnd);
        SetFocus(g_hWnd);
        if (robloxThread != myThread) {
            AttachThreadInput(robloxThread, myThread, FALSE);
        }
    } else {
        BringWindowToTop(g_hWnd);
        SetForegroundWindow(g_hWnd);
        SetActiveWindow(g_hWnd);
        SetFocus(g_hWnd);
    }
    
    g_focusTimer = SetTimer(g_hWnd, 1, 50, nullptr);
    g_positionTimer = SetTimer(g_hWnd, 2, 440, nullptr);
    g_editorActive = true;
    InvalidateRect(g_hWnd, nullptr, FALSE);
}

void StopEditor() {
    if (g_hWnd && IsWindow(g_hWnd)) {
        DestroyWindow(g_hWnd);
    }
}

bool IsEditorActive() {
    return g_editorActive;
}

}

