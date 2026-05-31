#ifndef LIGHTSPEEDREFLEX_H
#define LIGHTSPEEDREFLEX_H

#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <thread>

// Forward declarations
namespace ParryBar {
    int GetParryPercentage();
    bool IsShakyBlockActive();
}

namespace LightSpeedReflex {

static const ULONG_PTR kInjectedExtraInfo = (ULONG_PTR)0x4C535246u;
static std::atomic<bool> g_physHeld{ false };
static std::atomic<bool> g_forwardHeld{ false };
static std::atomic<bool> g_lastBlockIndicator{ false };
static std::atomic<ULONGLONG> g_blockUntilMs{ 0 };
static std::atomic<ULONGLONG> g_cooldownUntilMs{ 0 };
static std::atomic<bool> g_blockingConditionActive{ false };
static std::atomic<bool> g_allowCurrentHold{ false };
static std::atomic<bool> g_lastShakyBlock{ false };

static inline bool IsOurInjected(const KBDLLHOOKSTRUCT* kb) {
    if (!kb) return false;
    return (kb->dwExtraInfo == kInjectedExtraInfo) || ((kb->flags & LLKHF_INJECTED) != 0);
}

bool ShouldSimulateKeyPress(bool betterParryEnabled) {
    if (!betterParryEnabled) {
        return false;
    }
    
    bool blockIndicator = HoldM1::IsBlockIndicatorActive();
    
    if (!blockIndicator) {
        return true;
    }
    
    bool isM1 = HoldM1::IsM1();
    bool isCritical = HoldM1::CriticalAttack();
    bool isMantra = HoldM1::Mantra();
    
    if (isM1 || isCritical || isMantra) {
        return true;
    }
    
    return false;
}

static inline void SendKeyPress(USHORT scanCode, bool keyDown) {
    if (scanCode == 0) return;
    
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = scanCode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    input.ki.dwExtraInfo = kInjectedExtraInfo;
    if (!keyDown) {
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    }
    
    SendInput(1, &input, sizeof(INPUT));
}

static inline void EnsureForwardState(USHORT scanCode, bool wantDown) {
    if (wantDown) {
        if (!g_forwardHeld.exchange(true)) {
            SendKeyPress(scanCode, true);
        }
    } else {
        if (g_forwardHeld.exchange(false)) {
            SendKeyPress(scanCode, false);
        }
    }
}

void OnPhysicalParryDown() {
    g_physHeld.store(true);
}

void OnPhysicalParryUp(USHORT scanCode) {
    g_physHeld.store(false);
    g_allowCurrentHold.store(false); // Reset when key is released
    EnsureForwardState(scanCode, false);
}

static inline void UpdateBlockWindow(bool blockIndicator, ULONGLONG nowMs) {
    bool prev = g_lastBlockIndicator.exchange(blockIndicator);
    if (!prev && blockIndicator) {
        ULONGLONG cooldownUntil = g_cooldownUntilMs.load();
        if (nowMs >= cooldownUntil) {
            ULONGLONG blockUntil = nowMs + 220;
            g_blockUntilMs.store(blockUntil);
            g_cooldownUntilMs.store(blockUntil + 440);
        }
    }
    if (!blockIndicator) {
        g_blockUntilMs.store(0);
    }
}

void UpdateForwarding(USHORT scanCode, bool betterParryEnabled, bool robloxInFocus, bool antiShakyBlockEnabled) {
    if (scanCode == 0 || !betterParryEnabled || !robloxInFocus) {
        EnsureForwardState(scanCode, false);
        return;
    }
    
    if (!antiShakyBlockEnabled) {
        if (g_physHeld.load()) {
            EnsureForwardState(scanCode, true);
        } else {
            EnsureForwardState(scanCode, false);
        }
        return;
    }
    
    // Check parry percentage and shaky block before block indicator logic
    int parryPercentage = ParryBar::GetParryPercentage();
    bool isShakyBlock = ParryBar::IsShakyBlockActive();
    bool blockingCondition = (parryPercentage != 100 || isShakyBlock);
    
    bool prevBlockingCondition = g_blockingConditionActive.exchange(blockingCondition);
    bool prevShakyBlock = g_lastShakyBlock.exchange(isShakyBlock);
    
    // If shaky block just became active, immediately cancel allow current hold and block
    if (isShakyBlock && !prevShakyBlock) {
        g_allowCurrentHold.store(false);
    }
    
    // If blocking condition just became true (due to parry percentage, not shaky block) and key was already held, allow current hold
    // But only if shaky block is not active (shaky block always blocks immediately)
    if (blockingCondition && !prevBlockingCondition && g_physHeld.load() && !isShakyBlock) {
        g_allowCurrentHold.store(true);
    }
    
    // If blocking condition is false, reset allow current hold
    if (!blockingCondition) {
        g_allowCurrentHold.store(false);
    }
    
    // Disable F if parry percentage is not 100% or if shaky block is active
    if (blockingCondition) {
        // If shaky block is active, always block immediately
        // Otherwise, if we're allowing current hold and key is still held, forward it
        bool shouldForward = !isShakyBlock && g_allowCurrentHold.load() && g_physHeld.load();
        
        if (!g_physHeld.load()) {
            EnsureForwardState(scanCode, false);
        } else {
            EnsureForwardState(scanCode, shouldForward);
        }
        return;
    }
    
    ULONGLONG nowMs = GetTickCount64();
    bool blockIndicator = HoldM1::IsBlockIndicatorActive();
    UpdateBlockWindow(blockIndicator, nowMs);
    
    bool wantForward = false;
    if (!blockIndicator) {
        wantForward = true;
    } else {
        bool isM1 = HoldM1::IsM1();
        bool isCritical = HoldM1::CriticalAttack();
        bool isMantra = HoldM1::Mantra();
        if (isM1 || isCritical || isMantra) {
            wantForward = true;
        } else {
            ULONGLONG blockUntil = g_blockUntilMs.load();
            bool blockingNow = (blockUntil != 0 && nowMs < blockUntil);
            wantForward = !blockingNow;
        }
    }
    
    if (!g_physHeld.load()) {
        EnsureForwardState(scanCode, false);
        return;
    }
    
    EnsureForwardState(scanCode, wantForward);
}

bool ShouldIgnoreHookEvent(const KBDLLHOOKSTRUCT* kb) {
    return IsOurInjected(kb);
}



} // namespace LightSpeedReflex

#endif

