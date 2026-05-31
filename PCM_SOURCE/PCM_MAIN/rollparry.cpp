#ifndef ROLLPARRY_H
#define ROLLPARRY_H

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

namespace RollParry {

// Global state
static std::atomic<bool> g_running{false};
static std::atomic<bool> g_enabled{false};
static std::atomic<bool> g_isWKeyHeld{false};
static std::atomic<bool> g_triggerPending{false};
static std::atomic<bool> g_macroRunning{false};
static std::atomic<bool> g_wKeyPressedByMacro{false};
static std::atomic<bool> g_triggerKeyWasPressed{false};
static std::thread g_macroThread;
static USHORT g_parryKeyScanCode = 33;  // Default F key scan code
static USHORT g_dodgeKeyScanCode = 16;  // Default Q key scan code

// Send parry key (quick tap)
void SendParryKey() {
    if (g_parryKeyScanCode == 0) return;

    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = g_parryKeyScanCode;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = g_parryKeyScanCode;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

    SendInput(2, input, sizeof(INPUT));
}

// Press W key down (hold it) - only if not already held
void WKeyDown() {
    if (!g_isWKeyHeld) {
        WORD vk = 'W';
        WORD scan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);

        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = 0;
        down.ki.wScan = scan;
        down.ki.dwFlags = KEYEVENTF_SCANCODE;
        SendInput(1, &down, sizeof(INPUT));
        g_wKeyPressedByMacro = true;
    }
}

// Release W key (W up) - only if we pressed it down
void WKeyUp() {
    if (g_wKeyPressedByMacro) {
        WORD vk = 'W';
        WORD scan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);

        INPUT up = {};
        up.type = INPUT_KEYBOARD;
        up.ki.wVk = 0;
        up.ki.wScan = scan;
        up.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        SendInput(1, &up, sizeof(INPUT));
        g_wKeyPressedByMacro = false;
    }
}

// Send dodge key (quick tap)
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

// Macro thread
void MacroThread() {
    while (g_running) {
        if (g_triggerPending.exchange(false) && g_enabled && !g_macroRunning) {
            g_macroRunning = true;

            // Execute the sequence:
            // Parry key
            SendParryKey();

            // 5 ms delay
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // w key (press down) - only if W is not already held
            if (!g_isWKeyHeld) {
                WKeyDown();
            }

            // 10 ms delay
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // dodge key
            SendDodgeKey();

            // W up (release W key) - only if we pressed it down
            WKeyUp();

            g_macroRunning = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Public API functions

void SetParryKeyScanCode(USHORT sc) {
    g_parryKeyScanCode = sc;
}

void SetDodgeKeyScanCode(USHORT sc) {
    g_dodgeKeyScanCode = sc;
}

// Enable/disable the mod
void SetEnabled(bool enabled) {
    g_enabled = enabled;
}

// Check if mod is enabled
bool IsEnabled() {
    return g_enabled;
}

// Set W key held state (called by main.cpp)
void SetWKeyHeld(bool held) {
    g_isWKeyHeld = held;
}

// Trigger roll parry action (called by main.cpp)
void TriggerRollParry() {
    if (g_enabled) {
        g_triggerPending = true;
    }
}

bool GetTriggerKeyWasPressed() {
    return g_triggerKeyWasPressed;
}

void SetTriggerKeyWasPressed(bool pressed) {
    g_triggerKeyWasPressed = pressed;
}

// Initialize the RollParry system
void StartRollParry() {
    if (g_running) return; // Already running

    g_running = true;
    g_enabled = false;
    g_triggerPending = false;
    g_macroRunning = false;
    g_wKeyPressedByMacro = false;
    g_isWKeyHeld = false;

    // Start macro thread
    g_macroThread = std::thread(MacroThread);
}

// Stop the RollParry system
void StopRollParry() {
    if (!g_running) return; // Already stopped

    g_running = false;
    g_enabled = false;

    // Wait for thread to finish
    if (g_macroThread.joinable()) {
        g_macroThread.join();
    }
}

// Check if system is running
bool IsRollParryRunning() {
    return g_running;
}

} // namespace RollParry

#endif // ROLLPARRY_H
