#ifndef MAPCAST_H
#define MAPCAST_H

#define NOMINMAX
#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>

namespace MapCast {

static std::atomic<bool> g_running{false};
static std::atomic<bool> g_enabled{false};
static std::atomic<bool> g_triggerPending{false};
static std::atomic<bool> g_macroRunning{false};
static std::thread g_macroThread;
static USHORT g_openMapKeyScanCode = 50;

void SendOpenMapKey() {
    if (g_openMapKeyScanCode == 0) return;
    
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = g_openMapKeyScanCode;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = g_openMapKeyScanCode;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

void RunMacro() {
    if (g_macroRunning) return;
    g_macroRunning = true;
    
    SendOpenMapKey();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    SendOpenMapKey();
    
    g_macroRunning = false;
}

void MacroThread() {
    while (g_running) {
        if (g_triggerPending.exchange(false) && g_enabled && !g_macroRunning) {
            RunMacro();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SetOpenMapKeyScanCode(USHORT sc) {
    g_openMapKeyScanCode = sc;
}

USHORT GetOpenMapKeyScanCode() {
    return g_openMapKeyScanCode;
}

void SetEnabled(bool enabled) {
    g_enabled = enabled;
}

bool IsEnabled() {
    return g_enabled;
}

void TriggerMapCast() {
    if (g_enabled) {
        g_triggerPending = true;
    }
}

void StartMapCast() {
    if (g_running) return;
    
    g_running = true;
    g_enabled = false;
    g_triggerPending = false;
    g_macroRunning = false;
    
    g_macroThread = std::thread(MacroThread);
}

void StopMapCast() {
    if (!g_running) return;
    
    g_running = false;
    g_enabled = false;
    
    if (g_macroThread.joinable()) {
        g_macroThread.join();
    }
}

bool IsMapCastRunning() {
    return g_running;
}

} // namespace MapCast

#endif // MAPCAST_H