#ifndef HOLD_M1_PIXEL_READER_H
#define HOLD_M1_PIXEL_READER_H

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
#include <wrl.h>
#include <sstream>


#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;

namespace ParryBar {
bool IsGateCheckActive();
}

namespace WinRTCapture {
    bool InitializeCapture(ID3D11Device* device, ID3D11DeviceContext* context);
    bool IsCaptureInitialized();
    uint32_t GetPixelColor(int x, int y);
    bool GetCurrentFrameTexture(ID3D11Texture2D** outTexture);
    int GetCaptureWidth();
    int GetCaptureHeight();
    HWND FindRobloxWindow();
}

namespace HoldM1 {

struct PixelReaderApp
{
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;
};

static PixelReaderApp g_app = {};
static std::thread g_readerThread;
static std::thread g_blockIndicatorThread;
static std::atomic<bool> g_blockIndicatorThreadRunning{ false };
static bool g_pixelScanning = false;
static std::atomic<bool> g_scanningActive{ false };

static std::atomic<bool> g_leftClickHeld{ false };
static std::atomic<bool> g_macroRunning{ false };
static std::atomic<bool> g_ignoreNextUp{ false };
static std::thread g_macroThread;
static std::atomic<bool> g_macroThreadRunning{ false };
static std::atomic<bool> g_holdM1Enabled{ false };
static std::atomic<bool> g_intelligentHoldM1Enabled{ true };
static std::chrono::steady_clock::time_point g_clickStartTime;
static std::atomic<bool> g_checkingWindow{ false };
static std::atomic<bool> g_repositioning{ false };
static bool g_lastBlockStateCached = false;
static std::chrono::steady_clock::time_point g_nextRepositionTime;
static int g_successfulPatternCount = 0;
static bool g_skipReposition = false;
static int g_blockChangesSinceLastCheck = 0;
static bool g_prevBlockIndicator = false;
static int g_patternStage = 0; // 0: wait false->true, 1: wait true->false
static std::chrono::steady_clock::time_point g_patternStartTime;
static bool g_m1PressedDuringPattern = false;
static std::chrono::steady_clock::time_point g_blockTrueSince;
static bool g_forceFalseDuringReposition = false;
static bool g_repositionPaused = false;
static std::atomic<bool> g_gateOverrideArmed{ false };
static std::atomic<bool> g_localGateValue{ false };
static bool g_isM1LastState = false;
static bool g_isM1Triggered = false;
static std::chrono::steady_clock::time_point g_isM1PressTime;
static bool g_isM1WaitingForTrue = false;
static bool g_isM1PrevBlockIndicator = false;
static UINT g_criticalAttackVKCode = 'R';
static std::atomic<bool> g_criticalAttackKeyHeld{ false };
static bool g_isCriticalAttackTriggered = false;
static std::chrono::steady_clock::time_point g_isCriticalAttackPressTime;
static bool g_isCriticalAttackWaitingForTrue = false;
static bool g_isCriticalAttackPrevBlockIndicator = false;
// Ensure M1 and CriticalAttack can never be "true" at the same time.
// First input that starts a timing window owns it; the other is ignored until the window resolves.
static std::atomic<int> g_attackOwner{ 0 }; // 0 = none, 1 = M1, 2 = Critical, 3 = Mantra
static std::atomic<bool> g_mantraKeyHeld{ false };
static bool g_isMantraTriggered = false;
static std::chrono::steady_clock::time_point g_isMantraPressTime;
static bool g_isMantraWaitingForTrue = false;
static bool g_isMantraPrevBlockIndicator = false;

static inline bool GetGateCheckActive()
{
    if (g_gateOverrideArmed.load()) return g_localGateValue.load();
    return ParryBar::IsGateCheckActive();
}


static int g_currentX = 964;
static int g_baseY = 946;
static bool g_isSearching = false;
static int g_leftBound = 964;
static int g_rightBound = 964;
static int g_currentScanRow = 0;
static bool g_initialBoundsSet = false;
static int g_originalX = 964;
static int g_leftSearchOffset = 0;
static bool g_positionLocked = false;
static int g_lockedTargetX = 0;
static int g_lockedTargetY = 0;
static int g_lastClientWidth = 0;
static int g_lastClientHeight = 0;
static uint32_t g_lockedTargetColor = 0;
static std::atomic<bool> g_blockIndicatorActive{ false };
static bool g_lastBlockIndicatorState = false;
static int g_lastSearchX = -1;
static int g_sameSearchCount = 0;
static bool g_lastGateCheck = false;




void StopPixelScanning();
void CleanupPixelReader();
void MacroThreadFunc();
void HandleMouseDown();
void HandleMouseUp();
void HandleCriticalAttackKeyDown();
void HandleCriticalAttackKeyUp();
void SetCriticalAttackVKCode(UINT vkCode);
void HandleMantraKeyDown();
void StartBlockIndicatorCheck();
void StopBlockIndicatorCheck();
void SetIntelligentHoldM1Enabled(bool enabled);
bool IsM1();
bool CriticalAttack();
bool Mantra();
bool IsBlockIndicatorActive();




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

bool IsBrownColor(uint32_t color) {
    uint8_t red = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8) & 0xFF;
    uint8_t blue = color & 0xFF;
    return (abs(red - 65) <= 50 && abs(green - 50) <= 50 && abs(blue - 41) <= 75);
}

bool IsTargetColor(uint32_t color) {
    uint8_t red = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8) & 0xFF;
    uint8_t blue = color & 0xFF;
    return (abs(red - 219) <= 20 && abs(green - 235) <= 20 && abs(blue - 235) <= 25);
}

uint32_t GetPixelColor(int x, int y) {
    if (!WinRTCapture::IsCaptureInitialized()) {
        return 0;
    }
    return WinRTCapture::GetPixelColor(x, y);
}

void PerformPixelScan() {
    try {
        if (!WinRTCapture::IsCaptureInitialized()) {
            return;
        }
        
        if (!WinRTCapture::IsCaptureInitialized()) {
            return;
        }
        
        int clientWidth = WinRTCapture::GetCaptureWidth();
        int clientHeight = WinRTCapture::GetCaptureHeight();
        if (clientWidth <= 0 || clientHeight <= 0) {
            return;
        }
        
        bool currentGateCheck = GetGateCheckActive();
        
        // TEMPORARILY DISABLED GATECHECK EARLY RETURN FOR TESTING
        // if (currentGateCheck) {
        //     g_lastGateCheck = currentGateCheck;
        //     return;
        // }
        
        // Reposition scheduler:
        // - Every 1440ms, TRIGGER REPOSITION by simulating a gatecheck false transition
        //   (set g_lastGateCheck=true and let the existing gate false handler do the reset)
        // - Skip while gatecheck is true
        // - Skip entirely after 3 consecutive successful patterns (until gate/res changes)
        {
            auto now = std::chrono::steady_clock::now();
            bool gateChanged = (currentGateCheck != g_lastGateCheck);
            if (gateChanged) {
                g_successfulPatternCount = 0;
                g_skipReposition = false;
                g_patternStage = 0;
                g_prevBlockIndicator = false;
                g_repositionPaused = false;
                g_m1PressedDuringPattern = false;
                g_nextRepositionTime = now + std::chrono::milliseconds(1440);
            }
            if (!g_skipReposition && !g_repositionPaused && now >= g_nextRepositionTime) {
                if (g_successfulPatternCount >= 1) {
                    g_skipReposition = true;
                } else if (!g_positionLocked) {
                    // Force a local gate false transition every 1440ms to reuse the existing reposition logic path
                    // Only do this if position isn't locked yet
                    g_gateOverrideArmed.store(true);
                    g_localGateValue.store(false); // force currentGateCheck=false
                    g_lastGateCheck = true; // ensure previous=true so branch triggers next iteration
                    g_nextRepositionTime = now + std::chrono::milliseconds(1440);
                    return;
                }
            }
        }
        
        if (g_lastGateCheck && !currentGateCheck) {
            g_lastGateCheck = currentGateCheck;
            
            g_currentX = 964;
            g_originalX = 964;
            g_leftSearchOffset = 0;
            g_isSearching = false;
            g_currentScanRow = 0;
            g_initialBoundsSet = false;
            g_positionLocked = false;
            g_leftBound = 0;
            g_rightBound = 0;
            g_blockIndicatorActive.store(false);
            for (int i = 0; i < 3; ++i) {
            }
            g_forceFalseDuringReposition = false;
            g_blockTrueSince = {};
            g_gateOverrideArmed.store(false);
            g_localGateValue.store(false);
            return;
        }
        
        g_lastGateCheck = currentGateCheck;
        
        // Check if client rectangle changed (window resized)
        bool clientRectChanged = (clientWidth != g_lastClientWidth || clientHeight != g_lastClientHeight);
        if (clientRectChanged) {
            g_lastClientWidth = clientWidth;
            g_lastClientHeight = clientHeight;
            
            g_patternStage = 0;
            g_prevBlockIndicator = false;
            g_repositionPaused = false;
            g_m1PressedDuringPattern = false;
            g_currentX = 964;
            g_originalX = 964;
            g_leftSearchOffset = 0;
            g_isSearching = false;
            g_currentScanRow = 0;
            g_initialBoundsSet = false;
            g_positionLocked = false;
            g_leftBound = 0;
            g_rightBound = 0;
            g_blockIndicatorActive.store(false);
            for (int i = 0; i < 3; ++i) {
            }
            g_successfulPatternCount = 0;
            g_skipReposition = false;
            g_repositionPaused = false;
            g_blockChangesSinceLastCheck = 0;
            g_nextRepositionTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(1440);
            return;
        }
        
        // Get ONE frame like parrybar.cpp does
        if (!WinRTCapture::IsCaptureInitialized()) {
            return;
        }
        
        
        int capWidth = WinRTCapture::GetCaptureWidth();
        int capHeight = WinRTCapture::GetCaptureHeight();
        int currentX = (g_currentX * capWidth) / 1920;
        int y = g_baseY - (1080 - capHeight);
        
        // If position is locked and no changes, monitor the locked target color
        if (g_positionLocked && !clientRectChanged) {
            // Read the pixel at the locked target position
            if (g_lockedTargetX >= 0 && g_lockedTargetX < capWidth && g_lockedTargetY >= 0 && g_lockedTargetY < capHeight) {
                uint32_t currentColor = WinRTCapture::GetPixelColor(g_lockedTargetX, g_lockedTargetY);
                
                // Check if color changed from original locked color
                bool prevBlock = g_blockIndicatorActive.load();
                bool currBlock = (currentColor != g_lockedTargetColor);
                bool leftHeld = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                
                if (currBlock && !prevBlock) {
                    g_blockIndicatorActive.store(true);
                    g_blockTrueSince = std::chrono::steady_clock::now();
                    if (leftHeld && !g_repositionPaused) {
                        g_repositionPaused = true;
                    }
                } else if (!currBlock && prevBlock) {
                    g_blockIndicatorActive.store(false);
                    g_blockTrueSince = {};
                    if (g_repositionPaused && leftHeld) {
                        g_repositionPaused = false;
                        g_successfulPatternCount = 1;
                        g_skipReposition = true;
                        auto nowPattern = std::chrono::steady_clock::now();
                        g_nextRepositionTime = nowPattern + std::chrono::milliseconds(1440);
                    }
                }
                
                if (!leftHeld) {
                    g_repositionPaused = false;
                }
                
                // Pattern detection: M1 clicked while FALSE -> TRUE -> FALSE
                prevBlock = g_prevBlockIndicator;
                currBlock = g_blockIndicatorActive.load();
                auto nowPattern = std::chrono::steady_clock::now();
                
                if (g_patternStage == 0) {
                    if (!prevBlock && !currBlock && leftHeld) {
                        g_m1PressedDuringPattern = true;
                    }
                    if (!prevBlock && currBlock && g_m1PressedDuringPattern) {
                        g_patternStage = 1;
                        g_patternStartTime = nowPattern;
                    } else if (!prevBlock && currBlock && !g_m1PressedDuringPattern) {
                        g_m1PressedDuringPattern = false;
                    }
                } else if (g_patternStage == 1) {
                    if (!currBlock) {
                        if (g_m1PressedDuringPattern) {
                            g_successfulPatternCount = 1;
                            g_skipReposition = true;
                            g_nextRepositionTime = nowPattern + std::chrono::milliseconds(1440);
                        }
                        g_patternStage = 0;
                        g_m1PressedDuringPattern = false;
                    }
                }
                g_prevBlockIndicator = currBlock;
                
                // If block indicator stays TRUE longer than 4320ms → assume glitch, trigger reposition via gate false simulation
                if (g_blockIndicatorActive.load() && !g_repositionPaused) {
                    if (g_blockTrueSince.time_since_epoch().count() != 0) {
                        auto nowCheck = std::chrono::steady_clock::now();
                        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(nowCheck - g_blockTrueSince).count();
                        if (ms > 4320) {
                            // Force a reset using existing logic path; report false during this special reposition
                            g_forceFalseDuringReposition = true;
                            g_gateOverrideArmed.store(true);
                            g_localGateValue.store(false); // ensure currentGateCheck=false
                            g_lastGateCheck = true; // simulate previous=true so next pass processes true->false
                            g_nextRepositionTime = nowCheck + std::chrono::milliseconds(1440);
                            return;
                        }
                    }
                }
                
            }
            return;
        }
        
        if (currentX < 0 || currentX >= capWidth || y < 0 || y >= capHeight) {
            return;
        }
        
        auto samplePixelColor = [&](int sx, int sy) -> uint32_t {
            if (sx < 0 || sx >= capWidth || sy < 0 || sy >= capHeight) {
                return 0;
            }
            return WinRTCapture::GetPixelColor(sx, sy);
        };
        
        uint32_t pixelColor = samplePixelColor(currentX, y);
        
        if (!g_isSearching) {
            bool initialBrown = IsBrownColor(pixelColor);
            bool initialTarget = IsTargetColor(pixelColor);
            
            if (initialTarget) {
                if (!g_positionLocked) {
                    g_positionLocked = true;
                    g_lockedTargetX = currentX;
                    g_lockedTargetY = y;
                    g_lockedTargetColor = pixelColor;
                    g_blockIndicatorActive.store(false);
                }
                return;
            }
            
            if (initialBrown) {
                if (g_lastSearchX == currentX) {
                    g_sameSearchCount++;
                } else {
                    g_sameSearchCount = 0;
                }
            } else {
                g_sameSearchCount = 0;
            }
            g_lastSearchX = currentX;
            
            if (!initialBrown) {
                int offsetX = currentX + 3;
                uint32_t offsetColor = samplePixelColor(offsetX, y);
                if (IsTargetColor(offsetColor)) {
                    g_currentX = offsetX;
                    g_originalX = offsetX;
                    g_positionLocked = true;
                    g_lockedTargetX = offsetX;
                    g_lockedTargetY = y;
                    g_lockedTargetColor = offsetColor;
                    g_blockIndicatorActive.store(false);
                    return;
                }
                if (IsBrownColor(offsetColor)) {
                    g_currentX = offsetX;
                    g_originalX = offsetX;
                    currentX = offsetX;
                    pixelColor = offsetColor;
                    initialBrown = true;
                    g_sameSearchCount = 0;
                }
            }
            
            if (initialBrown && g_sameSearchCount > 1) {
                int forcedOffset = 3;
                int forcedX = currentX + forcedOffset;
                if (forcedX >= capWidth) forcedX = capWidth - 1;
                uint32_t forcedColor = samplePixelColor(forcedX, y);
                if (IsTargetColor(forcedColor)) {
                    g_currentX = forcedX;
                    g_originalX = forcedX;
                    g_positionLocked = true;
                    g_lockedTargetX = forcedX;
                    g_lockedTargetY = y;
                    g_lockedTargetColor = forcedColor;
                    g_blockIndicatorActive.store(false);
                    g_sameSearchCount = 0;
                    return;
                }
                if (IsBrownColor(forcedColor)) {
                    g_currentX = forcedX;
                    g_originalX = forcedX;
                    currentX = forcedX;
                    pixelColor = forcedColor;
                    initialBrown = true;
                    g_sameSearchCount = 0;
                }
            }
            
            if (initialBrown) {
                g_isSearching = true;
                g_leftBound = currentX - 9;
                g_rightBound = currentX + 9;
                g_currentScanRow = 0;
                g_initialBoundsSet = false;
            } else {
                // Try moving left pixel by pixel up to 9 pixels
                if (g_leftSearchOffset < 9) {
                    g_leftSearchOffset++;
                    g_currentX = g_originalX - g_leftSearchOffset;
                    return;
                } else {
                    // Reset back to original position if no brown found in 9 pixels
                    g_leftSearchOffset = 0;
                    g_currentX = g_originalX;
                return;
                }
            }
        }
        
        if (g_isSearching) {
            // Calculate current row Y position
            int currentRowY = y + g_currentScanRow;
            if (currentRowY >= capHeight || g_currentScanRow >= 9) {
                g_isSearching = false;
                g_currentScanRow = 0;
                g_initialBoundsSet = false;
                return;
            }
            
            // First row: establish initial boundaries
            if (g_currentScanRow == 0 && !g_initialBoundsSet) {
                g_leftBound = currentX;
                int leftSteps = 0;
                while (g_leftBound >= 0 && leftSteps < 5) {
                    uint32_t leftPixel = WinRTCapture::GetPixelColor(g_leftBound, currentRowY);
                    
                    if (!IsBrownColor(leftPixel) && !IsTargetColor(leftPixel)) {
                        g_leftBound++;
                        break;
                    }
                    g_leftBound--;
                    leftSteps++;
                }
                
                // Scan right from center
                g_rightBound = currentX;
                int rightSteps = 0;
                while (g_rightBound < capWidth && rightSteps < 5) {
                    uint32_t rightPixel = WinRTCapture::GetPixelColor(g_rightBound, currentRowY);
                    
                    if (IsTargetColor(rightPixel)) {
                                    // Lock position on first target found
                                    if (!g_positionLocked) {
                                        g_positionLocked = true;
                                        g_lockedTargetX = g_rightBound;
                                        g_lockedTargetY = currentRowY;
                                        g_lockedTargetColor = rightPixel; // Store original color
                                        g_blockIndicatorActive.store(false);
                                    } else {
                                    }
                        
                        
                        g_rightBound++;
                        g_initialBoundsSet = true;
                        break;
                    }
                    
                    if (!IsBrownColor(rightPixel) && !IsTargetColor(rightPixel)) {
                        g_rightBound--;
                        g_initialBoundsSet = true;
                        break;
                    }
                    g_rightBound++;
                    rightSteps++;
                }
                
                if (!g_initialBoundsSet) {
                    g_initialBoundsSet = true;
                }
                
                g_currentScanRow++;
                return;
            }
            
            // Subsequent rows: scan within inherited boundaries
            if (g_currentScanRow > 0 && g_initialBoundsSet) {
                // Region-based scanning like parrybar gatecheck
                int scanLeft = g_leftBound;
                int scanTop = currentRowY;
                int scanRight = g_rightBound;
                int scanBottom = currentRowY;
                
                if (scanLeft < 0) scanLeft = 0;
                if (scanRight >= capWidth) scanRight = capWidth - 1;
                if (scanTop < 0) scanTop = 0;
                if (scanBottom >= capHeight) scanBottom = capHeight - 1;
                
                int regionWidth = (scanRight - scanLeft + 1);
                int regionHeight = (scanBottom - scanTop + 1);
                if (regionWidth < 1) regionWidth = 1;
                if (regionHeight < 1) regionHeight = 1;
                
                D3D11_BOX regionBox = {};
                regionBox.left = scanLeft; regionBox.top = scanTop; regionBox.front = 0;
                regionBox.right = scanRight + 1; regionBox.bottom = scanBottom + 1; regionBox.back = 1;
                
                        bool foundTarget = false;
                        for (int ry = 0; ry < regionHeight && !foundTarget; ++ry) {
                            for (int rx = 0; rx < regionWidth; ++rx) {
                                    int actualX = scanLeft + rx;
                                    int actualY = scanTop + ry;
                        uint32_t color = WinRTCapture::GetPixelColor(actualX, actualY);
                                    
                        if (IsTargetColor(color)) {
                                    // Lock position on first target found
                                    if (!g_positionLocked) {
                                        g_positionLocked = true;
                                        g_lockedTargetX = actualX;
                                        g_lockedTargetY = actualY;
                                g_lockedTargetColor = color;
                                        g_blockIndicatorActive.store(false);
                                    }
                                    
                                    foundTarget = true;
                                    break;
                                }
                            }
                }
                
                g_currentScanRow++;
                return;
            }
        }
        
    } catch (const std::exception& e) {
    }
}

void StartPixelScanning()
{
    if (g_pixelScanning) {
        return;
    }
    
    if (!InitializeDirectX()) {
        return;
    }
    
    g_pixelScanning = true;
    g_scanningActive = true;
    
    g_currentX = 964;
    g_originalX = 964;
    g_baseY = 946;
    g_isSearching = false;
    g_leftBound = 964;
    g_rightBound = 964;
    g_currentScanRow = 0;
    g_initialBoundsSet = false;
    g_leftSearchOffset = 0;
    g_positionLocked = false;
    g_lockedTargetX = 0;
    g_lockedTargetY = 0;
    g_lastClientWidth = 0;
    g_lastClientHeight = 0;
    g_lockedTargetColor = 0;
    g_blockIndicatorActive.store(false);
    g_lastGateCheck = false;
    g_repositioning.store(false);
    g_lastBlockStateCached = false;
    g_successfulPatternCount = 0;
    g_skipReposition = false;
    g_repositionPaused = false;
    g_blockChangesSinceLastCheck = 0;
    g_nextRepositionTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(1440);
    
    if (!g_macroThreadRunning.load()) {
        g_macroThreadRunning = true;
        g_macroThread = std::thread(MacroThreadFunc);
    }
}

void StopPixelScanning()
{
    if (!g_pixelScanning) {
        return;
    }
    
    g_pixelScanning = false;
    g_scanningActive = false;
    
    g_macroThreadRunning = false;
    if (g_macroThread.joinable()) {
        g_macroThread.join();
    }
    
    CleanupPixelReader();
}

void SetHoldM1Enabled(bool enabled)
{
    g_holdM1Enabled.store(enabled);
}

void SetIntelligentHoldM1Enabled(bool enabled)
{
    g_intelligentHoldM1Enabled.store(enabled);
}

static inline bool IsRobloxForeground()
{
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    if (!robloxWindow) return false;
    HWND foregroundWindow = GetForegroundWindow();
    return (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
}

void InitializePixelReader()
{
    if (!InitializeDirectX()) {
        return;
    }
}


void MacroThreadFunc()
{
    while (g_macroThreadRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        if (g_leftClickHeld.load() && !g_macroRunning.load() && g_holdM1Enabled.load()) {
            if (!IsRobloxForeground()) {
                g_checkingWindow.store(false);
            } else
            if (!g_intelligentHoldM1Enabled.load()) {
                g_checkingWindow.store(false);
                g_macroRunning.store(true);
                
                while (g_leftClickHeld.load() && g_macroThreadRunning.load() && g_holdM1Enabled.load() && IsRobloxForeground()) {
                    INPUT down = {};
                    down.type = INPUT_MOUSE;
                    down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    SendInput(1, &down, sizeof(INPUT));
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    
                    g_ignoreNextUp.store(true);
                    
                    INPUT up = {};
                    up.type = INPUT_MOUSE;
                    up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &up, sizeof(INPUT));
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    std::this_thread::yield();
                }
                
                g_macroRunning.store(false);
            } else if (g_checkingWindow.load()) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_clickStartTime).count();
                
                if (g_blockIndicatorActive.load()) {
                    g_checkingWindow.store(false);
                    g_macroRunning.store(true);
                    
                    while (g_leftClickHeld.load() && g_macroThreadRunning.load() && g_holdM1Enabled.load() && IsRobloxForeground()) {
                        INPUT down = {};
                        down.type = INPUT_MOUSE;
                        down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                        SendInput(1, &down, sizeof(INPUT));
                        
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        
                        g_ignoreNextUp.store(true);
                        
                        INPUT up = {};
                        up.type = INPUT_MOUSE;
                        up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                        SendInput(1, &up, sizeof(INPUT));
                        
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        std::this_thread::yield();
                    }
                    
                    g_macroRunning.store(false);
                } else if (elapsed > 440) {
                    g_checkingWindow.store(false);
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void HandleMouseDown()
{
    if (!g_leftClickHeld.load() && !g_macroRunning.load() && g_holdM1Enabled.load()) {
        if (!IsRobloxForeground()) {
            return;
        }
        // Only allow arming M1 if nobody else owns the window yet (or M1 already owns it).
        int owner = g_attackOwner.load();
        if (owner != 0 && owner != 1) {
            return; // CriticalAttack or Mantra owns it, don't allow M1
        }
        if (owner == 0) {
            int expected = 0;
            if (!g_attackOwner.compare_exchange_strong(expected, 1)) {
                return; // someone else won the race
            }
        }
        g_leftClickHeld.store(true);
        g_clickStartTime = std::chrono::steady_clock::now();
        g_checkingWindow.store(g_intelligentHoldM1Enabled.load());
    }
}

void HandleMouseUp()
{
    if (g_ignoreNextUp.load()) {
        g_ignoreNextUp.store(false);
        return;
    }
    
    g_leftClickHeld.store(false);
    g_checkingWindow.store(false);
}

void HandleCriticalAttackKeyDown()
{
    // Only allow arming CriticalAttack if nobody else owns the window yet (or Critical already owns it).
    int owner = g_attackOwner.load();
    if (owner != 0 && owner != 2) {
        return; // M1 or Mantra owns it, don't allow CriticalAttack
    }
    if (owner == 0) {
        int expected = 0;
        if (!g_attackOwner.compare_exchange_strong(expected, 2)) {
            return; // someone else won the race
        }
    }

    if (!g_criticalAttackKeyHeld.load()) {
        g_criticalAttackKeyHeld.store(true);
        g_isCriticalAttackPressTime = std::chrono::steady_clock::now();
        g_isCriticalAttackWaitingForTrue = true;
        g_isCriticalAttackTriggered = false;
    }
}

void HandleCriticalAttackKeyUp()
{
    g_criticalAttackKeyHeld.store(false);
}

void SetCriticalAttackVKCode(UINT vkCode)
{
    g_criticalAttackVKCode = vkCode;
}

void HandleMantraKeyDown()
{
    // Only allow arming Mantra if nobody else owns the window yet (or Mantra already owns it).
    int owner = g_attackOwner.load();
    if (owner != 0 && owner != 3) {
        return;
    }
    if (owner == 0) {
        int expected = 0;
        if (!g_attackOwner.compare_exchange_strong(expected, 3)) {
            return; // someone else won the race
        }
    }
    if (!g_mantraKeyHeld.load()) {
        g_mantraKeyHeld.store(true);
        g_isMantraPressTime = std::chrono::steady_clock::now();
        g_isMantraWaitingForTrue = true;
        g_isMantraTriggered = false;
    }
}

void CleanupPixelReader()
{
    if (g_blockIndicatorThreadRunning.load()) {
        return;
    }
    
    if (g_app.deviceContext) {
        g_app.deviceContext->Release();
        g_app.deviceContext = nullptr;
    }
    
    if (g_app.device) {
        g_app.device->Release();
        g_app.device = nullptr;
    }
}

bool IsBlockIndicatorActive();

bool IsM1()
{
    if (g_holdM1Enabled.load() && g_macroRunning.load()) {
        return true;
    }
    
    if (!ParryBar::IsGateCheckActive()) {
        return false;
    }
    
    // Early ownership check: if CriticalAttack or Mantra owns it, M1 must return false immediately.
    int owner = g_attackOwner.load();
    if (owner == 2 || owner == 3) {
        return false; // CriticalAttack or Mantra owns it, M1 cannot be true
    }
    
    bool currentM1 = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    bool blockActive = g_blockIndicatorActive.load();
    bool prevBlock = g_isM1PrevBlockIndicator;
    auto now = std::chrono::steady_clock::now();
    
    if (currentM1 && !g_isM1LastState) {
        // Acquire ownership for this timing window if no one else has it.
        int owner = g_attackOwner.load();
        if (owner == 0) {
            int expected = 0;
            if (g_attackOwner.compare_exchange_strong(expected, 1)) {
                g_isM1PressTime = now;
                g_isM1WaitingForTrue = true;
                g_isM1Triggered = false;
            }
        } else if (owner == 1) {
            g_isM1PressTime = now;
            g_isM1WaitingForTrue = true;
            g_isM1Triggered = false;
        }
        // If owner == 2 (Critical), ignore M1 press entirely.
    }
    
    if (currentM1 && prevBlock && !blockActive) {
        // Only allow this restart behavior if M1 owns the window.
        if (g_attackOwner.load() == 1) {
            g_isM1PressTime = now;
            g_isM1WaitingForTrue = true;
            g_isM1Triggered = false;
        }
    }
    
    if (g_isM1WaitingForTrue) {
        // If Critical owns the window, M1 must not trigger.
        if (g_attackOwner.load() != 1) {
            g_isM1WaitingForTrue = false;
            g_isM1Triggered = false;
        } else {
        auto elapsedSincePress = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_isM1PressTime).count();
        
        if (blockActive && elapsedSincePress <= 440) {
            g_isM1Triggered = true;
            g_isM1WaitingForTrue = false;
        } else if (elapsedSincePress > 440) {
            g_isM1WaitingForTrue = false;
            // Window expired with no trigger; release ownership.
            int expectedOwner = 1;
            g_attackOwner.compare_exchange_strong(expectedOwner, 0);
        }
        }
    }
    
    if (g_isM1Triggered && !blockActive) {
        g_isM1Triggered = false;
        // Trigger consumed; release ownership.
        int expectedOwner = 1;
        g_attackOwner.compare_exchange_strong(expectedOwner, 0);
    }
    
    g_isM1PrevBlockIndicator = blockActive;
    g_isM1LastState = currentM1;
    
    return g_isM1Triggered;
}

bool CriticalAttack()
{
    if (!ParryBar::IsGateCheckActive()) {
        return false;
    }
    
    // Early ownership check: if M1 or Mantra owns it, CriticalAttack must return false immediately.
    int owner = g_attackOwner.load();
    if (owner == 1 || owner == 3) {
        return false; // M1 or Mantra owns it, CriticalAttack cannot be true
    }
    
    bool blockActive = g_blockIndicatorActive.load();
    auto now = std::chrono::steady_clock::now();
    
    if (g_isCriticalAttackWaitingForTrue) {
        // If M1 owns the window, Critical must not trigger.
        if (g_attackOwner.load() != 2) {
            g_isCriticalAttackWaitingForTrue = false;
            g_isCriticalAttackTriggered = false;
            g_criticalAttackKeyHeld.store(false);
        } else {
        auto elapsedSincePress = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_isCriticalAttackPressTime).count();
        
        if (blockActive && elapsedSincePress <= 440) {
            g_isCriticalAttackTriggered = true;
            g_isCriticalAttackWaitingForTrue = false;
        } else if (elapsedSincePress > 440) {
            g_isCriticalAttackWaitingForTrue = false;
            g_criticalAttackKeyHeld.store(false);
            // Window expired with no trigger; release ownership.
            int expectedOwner = 2;
            g_attackOwner.compare_exchange_strong(expectedOwner, 0);
        }
        }
    }
    
    if (g_isCriticalAttackTriggered && !blockActive) {
        g_isCriticalAttackTriggered = false;
        g_criticalAttackKeyHeld.store(false);
        // Trigger consumed; release ownership.
        int expectedOwner = 2;
        g_attackOwner.compare_exchange_strong(expectedOwner, 0);
    }
    
    g_isCriticalAttackPrevBlockIndicator = blockActive;
    
    return g_isCriticalAttackTriggered;
}

bool Mantra()
{
    if (!ParryBar::IsGateCheckActive()) {
        return false;
    }
    
    // Early ownership check: if M1 or CriticalAttack owns it, Mantra must return false immediately.
    int owner = g_attackOwner.load();
    if (owner == 1 || owner == 2) {
        return false; // M1 or CriticalAttack owns it, Mantra cannot be true
    }
    
    bool blockActive = g_blockIndicatorActive.load();
    auto now = std::chrono::steady_clock::now();
    
    if (g_isMantraWaitingForTrue) {
        // If Mantra doesn't own the window, it must not trigger.
        if (g_attackOwner.load() != 3) {
            g_isMantraWaitingForTrue = false;
            g_isMantraTriggered = false;
            g_mantraKeyHeld.store(false);
        } else {
        auto elapsedSincePress = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_isMantraPressTime).count();
        
        if (blockActive && elapsedSincePress <= 440) {
            g_isMantraTriggered = true;
            g_isMantraWaitingForTrue = false;
        } else if (elapsedSincePress > 440) {
            g_isMantraWaitingForTrue = false;
            g_mantraKeyHeld.store(false);
            // Window expired with no trigger; release ownership.
            int expectedOwner = 3;
            g_attackOwner.compare_exchange_strong(expectedOwner, 0);
        }
        }
    }
    
    if (g_isMantraTriggered && !blockActive) {
        g_isMantraTriggered = false;
        g_mantraKeyHeld.store(false);
        // Trigger consumed; release ownership.
        int expectedOwner = 3;
        g_attackOwner.compare_exchange_strong(expectedOwner, 0);
    }
    
    g_isMantraPrevBlockIndicator = blockActive;
    
    return g_isMantraTriggered;
}

bool IsBlockIndicatorActive()
{
    if (!ParryBar::IsGateCheckActive()) {
        return false;
    }
    return g_blockIndicatorActive.load();
}

void StartBlockIndicatorCheck()
{
    if (g_blockIndicatorThread.joinable()) {
        g_blockIndicatorThreadRunning = false;
        g_blockIndicatorThread.join();
    }
    
    if (!g_app.device) {
        if (!InitializeDirectX()) {
            return;
        }
    }
    
    g_lastBlockIndicatorState = false;
    g_blockIndicatorThreadRunning = true;
    g_blockIndicatorThread = std::thread([]() {
        while (g_blockIndicatorThreadRunning) {
            HWND robloxWindow = WinRTCapture::FindRobloxWindow();
            if (robloxWindow) {
                if (!WinRTCapture::IsCaptureInitialized()) {
                    if (!InitializeRobloxCapture()) {
                        Sleep(1000);
                        continue;
                    }
                }
                
                PerformPixelScan();
                
                bool currentState = IsBlockIndicatorActive();
                if (currentState != g_lastBlockIndicatorState) {
                    g_lastBlockIndicatorState = currentState;
                }
            } else {
                g_blockIndicatorActive.store(false);
                bool currentState = g_blockIndicatorActive.load();
                if (currentState != g_lastBlockIndicatorState) {
                    g_lastBlockIndicatorState = currentState;
                }
                Sleep(1000);
            }
            
            Sleep(16);
        }
    });
}

void StopBlockIndicatorCheck()
{
    g_blockIndicatorThreadRunning = false;
    if (g_blockIndicatorThread.joinable()) {
        g_blockIndicatorThread.join();
    }
}

} // namespace HoldM1

#endif