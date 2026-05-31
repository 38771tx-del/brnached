#ifndef ZOOM_LIB_H
#define ZOOM_LIB_H

#include <windows.h>
#include <magnification.h>
#include <atomic>
#include <cmath>
#include <chrono>

#pragma comment(lib, "Magnification.lib")

namespace WinRTCapture {
HWND FindRobloxWindow();
}

namespace Zoom {

typedef BOOL(WINAPI* MagInitializeFunc)();
typedef BOOL(WINAPI* MagUninitializeFunc)();
typedef BOOL(WINAPI* MagSetWindowSourceFunc)(HWND, RECT);
typedef BOOL(WINAPI* MagSetWindowTransformFunc)(HWND, MAGTRANSFORM*);
typedef BOOL(WINAPI* MagSetWindowFilterListFunc)(HWND, DWORD, int, HWND*);
typedef BOOL(WINAPI* MagSetLensUseBitmapSmoothingFunc)(HWND, BOOL);

static std::atomic<bool> g_hotkeyHeld{false};
static std::atomic<float> g_currentZoom{1.0f};
static std::atomic<float> g_targetZoom{1.0f};
static HWND g_overlayWindow = nullptr;
static HWND g_magnifierWindow = nullptr;
static HWND g_robloxWindow = nullptr;
static UINT_PTR g_updateTimer = 0;
static HMODULE g_magDll = nullptr;
static MagInitializeFunc g_MagInitialize = nullptr;
static MagUninitializeFunc g_MagUninitialize = nullptr;
static MagSetWindowSourceFunc g_MagSetWindowSource = nullptr;
static MagSetWindowTransformFunc g_MagSetWindowTransform = nullptr;
static MagSetWindowFilterListFunc g_MagSetWindowFilterList = nullptr;
static MagSetLensUseBitmapSmoothingFunc g_MagSetLensUseBitmapSmoothing = nullptr;
static HHOOK g_mouseHook = nullptr;
static USHORT g_triggerScanCode = 0;
static bool g_isAnimating = false;
static float g_startZoom = 1.0f;
static std::chrono::steady_clock::time_point g_animStart;
static POINT g_anchorPos{0, 0};
static const float MIN_ZOOM = 1.0f;
static const float MAX_ZOOM = 3.0f;
static const float SCROLL_SENSITIVITY = 0.18f;
static const int ANIM_MS = 200;

inline float EaseOutCubic(float t) {
    float inv = 1.0f - t;
    return 1.0f - (inv * inv * inv);
}

static bool InitializeMagnification() {
    if (g_magDll) return true;
    g_magDll = LoadLibraryW(L"magnification.dll");
    if (!g_magDll) return false;
    g_MagInitialize = (MagInitializeFunc)GetProcAddress(g_magDll, "MagInitialize");
    g_MagUninitialize = (MagUninitializeFunc)GetProcAddress(g_magDll, "MagUninitialize");
    g_MagSetWindowSource = (MagSetWindowSourceFunc)GetProcAddress(g_magDll, "MagSetWindowSource");
    g_MagSetWindowTransform = (MagSetWindowTransformFunc)GetProcAddress(g_magDll, "MagSetWindowTransform");
    g_MagSetWindowFilterList = (MagSetWindowFilterListFunc)GetProcAddress(g_magDll, "MagSetWindowFilterList");
    g_MagSetLensUseBitmapSmoothing = (MagSetLensUseBitmapSmoothingFunc)GetProcAddress(g_magDll, "MagSetLensUseBitmapSmoothing");
    if (!g_MagInitialize || !g_MagUninitialize || !g_MagSetWindowSource || !g_MagSetWindowTransform || !g_MagSetWindowFilterList) {
        FreeLibrary(g_magDll);
        g_magDll = nullptr;
        return false;
    }
    if (!g_MagInitialize()) {
        FreeLibrary(g_magDll);
        g_magDll = nullptr;
        return false;
    }
    return true;
}

static void SetZoomAt(float zoom, int x, int y) {
    if (!g_magnifierWindow || !g_robloxWindow || !IsWindow(g_robloxWindow)) return;
    RECT rect;
    if (!GetWindowRect(g_robloxWindow, &rect)) return;
    float anchorX = (float)(x - rect.left);
    float anchorY = (float)(y - rect.top);
    float w = (float)(rect.right - rect.left);
    float h = (float)(rect.bottom - rect.top);
    if (anchorX < 0.0f) anchorX = 0.0f;
    if (anchorY < 0.0f) anchorY = 0.0f;
    if (anchorX > w) anchorX = w;
    if (anchorY > h) anchorY = h;
    MAGTRANSFORM transform{};
    transform.v[0][0] = zoom;
    transform.v[1][1] = zoom;
    transform.v[2][2] = 1.0f;
    transform.v[0][2] = roundf((1.0f - zoom) * anchorX);
    transform.v[1][2] = roundf((1.0f - zoom) * anchorY);
    g_MagSetWindowTransform(g_magnifierWindow, &transform);
}

static void UpdateZoomAnimation() {
    if (!g_isAnimating) return;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_animStart).count();
    float target = g_targetZoom.load();
    if (elapsed >= ANIM_MS) {
        g_currentZoom.store(target);
        g_isAnimating = false;
        SetZoomAt(target, g_anchorPos.x, g_anchorPos.y);
        if (target <= MIN_ZOOM + 0.0001f && g_overlayWindow && IsWindow(g_overlayWindow)) {
            ShowWindow(g_overlayWindow, SW_HIDE);
            if (g_updateTimer) {
                KillTimer(g_overlayWindow, 1);
                g_updateTimer = 0;
            }
        }
    } else {
        float progress = EaseOutCubic((float)elapsed / (float)ANIM_MS);
        float current = g_startZoom + (target - g_startZoom) * progress;
        if (current < MIN_ZOOM) current = MIN_ZOOM;
        if (current > MAX_ZOOM) current = MAX_ZOOM;
        g_currentZoom.store(current);
        SetZoomAt(current, g_anchorPos.x, g_anchorPos.y);
    }
}

static void ZoomTo(float zoom) {
    if (zoom < MIN_ZOOM) zoom = MIN_ZOOM;
    if (zoom > MAX_ZOOM) zoom = MAX_ZOOM;
    g_targetZoom.store(zoom);
    g_startZoom = g_currentZoom.load();
    g_animStart = std::chrono::steady_clock::now();
    g_isAnimating = true;
}

static LRESULT CALLBACK OverlayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TIMER && wParam == 1) {
        if (g_isAnimating) {
            UpdateZoomAnimation();
        } else {
            SetZoomAt(g_currentZoom.load(), g_anchorPos.x, g_anchorPos.y);
        }
        return 0;
    }
    if (msg == WM_MOUSEWHEEL) {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta && g_hotkeyHeld.load()) {
            float target = g_targetZoom.load();
            float factor = powf(1.0f + SCROLL_SENSITIVITY, fabsf((float)delta) / 120.0f);
            if (delta > 0) {
                if (target < MAX_ZOOM - 0.001f) ZoomTo(target * factor);
            } else {
                if (target > MIN_ZOOM + 0.001f) ZoomTo(target / factor);
            }
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK LowLevelMouseProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0 && g_hotkeyHeld.load() && (wParam == WM_MOUSEWHEEL || wParam == 0x020E)) {
        if (g_overlayWindow && IsWindow(g_overlayWindow) && IsWindowVisible(g_overlayWindow)) {
            SetWindowPos(g_overlayWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
            AllowSetForegroundWindow(ASFW_ANY);
            SetForegroundWindow(g_overlayWindow);
            SetActiveWindow(g_overlayWindow);
            SetFocus(g_overlayWindow);
        }
        return 1;
    }
    return CallNextHookEx(g_mouseHook, code, wParam, lParam);
}

static void CreateOverlayWindow() {
    if (g_overlayWindow) return;
    if (!InitializeMagnification()) return;
    g_robloxWindow = WinRTCapture::FindRobloxWindow();
    if (!g_robloxWindow) return;
    RECT rect;
    if (!GetWindowRect(g_robloxWindow, &rect)) return;
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = OverlayWindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"ZoomVisual";
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    RegisterClassExW(&wc);
    g_overlayWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, L"ZoomVisual", L"", WS_POPUP, rect.left, rect.top, w, h, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!g_overlayWindow) return;
    SetLayeredWindowAttributes(g_overlayWindow, 0, 255, LWA_ALPHA);
    SetWindowDisplayAffinity(g_overlayWindow, 0);
    g_magnifierWindow = CreateWindowW(L"Magnifier", L"", WS_CHILD | WS_VISIBLE, 0, 0, w, h, g_overlayWindow, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!g_magnifierWindow) {
        DestroyWindow(g_overlayWindow);
        g_overlayWindow = nullptr;
        return;
    }
    if (g_MagSetWindowFilterList) {
        HWND exclude[] = {g_overlayWindow};
        g_MagSetWindowFilterList(g_magnifierWindow, 2, 1, exclude);
    }
    if (g_MagSetLensUseBitmapSmoothing) g_MagSetLensUseBitmapSmoothing(g_magnifierWindow, TRUE);
    MAGTRANSFORM transform{};
    transform.v[0][0] = 1.0f;
    transform.v[1][1] = 1.0f;
    transform.v[2][2] = 1.0f;
    g_MagSetWindowTransform(g_magnifierWindow, &transform);
    RECT src{rect.left, rect.top, rect.right, rect.bottom};
    g_MagSetWindowSource(g_magnifierWindow, src);
    ShowWindow(g_overlayWindow, SW_HIDE);
}

void Initialize() {
    InitializeMagnification();
    if (!g_mouseHook) {
        g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandleW(nullptr), 0);
    }
}

void SetTriggerScanCode(USHORT sc) {
    g_triggerScanCode = sc;
}

void HandleRawKey(USHORT makeCode, bool keyDown) {
    if (!g_triggerScanCode) return;
    if (makeCode != g_triggerScanCode) return;
    if (keyDown) {
        HWND robloxWindow = WinRTCapture::FindRobloxWindow();
        if (!robloxWindow) return;
        HWND foregroundWindow = GetForegroundWindow();
        bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
        if (!robloxInFocus) return;
    }
    bool was = g_hotkeyHeld.exchange(keyDown);
    if (keyDown && !was) {
        if (!g_overlayWindow) CreateOverlayWindow();
        if (g_overlayWindow) {
            if (g_robloxWindow && IsWindow(g_robloxWindow)) {
                RECT rect;
                if (GetWindowRect(g_robloxWindow, &rect)) {
                    int w = rect.right - rect.left;
                    int h = rect.bottom - rect.top;
                    SetWindowPos(g_overlayWindow, HWND_TOPMOST, rect.left, rect.top, w, h, SWP_SHOWWINDOW | SWP_NOACTIVATE);
                    if (g_magnifierWindow) {
                        SetWindowPos(g_magnifierWindow, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                        RECT src{rect.left, rect.top, rect.right, rect.bottom};
                        g_MagSetWindowSource(g_magnifierWindow, src);
                    }
                }
            }
            ShowWindow(g_overlayWindow, SW_SHOW);
            SetWindowPos(g_overlayWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
            AllowSetForegroundWindow(ASFW_ANY);
            SetForegroundWindow(g_overlayWindow);
            keybd_event(VK_MENU, 0, 0, 0);
            keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
            SetActiveWindow(g_overlayWindow);
            SetFocus(g_overlayWindow);
            if (!g_updateTimer) g_updateTimer = SetTimer(g_overlayWindow, 1, 16, nullptr);
            GetCursorPos(&g_anchorPos);
            ZoomTo(1.5f);
        }
    } else if (!keyDown && was) {
        ZoomTo(1.0f);
    }
}

void OnMouseWheel(short delta) {
    if (!delta) return;
    if (g_hotkeyHeld.load() && g_overlayWindow && IsWindowVisible(g_overlayWindow)) {
        float target = g_targetZoom.load();
        float factor = powf(1.0f + SCROLL_SENSITIVITY, fabsf((float)delta) / 120.0f);
        if (delta > 0) {
            if (target < MAX_ZOOM - 0.001f) ZoomTo(target * factor);
        } else {
            if (target > MIN_ZOOM + 0.001f) ZoomTo(target / factor);
        }
    }
}

bool IsHotkeyHeld() {
    return g_hotkeyHeld.load();
}

void Shutdown() {
    if (g_magnifierWindow) DestroyWindow(g_magnifierWindow);
    if (g_overlayWindow) DestroyWindow(g_overlayWindow);
    if (g_magDll) {
        if (g_MagUninitialize) g_MagUninitialize();
        FreeLibrary(g_magDll);
        g_magDll = nullptr;
    }
    if (g_mouseHook) {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = nullptr;
    }
    g_overlayWindow = nullptr;
    g_magnifierWindow = nullptr;
}

} // namespace Zoom

#endif
