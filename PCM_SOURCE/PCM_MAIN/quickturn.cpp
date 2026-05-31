#ifndef QUICKTURN_H
#define QUICKTURN_H

#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <thread>
#include <chrono>

namespace QuickTurn {

static std::atomic<bool> g_running{false};
static std::atomic<bool> g_enabled{false};
static std::atomic<bool> g_keyCurrentlyPressed{false};
static std::atomic<bool> g_isTurned{false};
static std::atomic<float> g_sensitivity{0.5f};
static std::atomic<float> g_targetDegrees{180.0f};
static std::atomic<bool> g_triggerPending{false};
static std::atomic<bool> g_releasePending{false};
static std::thread g_turnThread;

LONG CalculateTurnMagnitude(float sensitivity, float targetDegrees) {
    const float MOUSE_DELTA_SENSITIVITY = 0.5f;
    float requiredDelta = targetDegrees / (sensitivity * MOUSE_DELTA_SENSITIVITY);
    return static_cast<LONG>(requiredDelta);
}

void SendTurnInput(LONG delta) {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = delta;
    input.mi.dy = 0;
    SendInput(1, &input, sizeof(INPUT));
}

void PerformTurn(LONG magnitude) {
    LONG remaining = magnitude;
    const int segments = 6;
    for (int i = 0; i < segments - 1; ++i) {
        LONG chunk = remaining / (segments - i);
        if (chunk == 0) chunk = remaining > 0 ? 1 : -1;
        SendTurnInput(chunk);
        remaining -= chunk;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (remaining != 0) {
        SendTurnInput(remaining);
    }
}

void TurnThread() {
    while (g_running) {
        if (g_triggerPending.exchange(false) && g_enabled && !g_isTurned) {
            float sensitivity = g_sensitivity.load();
            float targetDegrees = g_targetDegrees.load();
            LONG turnMagnitude = CalculateTurnMagnitude(sensitivity, targetDegrees);
            PerformTurn(turnMagnitude);
            g_isTurned = true;
        }
        
        if (g_releasePending.exchange(false) && g_enabled && g_isTurned) {
            float sensitivity = g_sensitivity.load();
            float targetDegrees = g_targetDegrees.load();
            LONG turnMagnitude = CalculateTurnMagnitude(sensitivity, targetDegrees);
            PerformTurn(-turnMagnitude);
            g_isTurned = false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void SetEnabled(bool enabled) {
    g_enabled = enabled;
}

bool IsEnabled() {
    return g_enabled;
}

void SetSensitivity(float sensitivity) {
    g_sensitivity = sensitivity;
}

void SetTargetDegrees(float degrees) {
    g_targetDegrees = degrees;
}

void TriggerTurn() {
    if (g_enabled && !g_isTurned) {
        g_triggerPending = true;
    }
}

void ReleaseTurn() {
    if (g_enabled && g_isTurned) {
        g_releasePending = true;
    }
}

void ForceResetState() {
    g_isTurned = false;
    g_triggerPending = false;
    g_releasePending = false;
}

void StartQuickTurn() {
    if (g_running) return;
    
    g_running = true;
    g_enabled = false;
    g_keyCurrentlyPressed = false;
    g_isTurned = false;
    g_triggerPending = false;
    g_releasePending = false;
    
    g_turnThread = std::thread(TurnThread);
}

void StopQuickTurn() {
    if (!g_running) return;
    
    g_running = false;
    g_enabled = false;
    
    if (g_turnThread.joinable()) {
        g_turnThread.join();
    }
}

bool IsQuickTurnRunning() {
    return g_running;
}

}

#endif
