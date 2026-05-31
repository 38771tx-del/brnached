#ifndef ROLLSPIT_H
#define ROLLSPIT_H

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

namespace RollSpit {

static std::atomic<bool> g_running{false};
static std::atomic<bool> g_enabled{false};
static std::atomic<bool> g_isWKeyHeld{false};
static std::atomic<bool> g_triggerPending{false};
static std::atomic<bool> g_macroRunning{false};
static std::atomic<bool> g_wKeyPressedByMacro{false};
static std::atomic<bool> g_macroSendingSpit{false};
static std::atomic<bool> g_triggerKeyWasPressed{false};
static std::thread g_macroThread;
static USHORT g_rollKeyScanCode = 16;
static USHORT g_spitKeyScanCode = 20;

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

void SendRollKey() {
    if (g_rollKeyScanCode == 0) return;
    
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = g_rollKeyScanCode;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = g_rollKeyScanCode;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

void SendSpitKey() {
    if (g_spitKeyScanCode == 0) return;
    
    g_macroSendingSpit = true;
    
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = g_spitKeyScanCode;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = g_spitKeyScanCode;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    g_macroSendingSpit = false;
}

void MacroThread() {
    while (g_running) {
        if (g_triggerPending.exchange(false) && g_enabled && !g_macroRunning) {
            g_macroRunning = true;
            
            if (!g_isWKeyHeld) {
                WKeyDown();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            
            SendSpitKey();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            SendRollKey();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            WKeyUp();
            
            g_macroRunning = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SetRollKey(const std::wstring& key) {
}

void SetRollKeyScanCode(USHORT sc) {
    g_rollKeyScanCode = sc;
}

std::wstring GetRollKey() {
    return L"";
}

void SetSpitKey(const std::wstring& key) {
}

void SetSpitKeyScanCode(USHORT sc) {
    g_spitKeyScanCode = sc;
}

std::wstring GetSpitKey() {
    return L"";
}

void SetEnabled(bool enabled) {
    g_enabled = enabled;
}

bool IsEnabled() {
    return g_enabled;
}

void SetWKeyHeld(bool held) {
    g_isWKeyHeld = held;
}

void TriggerRollSpit() {
    if (g_enabled) {
        g_triggerPending = true;
    }
}

bool IsMacroSendingSpit() {
    return g_macroSendingSpit;
}

bool GetTriggerKeyWasPressed() {
    return g_triggerKeyWasPressed;
}

void SetTriggerKeyWasPressed(bool pressed) {
    g_triggerKeyWasPressed = pressed;
}

void StartRollSpit() {
    if (g_running) return;
    
    g_running = true;
    g_enabled = false;
    g_triggerPending = false;
    g_macroRunning = false;
    g_wKeyPressedByMacro = false;
    g_isWKeyHeld = false;
    g_macroSendingSpit = false;
    
    g_macroThread = std::thread(MacroThread);
}

void StopRollSpit() {
    if (!g_running) return;
    
    g_running = false;
    g_enabled = false;
    
    if (g_macroThread.joinable()) {
        g_macroThread.join();
    }
}

bool IsRollSpitRunning() {
    return g_running;
}

}

#endif
