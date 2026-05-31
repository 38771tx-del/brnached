#ifndef GOLDENTONGUE_H
#define GOLDENTONGUE_H

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

namespace GoldenTongue {

static std::atomic<bool> g_running{false};
static std::atomic<bool> g_enabled{false};
static std::atomic<bool> g_triggerPending{false};
static std::atomic<bool> g_macroRunning{false};
static std::thread g_macroThread;

void SendSlashKey() {
    WORD vk = VK_OEM_2;
    WORD scan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = scan;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = scan;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

void SendEKey() {
    WORD vk = 'E';
    WORD scan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = scan;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = scan;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

void SendEnterKey() {
    WORD vk = VK_RETURN;
    WORD scan = (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    
    INPUT input[2] = {};
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = scan;
    input[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = scan;
    input[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

void MacroThread() {
    while (g_running) {
        if (g_triggerPending.exchange(false) && g_enabled && !g_macroRunning) {
            g_macroRunning = true;
            
            SendSlashKey();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            
            SendSlashKey();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            
            SendEKey();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            
            SendEnterKey();
            
            g_macroRunning = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SetEnabled(bool enabled) {
    g_enabled = enabled;
}

bool IsEnabled() {
    return g_enabled;
}

void TriggerGoldenTongue() {
    if (g_enabled) {
        g_triggerPending = true;
    }
}

void StartGoldenTongue() {
    if (g_running) return;
    
    g_running = true;
    g_enabled = false;
    g_triggerPending = false;
    g_macroRunning = false;
    
    g_macroThread = std::thread(MacroThread);
}

void StopGoldenTongue() {
    if (!g_running) return;
    
    g_running = false;
    g_enabled = false;
    
    if (g_macroThread.joinable()) {
        g_macroThread.join();
    }
}

bool IsGoldenTongueRunning() {
    return g_running;
}

}

#endif

