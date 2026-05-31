#ifndef ROLLCAST_H
#define ROLLCAST_H

#define NOMINMAX
#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>

namespace RollCast {

static std::atomic<bool> g_running{false};
static std::atomic<bool> g_enabled{false};
static USHORT g_dodgeKeyScanCode = 16;

void SendDodgeKey() {
    if (g_dodgeKeyScanCode == 0) return;
    
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = g_dodgeKeyScanCode;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = g_dodgeKeyScanCode;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

void RollCast() {
    SendDodgeKey();
}

void RollCast(USHORT scanCode) {
    if (g_running && g_enabled) {
        std::thread([]() {
            // Small delay then send dodge key (user already pressed the hotkey)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            SendDodgeKey();
        }).detach();
    }
}

void SetDodgeKeyScanCode(USHORT sc) {
    g_dodgeKeyScanCode = sc;
}

USHORT GetDodgeKeyScanCode() {
    return g_dodgeKeyScanCode;
}

// Enable/disable the mod
void SetEnabled(bool enabled) {
    g_enabled = enabled;
}

// Check if mod is enabled
bool IsEnabled() {
    return g_enabled;
}

void StartRollCast() {
    if (g_running) return; // Already running
    
    g_running = true;
    g_enabled = false;
}

void StopRollCast() {
    if (!g_running) return; // Already stopped
    
    g_running = false;
    g_enabled = false;
}

bool IsRollCastRunning() {
    return g_running;
}

} // namespace RollCast

#endif // ROLLCAST_H