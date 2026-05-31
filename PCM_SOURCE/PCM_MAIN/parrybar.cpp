#ifndef ROBLOX_PIXEL_READER_H
#define ROBLOX_PIXEL_READER_H

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
#include <fstream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <tlhelp32.h>
#include <d3d11on12.h>
#include <wrl.h>
#include <d2d1.h>
#include <dwrite.h>
#include <cmath>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

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

namespace EditHUD {
    bool IsEditorActive();
}

namespace ParryBar {

// Visual Parry Bar Constants
static const int PARRY_BAR_WIDTH = 380;
static const int PARRY_BAR_HEIGHT = 22;
static const int BAR_WIDTH = 340;
static const int BAR_HEIGHT = 10;  // thin pill

// Colors

// Global state for visual bar
HWND g_parryBarWindow = nullptr;
int g_currentPercentage = 0;
bool g_isShakyBlock = false;
bool g_shouldShowBar = false;

 

ID2D1Factory* g_d2dFactoryPB = nullptr;
ID2D1DCRenderTarget* g_dcRenderTargetPB = nullptr;
HDC g_memDCPB = nullptr;
HBITMAP g_hBitmapPB = nullptr;
int g_backingWidthPB = 0;
int g_backingHeightPB = 0;
ID2D1SolidColorBrush* g_fillGreenPB = nullptr;
ID2D1SolidColorBrush* g_fillRedPB = nullptr;
ID2D1SolidColorBrush* g_glossGreenPB = nullptr;
ID2D1SolidColorBrush* g_glossRedPB = nullptr;
ID2D1SolidColorBrush* g_outlineEdgePB = nullptr;
ID2D1SolidColorBrush* g_outlineOuterPB = nullptr;

// Position tracking for stability check
int g_lastBarX = -1;
int g_lastBarY = -1;
std::chrono::steady_clock::time_point g_positionChangeTime;
bool g_positionStable = false;

bool g_freezeValues = false;
int g_frozenPercentage = 0;
bool g_frozenShaky = false;

static std::atomic<bool> g_hasRequestedPosition{ false };
static std::atomic<int> g_requestedX{ -1 };
static std::atomic<int> g_requestedY{ -1 };
static std::atomic<bool> g_customOffsetEnabled{ false };
static std::atomic<float> g_offsetXUnits{ 0.0f };
static std::atomic<float> g_offsetYUnits{ 0.0f };
static std::atomic<float> g_userScale{ 1.0f };

// Last rendered geometry to freeze position/size during fade-out
int g_lastRenderBarX = 0;
int g_lastRenderBarY = 0;
int g_lastRenderScaledInnerWidth = 0;
int g_lastRenderScaledInnerHeight = 0;
int g_lastRenderScreenWidth = 0;
int g_lastRenderScreenHeight = 0;

// Structure to hold parry reading results
struct ParryResult {
    int percentage;
    bool isShakyBlock;
    
    ParryResult(int p, bool s) : percentage(p), isShakyBlock(s) {}
};

static std::atomic<bool> g_fadingIn{ false };
static std::atomic<bool> g_fadingOut{ false };
static std::atomic<float> g_fadeOpacity{ 1.0f };
static int g_fadeStep = 0;
static std::atomic<bool> g_gateRed{ false };
static std::atomic<bool> g_gateCheckActive{ false };
static std::atomic<bool> g_wasJustHidden{ false };
static std::atomic<bool> g_waitingForNonZeroPercentage{ false };
static int g_baseY = 0;
static bool g_lastGateRed = false;
static int g_lastRenderedPercentage = -1;
static bool g_lastRenderedShaky = false;
static float g_lastRenderedOpacity = -1.0f;
static int g_lastRenderFillWidth = 0;
static int g_lastNonZeroFillWidth = 0;
static const int FADE_IN_TOTAL_STEPS = 8;
static const int FADE_OUT_TOTAL_STEPS = 8;
static std::chrono::steady_clock::time_point g_100PercentNoShakyStartTime;
static bool g_100PercentNoShakyActive = false;
static bool g_hiddenDueTo100Percent = false;
static std::atomic<bool> g_forceHidden{ true };
static std::chrono::steady_clock::time_point g_startup100StartTime;
static bool g_startup100Active = false;
static bool g_startup100Seen = false;
static bool g_startupWaitForChange = false;
static bool g_startupBaselineShaky = false;
static bool g_pendingShow = false;

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

static inline void TryShowParryBarWindow()
{
    if (g_parryBarWindow && !g_forceHidden.load()) g_pendingShow = true;
}


float CubicBezierEase(float t) {
    if (t <= 0) return 0;
    if (t >= 1) return 1;
    
    float oneMinusT = 1 - t;
    return 1 - (oneMinusT * oneMinusT * oneMinusT * oneMinusT);
}

 

 

void EnsureD2D()
{
	if (!g_d2dFactoryPB) {
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactoryPB);
	}
	if (!g_dcRenderTargetPB && g_d2dFactoryPB) {
		D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT
		);
		g_d2dFactoryPB->CreateDCRenderTarget(&props, &g_dcRenderTargetPB);
		if (g_dcRenderTargetPB) {
			g_dcRenderTargetPB->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		}
	}
	if (g_dcRenderTargetPB && !g_fillGreenPB) {
		g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xD8D8D8), &g_fillGreenPB);
		g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xFF6464), &g_fillRedPB);
		g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xF0F0F0), &g_glossGreenPB);
		g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xFFB4B4), &g_glossRedPB);
        g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0x22222A), &g_outlineEdgePB);
        g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xBDBBC3), &g_outlineOuterPB);
	}
}

void EnsureBackingStore(int width, int height)
{
	if (!g_memDCPB) {
		g_memDCPB = CreateCompatibleDC(nullptr);
	}
	if (width != g_backingWidthPB || height != g_backingHeightPB || !g_hBitmapPB) {
		if (g_hBitmapPB) { DeleteObject(g_hBitmapPB); g_hBitmapPB = nullptr; }
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
		void* bits = nullptr;
		g_hBitmapPB = CreateDIBSection(g_memDCPB, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
		SelectObject(g_memDCPB, g_hBitmapPB);
		g_backingWidthPB = width;
		g_backingHeightPB = height;
	}
}

void RenderParryBarD2D(int screenWidth, int screenHeight, int barX, int barY, int scaledInnerWidth, int scaledInnerHeight, int fillWidth, bool drawShaky, float opacity)
{
	RECT bind = { 0, 0, screenWidth, screenHeight };
	g_dcRenderTargetPB->BindDC(g_memDCPB, &bind);
	g_dcRenderTargetPB->BeginDraw();
	g_dcRenderTargetPB->Clear(D2D1::ColorF(0, 0, 0, 0));
	float fx = static_cast<float>(barX);
	float fy = static_cast<float>(barY);
	float fwidth = static_cast<float>(scaledInnerWidth);
	float fheight = static_cast<float>(scaledInnerHeight);
	float radius = fheight * 0.5f;
	D2D1_ROUNDED_RECT backgroundRect = D2D1::RoundedRect(D2D1::RectF(static_cast<float>(barX - 1), static_cast<float>(barY - 1), static_cast<float>(barX + scaledInnerWidth + 1), static_cast<float>(barY + scaledInnerHeight + 1)), radius, radius);
	ID2D1SolidColorBrush* backgroundBrush = nullptr;
	g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0x353535, opacity), &backgroundBrush);
	g_dcRenderTargetPB->FillRoundedRectangle(backgroundRect, backgroundBrush);
	backgroundBrush->Release();
	float backgroundGlossTop = static_cast<float>(barY + static_cast<int>(1.0f * 1.0f));
	float backgroundGlossBottom = static_cast<float>(barY + (scaledInnerHeight / 2) + static_cast<int>(1.0f * 1.0f));
	float backgroundGlossRadius = (fheight - 2.0f * 1.0f) * 0.5f;
	D2D1_ROUNDED_RECT backgroundGlossRect = D2D1::RoundedRect(D2D1::RectF(static_cast<float>(barX + static_cast<int>(2.0f * 1.0f)), backgroundGlossTop, static_cast<float>(barX + scaledInnerWidth - static_cast<int>(2.0f * 1.0f)), backgroundGlossBottom), backgroundGlossRadius, backgroundGlossRadius);
	ID2D1SolidColorBrush* backgroundGlossBrush = nullptr;
	g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0x4A4A4A, opacity), &backgroundGlossBrush);
	g_dcRenderTargetPB->FillRoundedRectangle(backgroundGlossRect, backgroundGlossBrush);
	backgroundGlossBrush->Release();
	if (fillWidth > 0) {
		float fillRight = static_cast<float>(barX + (fillWidth > scaledInnerWidth ? scaledInnerWidth : fillWidth));
		float rx = (fheight + 2.0f) * 0.5f;
		D2D1_ROUNDED_RECT fillRect = D2D1::RoundedRect(D2D1::RectF(static_cast<float>(barX - 1), static_cast<float>(barY - 1), fillRight + 1.0f, static_cast<float>(barY + scaledInnerHeight + 1)), rx, rx);
		// Create brushes with opacity
		ID2D1SolidColorBrush* fillBrush = nullptr;
		if (drawShaky) {
			g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xFF6464, opacity), &fillBrush);
		} else {
			g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xD8D8D8, opacity), &fillBrush);
		}
		g_dcRenderTargetPB->FillRoundedRectangle(fillRect, fillBrush);
		fillBrush->Release();
		int glossWidth = fillWidth - static_cast<int>(4.0f * 1.0f);
		if (glossWidth > 0) {
			float glossLeft = static_cast<float>(barX + static_cast<int>(2.0f * 1.0f));
			float glossTop = static_cast<float>(barY + static_cast<int>(1.0f * 1.0f));
			float glossRight = glossLeft + static_cast<float>(glossWidth);
			float glossBottom = static_cast<float>(barY + (scaledInnerHeight / 2) + static_cast<int>(1.0f * 1.0f));
			float gr = (fheight - 2.0f * 1.0f) * 0.5f;
			if (glossRight > static_cast<float>(barX + scaledInnerWidth)) glossRight = static_cast<float>(barX + scaledInnerWidth);
			D2D1_ROUNDED_RECT glossRect = D2D1::RoundedRect(D2D1::RectF(glossLeft, glossTop, glossRight, glossBottom), gr, gr);
			// Create gloss brush with opacity
			ID2D1SolidColorBrush* glossBrush = nullptr;
			if (drawShaky) {
				g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xFFB4B4, opacity), &glossBrush);
			} else {
				g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xF0F0F0, opacity), &glossBrush);
			}
			g_dcRenderTargetPB->FillRoundedRectangle(glossRect, glossBrush);
			glossBrush->Release();
		}
	}
	D2D1_ROUNDED_RECT innerRect = D2D1::RoundedRect(D2D1::RectF(fx, fy, fx + fwidth, fy + fheight), radius, radius);
	ID2D1SolidColorBrush* outlineEdgeBrush = nullptr;
	g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0x22222A, opacity), &outlineEdgeBrush);
	g_dcRenderTargetPB->DrawRoundedRectangle(innerRect, outlineEdgeBrush, 1.0f);
	outlineEdgeBrush->Release();
	float orx = (fheight + 2.0f) * 0.5f;
	D2D1_ROUNDED_RECT outerRect = D2D1::RoundedRect(D2D1::RectF(static_cast<float>(barX - 1), static_cast<float>(barY - 1), static_cast<float>(barX + scaledInnerWidth + 1), static_cast<float>(barY + scaledInnerHeight + 1)), orx, orx);
	ID2D1SolidColorBrush* outlineOuterBrush = nullptr;
	g_dcRenderTargetPB->CreateSolidColorBrush(D2D1::ColorF(0xBDBBC3, opacity), &outlineOuterBrush);
    g_dcRenderTargetPB->DrawRoundedRectangle(outerRect, outlineOuterBrush, 1.0f);
	outlineOuterBrush->Release();
    
    g_dcRenderTargetPB->EndDraw();
}

void UpdateFadeAnimation() {
    if (g_fadingIn) {
        // If we're waiting for non-zero percentage, stay at 0 opacity
        if (g_waitingForNonZeroPercentage.load()) {
            g_fadeOpacity = 0.0f;
            return;
        }
        
        g_fadeStep++;
        if (g_fadeStep >= FADE_IN_TOTAL_STEPS) {
            g_fadeOpacity = 1.0f;
            g_fadingIn = false;
            g_fadeStep = 0;
            g_pendingShow = false;
        } else {
            float progress = static_cast<float>(g_fadeStep) / static_cast<float>(FADE_IN_TOTAL_STEPS);
            float easedProgress = CubicBezierEase(progress);
            g_fadeOpacity = easedProgress;
        }
    } else if (g_fadingOut) {
        g_fadeStep++;
        if (g_fadeStep >= FADE_OUT_TOTAL_STEPS) {
            g_fadeOpacity = 0.0f;
            g_fadingOut = false;
            g_fadeStep = 0;
            g_pendingShow = false;
            if (g_parryBarWindow) {
                ShowWindow(g_parryBarWindow, SW_HIDE);
            }
        } else {
            float progress = static_cast<float>(g_fadeStep) / static_cast<float>(FADE_OUT_TOTAL_STEPS);
            float easedProgress = CubicBezierEase(progress);
            g_fadeOpacity = 1.0f - easedProgress;
        }
    }
}

struct PixelReaderApp
{
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;
};

static PixelReaderApp g_app = {};
static std::thread g_readerThread;
static bool g_parryRunning = false;

bool InitializeDirectX()
{
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

struct PixelColor {
    uint8_t r, g, b, a;
};

bool IsGrayColor(uint32_t color, int tolerance = 20) {
    uint8_t red = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8) & 0xFF;
    uint8_t blue = color & 0xFF;
    
    return (abs(red - green) <= tolerance) && (abs(red - blue) <= tolerance) && (abs(green - blue) <= tolerance);
}

bool IsRedColor(uint32_t color, int minRed = 50, int redDelta = 10, int minAlpha = 0x00, bool requireOpaque = false) {
    uint8_t red = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8) & 0xFF;
    uint8_t blue = color & 0xFF;
    uint8_t alpha = (color >> 24) & 0xFF;
    if (requireOpaque && alpha < minAlpha) return false;
    return (red > minRed && red > green + redDelta && red > blue + redDelta);
}


ParryResult ReadParryPercentage() {
    if (!WinRTCapture::IsCaptureInitialized()) {
        return ParryResult(-1, false);
    }
    
    int clientWidth = WinRTCapture::GetCaptureWidth();
    int clientHeight = WinRTCapture::GetCaptureHeight();
    if (clientWidth <= 0 || clientHeight <= 0) {
        return ParryResult(-1, false);
    }
    
    const int baseStartX = 730;
    const int baseEndX = 1187;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    bool isFullscreen = (clientWidth == screenWidth && clientHeight == screenHeight);
    
    int startX = 0;
    int endX = 0;
    int y = 0;
    
    int capWidth = clientWidth;
    int capHeight = clientHeight;

    try {
        // Calibration: check if we need to determine baseY or re-run on gate false->true
        bool gateFalseToTrue = (!g_lastGateRed && g_gateRed.load());
        if (g_baseY == 0 || gateFalseToTrue) {
            int testY866 = 866 - (1080 - clientHeight);
            int rightmostX = endX >= capWidth ? capWidth - 1 : endX;
            auto isNonGrayAtRightmost = [&](int testY) -> bool {
                if (testY < 0 || testY >= capHeight || rightmostX < 0) return false;
                uint32_t color = WinRTCapture::GetPixelColor(rightmostX, testY);
                return !IsGrayColor(color);
            };

            if (isNonGrayAtRightmost(testY866 + 6)) {
                g_baseY = testY866 + 6;
            } else {
                g_baseY = 866;
            }
        }
        
        // Calculate y from calibrated g_baseY
        y = g_baseY - (1080 - clientHeight);

        // Calculate startX and endX
        startX = (baseStartX * clientWidth) / 1920;
        endX = (baseEndX * clientWidth) / 1920;

        bool gateRedOk = g_gateCheckActive.load();

        if (startX < 0) startX = 0;
        if (endX >= clientWidth) endX = clientWidth - 1;
        if (endX < startX) endX = startX;
        if (y < 3) y = 3;
        if (y >= clientHeight) y = clientHeight - 1;
        
        const int redCheckY = 3;
        
        D3D11_BOX srcBox = {};
        srcBox.left = startX;
        srcBox.top = y - redCheckY;
        srcBox.front = 0;
        srcBox.right = endX + 1;
        srcBox.bottom = y + 1;
        srcBox.back = 1;

        int desiredWidth = (endX - startX + 1);
        int desiredHeight = 4;
        if (desiredWidth <= 0) desiredWidth = 1;
        if (desiredHeight <= 0) desiredHeight = 1;

        

            int totalLength = endX - startX;
            int rightmostPosition = startX;
            
        for (int x = endX; x >= startX; --x) {
                int nonGray = 0;
                for (int row = 0; row < 4; ++row) {
                int checkY = y - redCheckY + row;
                if (checkY >= 0 && checkY < capHeight) {
                    uint32_t color = WinRTCapture::GetPixelColor(x, checkY);
                    if (!IsGrayColor(color)) { nonGray = 1; break; }
                }
            }
            if (nonGray) { rightmostPosition = x; break; }
            }
            
            bool shakyBlock = false;
            if (rightmostPosition > 0) {
                int redCheckRow = 0;
            int checkY = y - redCheckY + redCheckRow;
            if (checkY >= 0 && checkY < capHeight) {
                uint32_t redColor = WinRTCapture::GetPixelColor(rightmostPosition, checkY);
                    shakyBlock = IsRedColor(redColor);
                }
            }
            
        if (true) {
            
            int progress = rightmostPosition - startX;
            int rawPercent = (totalLength > 0) ? (progress * 100) / totalLength : 0;
            if (rawPercent < 0) rawPercent = 0;
            if (rawPercent > 100) rawPercent = 100;

            // Time-matched smoothing with continuous adaptation (segment-based)
            using Clock = std::chrono::steady_clock;
            static bool smoothInit = false;
            static float smoothedPercent = 100.0f;
            static bool rampFromZero = false;
            static bool lastRawAboveThreshold = true;
            static Clock::time_point lastUpdate = Clock::now();
            // Cycle tracking
            static bool cycleActive = false;
            static Clock::time_point cycleStartTime = Clock::now();
            // Segment timing for adaptive estimate
            static int lastRawSamplePercent = 0;
            static Clock::time_point lastRawSampleTime = Clock::now();
            static float ewmaMsPerPercent = 0.0f;
            static int lastDisplayPercentage = 0;
            static bool finalEaseActive = false;
            static Clock::time_point finalEaseStart = Clock::now();
            static int finalEaseFrom = 0;
            static int nextDecileTarget = 10;
            static Clock::time_point decileStartTime = Clock::now();
            static float finalEaseDurationMs = 0.0f;
            static std::vector<float> decileDurationsMs;

            auto now = Clock::now();
            float dtMs = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate).count());
            if (dtMs <= 0.0f) dtMs = 16.0f;
            if (dtMs > 100.0f) dtMs = 100.0f;

            if (!smoothInit) {
                smoothedPercent = static_cast<float>(rawPercent);
                lastUpdate = now;
                smoothInit = true;
            }

            if (rawPercent == 0) {
                smoothedPercent = 0.0f;
                cycleActive = false;
                ewmaMsPerPercent = 0.0f;
                rampFromZero = false;
                lastDisplayPercentage = 0;
                finalEaseActive = false;
                finalEaseFrom = 0;
                nextDecileTarget = 10;
                decileStartTime = now;
                decileDurationsMs.clear();
                finalEaseDurationMs = 0.0f;
            }
            if (rawPercent >= 100) {
                cycleActive = false;
                lastDisplayPercentage = 100;
                finalEaseActive = false;
                nextDecileTarget = 10;
                decileDurationsMs.clear();
                finalEaseDurationMs = 0.0f;
                finalEaseFrom = 0;
            }
            if (rawPercent <= 5) {
                lastRawAboveThreshold = false;
            } else if (!lastRawAboveThreshold) {
                lastRawAboveThreshold = true;
                rampFromZero = true;
                smoothedPercent = 0.0f;
            }

            // After resets and state updates, if difference is huge, snap to raw
            {
                int ahead = rawPercent - static_cast<int>(smoothedPercent);
                if (ahead >= 35) {
                    smoothedPercent = static_cast<float>(rawPercent);
                }
            }

            // Arm the cycle and initialize segment tracking on first nonzero
            if (!cycleActive && rawPercent > 0) {
                cycleActive = true;
                cycleStartTime = now;
                lastRawSamplePercent = 0;
                lastRawSampleTime = cycleStartTime;
                ewmaMsPerPercent = 0.0f;
                nextDecileTarget = 10;
                decileStartTime = now;
                decileDurationsMs.clear();
                finalEaseActive = false;
                finalEaseFrom = 0;
                finalEaseDurationMs = 0.0f;
            }

            // Update EWMA ms/percent whenever raw advances
            if (cycleActive && rawPercent > lastRawSamplePercent) {
                int deltaPct = rawPercent - lastRawSamplePercent;
                float segMs = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRawSampleTime).count());
                if (deltaPct > 0 && segMs > 0.0f) {
                    float msPerPct = segMs / static_cast<float>(deltaPct);
                    float alpha = 0.2f; // blend weight for adaptation
                    if (ewmaMsPerPercent <= 0.0f) ewmaMsPerPercent = msPerPct;
                    else ewmaMsPerPercent = (1.0f - alpha) * ewmaMsPerPercent + alpha * msPerPct;
                }
                lastRawSamplePercent = rawPercent;
                lastRawSampleTime = now;
            }
            // Bootstrap: if raw has left zero but no EWMA yet, initialize it from the first delta
            if (cycleActive && ewmaMsPerPercent <= 0.0f && rawPercent > 0) {
                float estimate = dtMs / static_cast<float>(rawPercent); // ms per % from first jump
                if (estimate < 1.0f) estimate = 1.0f;
                if (estimate > 200.0f) estimate = 200.0f;
                ewmaMsPerPercent = estimate;
                lastRawSamplePercent = rawPercent;
                lastRawSampleTime = now;
            }

            if (rawPercent < static_cast<int>(smoothedPercent)) {
                // Monotonic: do not decrease mid-cycle
                // (reset at raw==0 handled above)
            } else {
                // Drive smoothed by schedule derived from current ewmaMsPerPercent,
                // and also ensure catch-up to finish exactly when raw is predicted to finish
                if (cycleActive && ewmaMsPerPercent > 0.0f) {
                    float elapsedMs = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(now - cycleStartTime).count());
                    if (elapsedMs < 0.0f) elapsedMs = 0.0f;
                    float scheduledPercent = elapsedMs / ewmaMsPerPercent;
                    if (scheduledPercent < 0.0f) scheduledPercent = 0.0f;
                    if (scheduledPercent > 100.0f) scheduledPercent = 100.0f;

                    // Allow a small lead over raw to avoid visible stalls
                    const float leadMargin = 3.0f; // percent
                    float maxAhead = static_cast<float>(rawPercent) + leadMargin;
                    if (scheduledPercent > maxAhead) scheduledPercent = maxAhead;

                    float target = (static_cast<float>(rawPercent) < scheduledPercent) ? static_cast<float>(rawPercent) : scheduledPercent;
                    // Critically damped spring (SmoothDamp) toward target with time-to-go control
                    static float smoothVel = 0.0f;
                    float rawRemainMs = (100.0f - static_cast<float>(rawPercent)) * ewmaMsPerPercent;
                    if (rawRemainMs < 1.0f) rawRemainMs = 1.0f;
                    float smoothTimeSec = (rawRemainMs / 1000.0f) * 0.33f; // finish in ~1/3 of remaining time, adaptive
                    if (smoothTimeSec < 0.05f) smoothTimeSec = 0.05f;
                    float dtSec = dtMs / 1000.0f;
                    float omega = 2.0f / smoothTimeSec;
                    float x = omega * dtSec;
                    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
                    float change = smoothedPercent - target;
                    float temp = (smoothVel + omega * change) * dtSec;
                    smoothVel = (smoothVel - omega * temp) * exp;
                    float newValue = target + (change + temp) * exp;

                    // Monotonic clamp and never exceed bounds
                    if (newValue < smoothedPercent) newValue = smoothedPercent;
                    if (newValue > target) newValue = target;
                    if (newValue > static_cast<float>(rawPercent) + leadMargin) newValue = static_cast<float>(rawPercent) + leadMargin;

                    // Minimum forward step to avoid freezing when target is slightly ahead
                    float minStep = 0.2f * (dtMs / 16.0f); // ~0.2% per 16ms tick
                    float maxStep = 1.0f * (dtMs / 16.0f); // cap ~1% per 16ms tick
                    float desired = newValue - smoothedPercent;
                    if (desired > 0.0f && desired < minStep) newValue = smoothedPercent + minStep;
                    if (newValue - smoothedPercent > maxStep) newValue = smoothedPercent + maxStep;

                    if (newValue < 0.0f) newValue = 0.0f;
                    if (newValue > 100.0f) newValue = 100.0f;

                    smoothedPercent = newValue;

                    // Time alignment: if smoothed progress suggests it would finish later, add a boost
                    float smoothRemainPct = 100.0f - smoothedPercent;
                    if (rawRemainMs > 1.0f && smoothRemainPct > 0.0f) {
                        float requiredPerMs = smoothRemainPct / rawRemainMs; // % needed per ms to finish with raw
                        float boost = requiredPerMs * dtMs; // additional % this tick
                        // Do not overtake raw+margin in this tick
                        float gapToCap = (static_cast<float>(rawPercent) + leadMargin) - smoothedPercent;
                        if (boost > gapToCap) boost = gapToCap;
                        if (boost > 0.0f) smoothedPercent += boost;
                    }
                } else if (cycleActive && ewmaMsPerPercent <= 0.0f) {
                    // If cycle active but no timing yet, nudge forward softly under raw+margin
                    const float leadMargin = 3.0f;
                    float cap = static_cast<float>(rawPercent) + leadMargin;
                    float minStep = 0.2f * (dtMs / 16.0f);
                    float maxStep = 1.0f * (dtMs / 16.0f);
                    float step = (minStep < maxStep) ? minStep : maxStep;
                    float next = smoothedPercent + step;
                    if (next > cap) next = cap;
                    if (next > 100.0f) next = 100.0f;
                    if (next > smoothedPercent) smoothedPercent = next;
                }
                if (rampFromZero && smoothedPercent >= static_cast<float>(rawPercent)) rampFromZero = false;
            }

            if (smoothedPercent < 0.0f) smoothedPercent = 0.0f;
            if (smoothedPercent > 100.0f) smoothedPercent = 100.0f;

            lastUpdate = now;

            int percentage = static_cast<int>(smoothedPercent + 0.5f);
            if (rawPercent > percentage + 35) {
                smoothedPercent = static_cast<float>(rawPercent);
                percentage = rawPercent;
            }
            if (!finalEaseActive) {
                while (percentage >= nextDecileTarget && nextDecileTarget <= 90) {
                    float segMs = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(now - decileStartTime).count());
                    if (segMs < 0.0f) segMs = 0.0f;
                    decileDurationsMs.push_back(segMs);
                    decileStartTime = now;
                    nextDecileTarget += 10;
                }
            }
            if (!finalEaseActive) {
                if (percentage >= 90 && percentage < 100) {
                    finalEaseActive = true;
                    finalEaseStart = now;
                    finalEaseFrom = lastDisplayPercentage > 0 ? lastDisplayPercentage : percentage;
                    if (finalEaseFrom < 90) finalEaseFrom = 90;
                    if (!decileDurationsMs.empty()) {
                        float sum = 0.0f;
                        for (float v : decileDurationsMs) sum += v;
                        float avg = sum / static_cast<float>(decileDurationsMs.size());
                        finalEaseDurationMs = avg;
                    } else if (ewmaMsPerPercent > 0.0f) {
                        finalEaseDurationMs = ewmaMsPerPercent * 10.0f;
                    } else {
                        finalEaseDurationMs = 144.0f;
                    }
                }
            }
            if (finalEaseActive) {
                float elapsedMs = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(now - finalEaseStart).count());
                float denom = finalEaseDurationMs;
                if (denom < 1.0f) denom = 1.0f;
                float t = elapsedMs / denom;
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                float eased = CubicBezierEase(t);
                int easedPerc = finalEaseFrom + static_cast<int>(((100.0f - static_cast<float>(finalEaseFrom)) * eased) + 0.5f);
                if (easedPerc > 100) easedPerc = 100;
                if (easedPerc < percentage) easedPerc = percentage;
                if (easedPerc < lastDisplayPercentage) easedPerc = lastDisplayPercentage;
                percentage = easedPerc;
                if (t >= 1.0f || percentage >= 100) {
                    percentage = 100;
                    finalEaseActive = false;
                }
            }
            if (percentage < lastDisplayPercentage) {
                percentage = lastDisplayPercentage;
            }
            if (percentage > lastDisplayPercentage) {
                lastDisplayPercentage = percentage;
            }
            
            
            printf("ShakyBlock: %s\n", shakyBlock ? "True" : "False");
            
            // Always update values regardless of UI state
            if (!g_freezeValues) {
                g_currentPercentage = percentage;
                g_isShakyBlock = shakyBlock;
            }

            // Continuous stuck-percentage detection: if percentage remains unchanged (and not 100%)
            // for more than 440ms, toggle the scan Y between 866 and 872.
            {
                static int s_lastPercentageObserved = -1;
                static std::chrono::steady_clock::time_point s_lastChangeTime = std::chrono::steady_clock::now();
                auto nowStuck = std::chrono::steady_clock::now();

                if (percentage != s_lastPercentageObserved) {
                    s_lastPercentageObserved = percentage;
                    s_lastChangeTime = nowStuck;
                } else {
                    if (percentage != 100) {
                        long long stuckMs = std::chrono::duration_cast<std::chrono::milliseconds>(nowStuck - s_lastChangeTime).count();
                        if (stuckMs > 440) {
                            if (g_baseY == 866 + 6) g_baseY = 866; else if (g_baseY == 866) g_baseY = 866 + 6;
                            s_lastChangeTime = nowStuck;
                            s_lastPercentageObserved = -1; // force re-evaluation after switch
                        }
                    } else {
                        // At 100% we do not consider it stuck; reset timer
                        s_lastChangeTime = nowStuck;
                    }
                }
            }

            // Gate check: only update gate state; visibility handled elsewhere
            g_lastGateRed = g_gateRed.load();
            if (gateRedOk) {
                g_shouldShowBar = true;
                g_gateCheckActive = true;
                
                // Check if we were just hidden and percentage is not 100%
                if (g_wasJustHidden.load() && percentage != 100) {
                    g_waitingForNonZeroPercentage = true;
                    g_gateCheckActive = false; // Don't show yet
                } else if (g_waitingForNonZeroPercentage.load() && percentage == 100) {
                    g_waitingForNonZeroPercentage = false;
                    g_wasJustHidden = false;
                    g_gateCheckActive = true; // Now we can show
                } else if (percentage == 100) {
                    g_waitingForNonZeroPercentage = false;
                    g_wasJustHidden = false;
                }
                
                // If gate was false and now true, fade in
                if (g_gateRed.load() == false && !g_fadingIn && !g_fadingOut && !g_waitingForNonZeroPercentage.load()) {
                    g_fadingIn = true;
                    g_fadeStep = 0;
                    g_fadeOpacity = 0.0f;
                    TryShowParryBarWindow();
                }
                g_gateRed = true;
            } else {
                g_shouldShowBar = false;
                g_gateCheckActive = false;
                g_wasJustHidden = true; // Mark that we were just hidden
                
                // If gate was true and now false, fade out
                if (g_gateRed.load() == true && !g_fadingIn && !g_fadingOut) {
                    g_fadingOut = true;
                    g_fadeStep = 0;
                    g_fadeOpacity = 1.0f;
                }
                g_gateRed = false;
            }
            
            if (g_parryBarWindow) {
                HDC hdc = GetDC(g_parryBarWindow);
                if (hdc) {
                    if (!WinRTCapture::IsCaptureInitialized()) {
                        ReleaseDC(g_parryBarWindow, hdc);
                        // Still update values even if capture not initialized
                        if (!g_freezeValues) {
                            g_currentPercentage = -1;
                            g_isShakyBlock = false;
                        }
                        return ParryResult(-1, false);
                    }
                    
                    int currentWindowWidth = WinRTCapture::GetCaptureWidth();
                    int currentWindowHeight = WinRTCapture::GetCaptureHeight();
                    if (currentWindowWidth <= 0 || currentWindowHeight <= 0) {
                        ReleaseDC(g_parryBarWindow, hdc);
                        return ParryResult(-1, false);
                    }
                    
                    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                    float scaleX = (float)currentWindowWidth / (float)screenWidth;
                    float scaleY = (float)currentWindowHeight / (float)screenHeight;
                    float baseScale = (scaleX < scaleY) ? scaleX : scaleY;
                    if (baseScale < 0.6f) baseScale = 0.6f;
                    float user = clampf(g_userScale.load(), 0.6f, 2.0f);
                    float scale = baseScale * user;
                    float maxScale = baseScale * 2.0f;
                    if (scale < 0.6f) scale = 0.6f;
                    if (scale > maxScale) scale = maxScale;
                    
                    int scaledBarWidth = (int)(PARRY_BAR_WIDTH * scale);
                    int scaledBarHeight = (int)(PARRY_BAR_HEIGHT * scale);
                    int scaledInnerWidth = (int)(BAR_WIDTH * scale);
                    int scaledInnerHeight = (int)(BAR_HEIGHT * scale);
                    
                    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                    if (!robloxWindow) {
                        ReleaseDC(g_parryBarWindow, hdc);
                        return ParryResult(-1, false);
                    }
                    
                    RECT clientRect;
                    GetClientRect(robloxWindow, &clientRect);
                    POINT clientTopLeft = {clientRect.left, clientRect.top};
                    ClientToScreen(robloxWindow, &clientTopLeft);
                    
                    int robloxCenterX = clientTopLeft.x + currentWindowWidth / 2;
                    int robloxTopY = clientTopLeft.y + currentWindowHeight / 4;
                    
                    int baseX = robloxCenterX - scaledInnerWidth / 2;
                    int baseY = robloxTopY;
                    
                    if (g_hasRequestedPosition.exchange(false)) {
                        int rx = g_requestedX.load();
                        int ry = g_requestedY.load();
                        if (rx >= 0 && ry >= 0) {
                            int robloxW = clientRect.right - clientRect.left;
                            int robloxH = clientRect.bottom - clientRect.top;
                            int availW = robloxW - scaledInnerWidth;
                            int availH = robloxH - scaledInnerHeight;
                            float nx = 0.0f;
                            float ny = 0.0f;
                            if (availW > 0) nx = (float)(rx - clientTopLeft.x) / (float)availW;
                            if (availH > 0) ny = (float)(ry - clientTopLeft.y) / (float)availH;
                            g_offsetXUnits = clampf(nx, 0.0f, 1.0f);
                            g_offsetYUnits = clampf(ny, 0.0f, 1.0f);
                            g_customOffsetEnabled = true;
                        }
                    }
                    
                    int barX = baseX;
                    int barY = baseY;
                    if (g_customOffsetEnabled.load()) {
                        float oxu = g_offsetXUnits.load();
                        float oyu = g_offsetYUnits.load();
                        int robloxW = clientRect.right - clientRect.left;
                        int robloxH = clientRect.bottom - clientRect.top;
                        int availW = robloxW - scaledInnerWidth;
                        int availH = robloxH - scaledInnerHeight;
                        barX = clientTopLeft.x + (availW > 0 ? (int)lroundf(clampf(oxu, 0.0f, 1.0f) * (float)availW) : 0);
                        barY = clientTopLeft.y + (availH > 0 ? (int)lroundf(clampf(oyu, 0.0f, 1.0f) * (float)availH) : 0);
                    }
                    int robloxW = clientRect.right - clientRect.left;
                    int robloxH = clientRect.bottom - clientRect.top;
                    int minX = clientTopLeft.x;
                    int minY = clientTopLeft.y;
                    int maxX = clientTopLeft.x + robloxW - scaledInnerWidth;
                    int maxY = clientTopLeft.y + robloxH - scaledInnerHeight;
                    if (maxX < minX) maxX = minX;
                    if (maxY < minY) maxY = minY;
                    barX = clampi(barX, minX, maxX);
                    barY = clampi(barY, minY, maxY);

                    bool editorActive = EditHUD::IsEditorActive();
                    if (editorActive) {
                        g_fadingIn = false;
                        g_fadingOut = false;
                        g_fadeOpacity = 1.0f;
                        g_fadeStep = 0;
                    }

                    int renderScreenW = screenWidth;
                    int renderScreenH = screenHeight;
                    int renderInnerW = scaledInnerWidth;
                    int renderInnerH = scaledInnerHeight;
                    int renderBarX = barX;
                    int renderBarY = barY;

                    if (g_fadingOut && !editorActive) {
                        renderScreenW = g_lastRenderScreenWidth > 0 ? g_lastRenderScreenWidth : screenWidth;
                        renderScreenH = g_lastRenderScreenHeight > 0 ? g_lastRenderScreenHeight : screenHeight;
                        renderInnerW = g_lastRenderScaledInnerWidth > 0 ? g_lastRenderScaledInnerWidth : scaledInnerWidth;
                        renderInnerH = g_lastRenderScaledInnerHeight > 0 ? g_lastRenderScaledInnerHeight : scaledInnerHeight;
                        renderBarX = g_lastRenderBarX > 0 ? g_lastRenderBarX : barX;
                        renderBarY = g_lastRenderBarY > 0 ? g_lastRenderBarY : barY;
                    }
                    
                    EnsureD2D();
                    EnsureBackingStore(renderScreenW, renderScreenH);

                    int currentPercentage = editorActive ? 100 : (g_freezeValues ? g_frozenPercentage : percentage);
                    if (currentPercentage < 0) currentPercentage = 0;
                    if (currentPercentage > 100) currentPercentage = 100;
                    
                    int fillWidth = (int)((renderInnerW * currentPercentage) / 100.0f);
                    if (g_fadingOut) {
                        if (fillWidth == 0) fillWidth = g_lastNonZeroFillWidth;
                    } else {
                        g_lastRenderFillWidth = fillWidth;
                        if (fillWidth > 0) g_lastNonZeroFillWidth = fillWidth;
                    }
                    bool drawShaky = editorActive ? false : (g_freezeValues ? g_frozenShaky : shakyBlock);
                    float currentOpacity = editorActive ? 1.0f : g_fadeOpacity.load();

                    int prevRenderBarX = g_lastRenderBarX;
                    int prevRenderBarY = g_lastRenderBarY;
                    int prevRenderInnerW = g_lastRenderScaledInnerWidth;
                    int prevRenderInnerH = g_lastRenderScaledInnerHeight;
                    int prevRenderScreenW = g_lastRenderScreenWidth;
                    int prevRenderScreenH = g_lastRenderScreenHeight;

                    bool needsRender = (currentPercentage != g_lastRenderedPercentage ||
                                       drawShaky != g_lastRenderedShaky ||
                                       currentOpacity != g_lastRenderedOpacity ||
                                       fillWidth != g_lastRenderFillWidth ||
                                       renderBarX != prevRenderBarX ||
                                       renderBarY != prevRenderBarY ||
                                       renderInnerW != prevRenderInnerW ||
                                       renderInnerH != prevRenderInnerH ||
                                       renderScreenW != prevRenderScreenW ||
                                       renderScreenH != prevRenderScreenH);

                    if (needsRender) {
                        RenderParryBarD2D(renderScreenW, renderScreenH, renderBarX, renderBarY, renderInnerW, renderInnerH, fillWidth, drawShaky, currentOpacity);

                        HDC hdcScreenULW = GetDC(nullptr);
                        POINT ptSrc = { 0, 0 };
                        POINT ptDst = { 0, 0 };
                        SIZE size = { renderScreenW, renderScreenH };
                        BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                        UpdateLayeredWindow(g_parryBarWindow, hdcScreenULW, &ptDst, &size, g_memDCPB, &ptSrc, 0, &blend, ULW_ALPHA);
                        ReleaseDC(nullptr, hdcScreenULW);

                        if (g_pendingShow) {
                            ShowWindow(g_parryBarWindow, SW_SHOW);
                            g_pendingShow = false;
                        }

                        g_lastRenderedPercentage = currentPercentage;
                        g_lastRenderedShaky = drawShaky;
                        g_lastRenderedOpacity = currentOpacity;
                        if (!g_fadingOut) {
                            g_lastRenderFillWidth = fillWidth;
                        }

                        if (!g_fadingOut || editorActive) {
                            g_lastRenderScreenWidth = renderScreenW;
                            g_lastRenderScreenHeight = renderScreenH;
                            g_lastRenderScaledInnerWidth = renderInnerW;
                            g_lastRenderScaledInnerHeight = renderInnerH;
                            g_lastRenderBarX = renderBarX;
                            g_lastRenderBarY = renderBarY;
                        }
                    }
                    
                    ReleaseDC(g_parryBarWindow, hdc);
                }
            }
            
            return ParryResult(percentage, shakyBlock);
        } else {
            return ParryResult(-1, false);
        }
        
    } catch (const std::exception& e) {
        return ParryResult(-1, false);
    } catch (...) {
        return ParryResult(-1, false);
    }
}




void UpdateParryBarPosition()
{
    if (!g_parryBarWindow) return;
    bool editorActive = EditHUD::IsEditorActive();
    if (editorActive) {
        g_positionStable = true;
        g_fadingIn = false;
        g_fadingOut = false;
        g_fadeOpacity = 1.0f;
        g_fadeStep = 0;
        g_hiddenDueTo100Percent = false;
        g_100PercentNoShakyActive = false;
        g_waitingForNonZeroPercentage = false;
    }
    
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (robloxWindow) {
        if (!WinRTCapture::IsCaptureInitialized()) return;
        
        int windowWidth = WinRTCapture::GetCaptureWidth();
        int windowHeight = WinRTCapture::GetCaptureHeight();
        if (windowWidth <= 0 || windowHeight <= 0) return;
        
        RECT clientRect;
        GetClientRect(robloxWindow, &clientRect);
        POINT clientTopLeft = {clientRect.left, clientRect.top};
        ClientToScreen(robloxWindow, &clientTopLeft);
        
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        float scaleX = (float)windowWidth / (float)screenWidth;
        float scaleY = (float)windowHeight / (float)screenHeight;
        float baseScale = (scaleX < scaleY) ? scaleX : scaleY;
        if (baseScale < 0.6f) baseScale = 0.6f;
        float user = clampf(g_userScale.load(), 0.6f, 2.0f);
        float scale = baseScale * user;
        float maxScale = baseScale * 2.0f;
        if (scale < 0.6f) scale = 0.6f;
        if (scale > maxScale) scale = maxScale;
        
        screenWidth = GetSystemMetrics(SM_CXSCREEN);
        screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        int scaledInnerWidth = (int)(BAR_WIDTH * scale);
        int scaledInnerHeight = (int)(BAR_HEIGHT * scale);
        
        int robloxCenterX = clientTopLeft.x + windowWidth / 2;
        int robloxTopY = clientTopLeft.y + windowHeight / 4;
        
        int baseX = robloxCenterX - scaledInnerWidth / 2;
        int baseY = robloxTopY;
        
        if (g_hasRequestedPosition.exchange(false)) {
            int rx = g_requestedX.load();
            int ry = g_requestedY.load();
            if (rx >= 0 && ry >= 0) {
                int robloxW = clientRect.right - clientRect.left;
                int robloxH = clientRect.bottom - clientRect.top;
                int availW = robloxW - scaledInnerWidth;
                int availH = robloxH - scaledInnerHeight;
                float nx = 0.0f;
                float ny = 0.0f;
                if (availW > 0) nx = (float)(rx - clientTopLeft.x) / (float)availW;
                if (availH > 0) ny = (float)(ry - clientTopLeft.y) / (float)availH;
                g_offsetXUnits = clampf(nx, 0.0f, 1.0f);
                g_offsetYUnits = clampf(ny, 0.0f, 1.0f);
                g_customOffsetEnabled = true;
            }
        }
        
        int barX = baseX;
        int barY = baseY;
        if (g_customOffsetEnabled.load()) {
            float oxu = g_offsetXUnits.load();
            float oyu = g_offsetYUnits.load();
            int robloxW = clientRect.right - clientRect.left;
            int robloxH = clientRect.bottom - clientRect.top;
            int availW = robloxW - scaledInnerWidth;
            int availH = robloxH - scaledInnerHeight;
            barX = clientTopLeft.x + (availW > 0 ? (int)lroundf(clampf(oxu, 0.0f, 1.0f) * (float)availW) : 0);
            barY = clientTopLeft.y + (availH > 0 ? (int)lroundf(clampf(oyu, 0.0f, 1.0f) * (float)availH) : 0);
        }
        int robloxW = clientRect.right - clientRect.left;
        int robloxH = clientRect.bottom - clientRect.top;
        int minX = clientTopLeft.x;
        int minY = clientTopLeft.y;
        int maxX = clientTopLeft.x + robloxW - scaledInnerWidth;
        int maxY = clientTopLeft.y + robloxH - scaledInnerHeight;
        if (maxX < minX) maxX = minX;
        if (maxY < minY) maxY = minY;
        barX = clampi(barX, minX, maxX);
        barY = clampi(barY, minY, maxY);
        
        if (editorActive) {
            g_lastBarX = barX;
            g_lastBarY = barY;
            g_positionChangeTime = std::chrono::steady_clock::now();
            g_positionStable = true;
            g_freezeValues = false;
        } else if (barX != g_lastBarX || barY != g_lastBarY) {
            g_lastBarX = barX;
            g_lastBarY = barY;
            g_positionChangeTime = std::chrono::steady_clock::now();
            g_positionStable = false;
            if (!g_freezeValues) {
                g_freezeValues = true;
                g_frozenPercentage = g_currentPercentage;
                g_frozenShaky = g_isShakyBlock;
            }
            
            // Hide the bar while moving with fade out
            if (!g_fadingOut && !g_fadingIn) {
                g_fadingOut = true;
                g_fadeStep = 0;
                g_fadeOpacity = 1.0f;
            }
        } else {
            // Position hasn't changed, check if it's been stable for 440ms
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_positionChangeTime);
            
            if (!g_positionStable && elapsed.count() >= 440) {
                g_positionStable = true;
                g_freezeValues = false;
                // Show the bar after position is stable with fade in
                if (!g_fadingIn && !g_fadingOut && !g_waitingForNonZeroPercentage.load()) {
                    g_fadingIn = true;
                    g_fadeStep = 0;
                    g_fadeOpacity = 0.0f;
                    TryShowParryBarWindow();
                }
            }
        }
        
        // Only update position/size when not fading to prevent shifting during animation
        if (!g_fadingIn && !g_fadingOut) {
            SetWindowPos(g_parryBarWindow, HWND_TOPMOST, 0, 0, screenWidth, screenHeight, 
                         SWP_NOACTIVATE | SWP_NOZORDER);
        } else {
            SetWindowPos(g_parryBarWindow, HWND_TOPMOST, 0, 0, g_lastRenderScreenWidth > 0 ? g_lastRenderScreenWidth : screenWidth,
                         g_lastRenderScreenHeight > 0 ? g_lastRenderScreenHeight : screenHeight,
                         SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }
}

void StartPixelReading(bool enableUI = true)
{
    if (g_readerThread.joinable()) {
        g_parryRunning = false;
        g_readerThread.join();
    }
    
    g_parryRunning = true;
    g_readerThread = std::thread([enableUI]() {
        while (g_parryRunning) {
            HWND robloxWindow = WinRTCapture::FindRobloxWindow();
            if (robloxWindow) {
                if (!WinRTCapture::IsCaptureInitialized()) {
                    if (!g_app.device) {
                        if (!InitializeDirectX()) {
                            Sleep(1000);
                            continue;
                        }
                    }
                    
                    if (!InitializeRobloxCapture()) {
                        Sleep(1000);
                        continue;
                    }
                    
                }
                
    if (enableUI && !g_parryBarWindow) {
        const wchar_t* CLASS_NAME = L"VisualParryBarWnd";
        WNDCLASSW wc = {};
                    wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        
        RegisterClassW(&wc);

                    if (!WinRTCapture::IsCaptureInitialized()) {
                        continue;
                    }
                    
                    int windowWidth = WinRTCapture::GetCaptureWidth();
                    int windowHeight = WinRTCapture::GetCaptureHeight();
                    if (windowWidth <= 0 || windowHeight <= 0) {
                        continue;
                    }

                    RECT clientRect;
                    GetClientRect(robloxWindow, &clientRect);
                    POINT clientTopLeft = {clientRect.left, clientRect.top};
                    ClientToScreen(robloxWindow, &clientTopLeft);
                    
                    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                    float scaleX = (float)windowWidth / (float)screenWidth;
                    float scaleY = (float)windowHeight / (float)screenHeight;
                    float scale = (scaleX < scaleY) ? scaleX : scaleY;
                    
                    if (scale < 0.6f) scale = 0.6f;
                    
                    
                    int scaledInnerWidth = (int)(BAR_WIDTH * scale);
                    int scaledInnerHeight = (int)(BAR_HEIGHT * scale);
                    
                    int robloxCenterX = clientTopLeft.x + windowWidth / 2;
                    int robloxTopY = clientTopLeft.y + windowHeight / 4;
                    
                    int barX = robloxCenterX - scaledInnerWidth / 2;
                    int barY = robloxTopY;
                    
                    g_parryBarWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            CLASS_NAME, L"Parry Bar", WS_POPUP,
                        0, 0, screenWidth, screenHeight, NULL, NULL, GetModuleHandle(nullptr), NULL);
        
        if (g_parryBarWindow) {
            g_fadeOpacity = 0.0f;
            g_fadingIn = false;
            g_fadingOut = false;
            g_fadeStep = 0;
            g_pendingShow = false;
            g_forceHidden.store(true);
            g_startup100Active = false;
            g_startup100Seen = false;
            g_startupWaitForChange = false;
            g_startupBaselineShaky = false;
            ShowWindow(g_parryBarWindow, SW_HIDE);
                        
                        // Initialize position tracking
                        g_lastBarX = barX;
                        g_lastBarY = barY;
                        g_positionChangeTime = std::chrono::steady_clock::now();
                        g_positionStable = true;
                    }
                }
                
                if (g_parryBarWindow) {
                    UpdateParryBarPosition();
                UpdateFadeAnimation();
                    
                    // Check focus and gate state to show/hide (only if position is stable)
                    HWND foregroundWindow = GetForegroundWindow();
                    bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                    
                    if (g_positionStable) {
                        bool editorActive = EditHUD::IsEditorActive();
                        if (editorActive) {
                            g_forceHidden.store(false);
                            g_hiddenDueTo100Percent = false;
                            g_100PercentNoShakyActive = false;
                            g_waitingForNonZeroPercentage = false;
                            g_fadingIn = false;
                            g_fadingOut = false;
                            g_fadeOpacity = 1.0f;
                            if (!IsWindowVisible(g_parryBarWindow)) {
                                TryShowParryBarWindow();
                                ShowWindow(g_parryBarWindow, SW_SHOW);
                            }
                        }
                        
                        bool wantVisible = editorActive || (robloxInFocus && g_gateRed.load() && !g_hiddenDueTo100Percent && !g_forceHidden.load());
                        if (wantVisible && !g_fadingIn && !g_fadingOut && !g_waitingForNonZeroPercentage.load()) {
                            if (!IsWindowVisible(g_parryBarWindow)) {
                                g_fadingIn = true;
                                g_fadeStep = 0;
                                g_fadeOpacity = 0.0f;
                                TryShowParryBarWindow();
                            }
                        } else if (!wantVisible) {
                            if (!robloxInFocus) {
                                if (IsWindowVisible(g_parryBarWindow)) {
                                    g_fadingIn = false;
                                    g_fadingOut = false;
                                    g_fadeStep = 0;
                                    g_fadeOpacity = 0.0f;
                                    ShowWindow(g_parryBarWindow, SW_HIDE);
                                }
                            } else if (!g_fadingIn && !g_fadingOut) {
                                if (IsWindowVisible(g_parryBarWindow)) {
                                    g_fadingOut = true;
                                    g_fadeStep = 0;
                                    g_fadeOpacity = 1.0f;
                                }
                            }
                        }
                    }
                }
                
                ParryResult result = ReadParryPercentage();
                if (result.percentage >= 0) {
                    printf("Parry Percentage: %d%%\n", result.percentage);
                    
                    int currentPercentage = g_freezeValues ? g_frozenPercentage : result.percentage;
                    bool currentShaky = g_freezeValues ? g_frozenShaky : result.isShakyBlock;

                    if (g_forceHidden.load()) {
                        auto now = std::chrono::steady_clock::now();
                        if (!g_startup100Seen) {
                            if (currentPercentage == 100) {
                                if (!g_startup100Active) {
                                    g_startup100Active = true;
                                    g_startup100StartTime = now;
                                } else {
                                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_startup100StartTime).count();
                                    if (ms >= 440) {
                                        g_startup100Seen = true;
                                        g_startupWaitForChange = true;
                                        g_startupBaselineShaky = currentShaky;
                                    }
                                }
                            } else {
                                g_startup100Active = false;
                            }
                        } else if (g_startupWaitForChange) {
                            if (currentPercentage != 100 || currentShaky != g_startupBaselineShaky) {
                                g_forceHidden.store(false);
                                g_startupWaitForChange = false;
                            }
                        }
                    }
                    
                    bool editorActive = EditHUD::IsEditorActive();
                    if (editorActive) {
                        g_100PercentNoShakyActive = false;
                        g_hiddenDueTo100Percent = false;
                        g_waitingForNonZeroPercentage = false;
                    }
                    
                    bool is100PercentNoShaky = (!editorActive) && (currentPercentage == 100 && !currentShaky);
                    
                    if (is100PercentNoShaky) {
                        if (!g_100PercentNoShakyActive) {
                            g_100PercentNoShakyActive = true;
                            g_100PercentNoShakyStartTime = std::chrono::steady_clock::now();
                        } else {
                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_100PercentNoShakyStartTime);
                            
                            if (elapsed.count() >= 1440 && !g_hiddenDueTo100Percent && !g_fadingOut && !g_forceHidden.load()) {
                                if (g_positionStable && g_gateRed.load() && !g_fadingIn) {
                                    g_fadingOut = true;
                                    g_fadeStep = 0;
                                    g_fadeOpacity = 1.0f;
                                    g_hiddenDueTo100Percent = true;
                                }
                            }
                        }
                    } else {
                        if (g_100PercentNoShakyActive || g_hiddenDueTo100Percent) {
                            g_100PercentNoShakyActive = false;
                            
                            if (g_hiddenDueTo100Percent) {
                                g_hiddenDueTo100Percent = false;
                                if (g_positionStable && g_gateRed.load() && !g_fadingIn && !g_fadingOut && !g_waitingForNonZeroPercentage.load() && !g_forceHidden.load()) {
                                    g_fadingIn = true;
                                    g_fadeStep = 0;
                                    g_fadeOpacity = 0.0f;
                                    TryShowParryBarWindow();
                                }
                            }
                        }
                    }
                } else {
                    printf("Failed to read parry percentage\n");
                    g_shouldShowBar = false;
                    g_100PercentNoShakyActive = false;
                    g_hiddenDueTo100Percent = false;
                    g_forceHidden.store(true);
                    g_startup100Active = false;
                    g_startup100Seen = false;
                    g_startupWaitForChange = false;
                }
            } else {
                if (WinRTCapture::IsCaptureInitialized()) {
                    g_shouldShowBar = false;
                }
                printf("Roblox not found, waiting...\n");
                Sleep(1000);
            }
            
            Sleep(16);
        }
    });
}

void StopPixelReading()
{
    g_parryRunning = false;
    if (g_readerThread.joinable()) {
        g_readerThread.join();
    }
    // Hide the bar when stopping
    g_shouldShowBar = false;
}

void InitializePixelReader()
{
    if (g_app.device && g_app.deviceContext) {
        return;
    }
    if (!InitializeDirectX()) {
        return;
    }
}

bool IsGateCheckActive()
{
    return g_gateCheckActive.load();
}

static std::thread g_gateThread;
static std::atomic<bool> g_gateThreadRunning{ false };

void CheckGate() {
    if (!WinRTCapture::IsCaptureInitialized()) {
        g_gateCheckActive = false;
        return;
    }
    
    int clientWidth = WinRTCapture::GetCaptureWidth();
    int clientHeight = WinRTCapture::GetCaptureHeight();
    if (clientWidth <= 0 || clientHeight <= 0) {
        g_gateCheckActive = false;
        return;
    }
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    bool isFullscreen = (clientWidth == screenWidth && clientHeight == screenHeight);
    
    int gateX = (15 * clientWidth) / 1920;
    int gateY = 1063 - (1080 - clientHeight);
    
    try {
        ID3D11Texture2D* capturedTexture = nullptr;
        if (!WinRTCapture::GetCurrentFrameTexture(&capturedTexture)) return;
        ComPtr<ID3D11Texture2D> texturePtr(capturedTexture);
        
        int capWidth = clientWidth;
        int capHeight = clientHeight;
        
        if (gateX < 0) gateX = 0;
        if (gateX >= capWidth) gateX = capWidth - 1;
        if (gateY < 0) gateY = 0;
        if (gateY >= capHeight) gateY = capHeight - 1;
        
        bool gateRedOk = false;
        if (gateX >= 0 && gateX < capWidth && gateY >= 0 && gateY < capHeight) {
            int scanLeft = gateX;
            int scanTop = gateY - 8;
            int scanRight = gateX + 8;
            int scanBottom = gateY + 8;
            if (scanTop < 0) scanTop = 0;
            if (scanBottom >= capHeight) scanBottom = capHeight - 1;
            if (scanRight >= capWidth) scanRight = capWidth - 1;
            if (scanLeft < 0) scanLeft = 0;
            
            int regionWidth = (scanRight - scanLeft + 1);
            int regionHeight = (scanBottom - scanTop + 1);
            if (regionWidth < 1) regionWidth = 1;
            if (regionHeight < 1) regionHeight = 1;
            
                    for (int ry = 0; ry < regionHeight && !gateRedOk; ++ry) {
                        for (int rx = 0; rx < regionWidth; ++rx) {
                    int actualX = scanLeft + rx;
                    int actualY = scanTop + ry;
                    if (actualX >= 0 && actualX < capWidth && actualY >= 0 && actualY < capHeight) {
                        uint32_t color = WinRTCapture::GetPixelColor(actualX, actualY);
                            if (IsRedColor(color, 150, 35, 240, true)) { gateRedOk = true; break; }
                        }
                    }
            }
        }
        
        g_gateCheckActive = gateRedOk;
    }
    catch (...) {
        g_gateCheckActive = false;
    }
}

void StartGateCheck() {
    if (g_gateThread.joinable()) {
        g_gateThreadRunning = false;
        g_gateThread.join();
    }
    
    g_gateThreadRunning = true;
    g_gateThread = std::thread([]() {
        while (g_gateThreadRunning) {
            HWND robloxWindow = WinRTCapture::FindRobloxWindow();
            if (robloxWindow) {
                if (!WinRTCapture::IsCaptureInitialized()) {
                    if (!g_app.device) {
                        if (!InitializeDirectX()) {
                            Sleep(1000);
                            continue;
                        }
                    }
                    
                    if (!InitializeRobloxCapture()) {
                        Sleep(1000);
                        continue;
                    }
                    
                }
                
                CheckGate();
            } else {
                g_gateCheckActive = false;
                Sleep(1000);
            }
            
            Sleep(16);
        }
    });
}

void StopGateCheck() {
    g_gateThreadRunning = false;
    if (g_gateThread.joinable()) {
        g_gateThread.join();
    }
    g_gateCheckActive = false;
    
    if (g_app.deviceContext) {
        g_app.deviceContext->Release();
        g_app.deviceContext = nullptr;
    }
    if (g_app.device) {
        g_app.device->Release();
        g_app.device = nullptr;
    }
}

void CleanupPixelReader()
{
    // Destroy visual parry bar window - INLINED
    if (g_parryBarWindow) {
        DestroyWindow(g_parryBarWindow);
        g_parryBarWindow = nullptr;
    }
    
    // Reset all state variables
    g_shouldShowBar = false;
    g_currentPercentage = 0;
    g_isShakyBlock = false;
    g_lastBarX = -1;
    g_lastBarY = -1;
    g_positionStable = false;
    g_100PercentNoShakyActive = false;
    g_hiddenDueTo100Percent = false;
    g_pendingShow = false;
    g_forceHidden.store(true);
    g_startup100Active = false;
    g_startup100Seen = false;
    g_startupWaitForChange = false;
    g_startupBaselineShaky = false;
    

    if (g_fillGreenPB) { g_fillGreenPB->Release(); g_fillGreenPB = nullptr; }
    if (g_fillRedPB) { g_fillRedPB->Release(); g_fillRedPB = nullptr; }
    if (g_glossGreenPB) { g_glossGreenPB->Release(); g_glossGreenPB = nullptr; }
    if (g_glossRedPB) { g_glossRedPB->Release(); g_glossRedPB = nullptr; }
    if (g_outlineEdgePB) { g_outlineEdgePB->Release(); g_outlineEdgePB = nullptr; }
    if (g_outlineOuterPB) { g_outlineOuterPB->Release(); g_outlineOuterPB = nullptr; }
    if (g_dcRenderTargetPB) { g_dcRenderTargetPB->Release(); g_dcRenderTargetPB = nullptr; }
    if (g_d2dFactoryPB) { g_d2dFactoryPB->Release(); g_d2dFactoryPB = nullptr; }
    if (g_hBitmapPB) { DeleteObject(g_hBitmapPB); g_hBitmapPB = nullptr; }
    if (g_memDCPB) { DeleteDC(g_memDCPB); g_memDCPB = nullptr; }
}

void GetParryBarPosition(int& x, int& y, int& width, int& height) {
    x = g_lastBarX;
    y = g_lastBarY;
    if (g_parryBarWindow) {
        HWND robloxWindow = WinRTCapture::FindRobloxWindow();
        if (robloxWindow && WinRTCapture::IsCaptureInitialized()) {
            int windowWidth = WinRTCapture::GetCaptureWidth();
            int windowHeight = WinRTCapture::GetCaptureHeight();
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            float scaleX = (float)windowWidth / (float)screenWidth;
            float scaleY = (float)windowHeight / (float)screenHeight;
            float baseScale = (scaleX < scaleY) ? scaleX : scaleY;
            if (baseScale < 0.6f) baseScale = 0.6f;
            float user = clampf(g_userScale.load(), 0.6f, 2.0f);
            float scale = baseScale * user;
            float maxScale = baseScale * 2.0f;
            if (scale < 0.6f) scale = 0.6f;
            if (scale > maxScale) scale = maxScale;
            width = (int)(BAR_WIDTH * scale);
            height = (int)(BAR_HEIGHT * scale);
        } else {
            width = BAR_WIDTH;
            height = BAR_HEIGHT;
        }
    } else {
        width = BAR_WIDTH;
        height = BAR_HEIGHT;
    }
}

void SetParryBarPosition(int x, int y) {
    g_requestedX = x;
    g_requestedY = y;
    g_hasRequestedPosition = true;
}

bool IsParryBarRunning() {
    return g_parryRunning;
}

void SetUserScale(float s) {
    g_userScale = clampf(s, 0.6f, 2.0f);
}

float GetUserScale() {
    return g_userScale.load();
}

int GetParryPercentage() {
    return g_currentPercentage;
}

bool IsShakyBlockActive() {
    return g_isShakyBlock;
}

} // namespace ParryBar

#endif

