#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <windowsx.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <tlhelp32.h>
#include <fstream>
#include <shlobj.h>
#include "winrt.cpp"
#include "parrybar.cpp"
#include "holdm1.cpp"
#include "blur.cpp"
#include "rollm1.cpp"
#include "rollcritical.cpp"
#include "goldentongue.cpp"
#include "rollparry.cpp"
#include "rollspit.cpp"
#include "rollcast.cpp"
#include "mapcast.cpp"
#include "keystrokes.cpp"
#include "quickturn.cpp"
#include "cps.cpp"
void HudLayoutSaveKeystrokes(int x, int y, float scale, bool hasPos);
void HudLayoutSaveCPS(int x, int y, float scale, bool hasPos);
void HudLayoutSaveParryBar(int x, int y, float scale, bool hasPos);
#include "edithud.cpp"
#include "crosshair.cpp"
#include "motionblur.cpp"
#include "zoom.cpp"
#include "traytrip.cpp"
#include "lightspeedreflex.cpp"

#define NANOSVG_IMPLEMENTATION
#include "svg.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "comdlg32.lib")

// Global state for parry bar mod
static bool g_parryBarRunning = false;
static bool g_parryBarPendingStart = false;
static int g_cursorShowIncrements = 0;

// Global state for Roll M1 mod
static bool g_rollM1Running = false;
static bool g_rollM1PendingStart = false;
static bool g_rollCriticalRunning = false;
static bool g_goldenTongueRunning = false;
static bool g_goldenTonguePendingStart = false;
static bool g_rollCriticalPendingStart = false;
static bool g_rollParryRunning = false;
static bool g_rollParryPendingStart = false;
static bool g_quickTurnRunning = false;
static bool g_quickTurnPendingStart = false;
static bool g_holdM1Running = false;
static bool g_holdM1PendingStart = false;
static bool g_rollSpitRunning = false;
static bool g_rollSpitPendingStart = false;
static bool g_rollCastRunning = false;
static bool g_rollCastPendingStart = false;
static bool g_mapCastRunning = false;
static bool g_mapCastPendingStart = false;
static bool g_keystrokesRunning = false;
static bool g_keystrokesPendingStart = false;
static bool g_cpsRunning = false;
static bool g_cpsPendingStart = false;
static bool g_crosshairRunning = false;
static bool g_crosshairPendingStart = false;
static bool g_crosshairPendingDisable = false;
static bool g_motionBlurRunning = false;
static bool g_motionBlurPendingStart = false;
static bool g_zoomRunning = false;
static bool g_zoomPendingStart = false;
static bool g_editHudPendingStart = false;
static bool g_gammaRunning = false;
static std::atomic<bool> g_gammaEnabled{false};
static float g_currentGamma = 1.0f;
static std::atomic<float> g_gammaTarget{1.0f};
static std::atomic<bool> g_gammaMonitorRunning{false};
static std::thread g_gammaMonitorThread;

static bool SetGamma(float gamma) {
    if (gamma < 0.1f) gamma = 0.1f;
    if (gamma > 4.0f) gamma = 4.0f;
    
    HDC hDC = GetDC(0);
    if (!hDC) return false;
    
    WORD gammaArray[3][256] = {0};
    
    for (int i = 0; i < 256; i++) {
        double val = std::pow((double)i / 255.0, 1.0 / gamma) * 65535.0;
        if (val > 65535.0) val = 65535.0;
        if (val < 0.0) val = 0.0;
        
        WORD wordVal = (WORD)std::round(val);
        gammaArray[0][i] = wordVal;
        gammaArray[1][i] = wordVal;
        gammaArray[2][i] = wordVal;
    }
    
    BOOL success = SetDeviceGammaRamp(hDC, (LPVOID)gammaArray);
    ReleaseDC(0, hDC);
    
    if (success) {
        g_currentGamma = gamma;
    }
    
    return success != FALSE;
}

static void StartGammaMonitor() {
    if (g_gammaMonitorRunning.load()) return;
    g_gammaMonitorRunning.store(true);
    g_gammaMonitorThread = std::thread([]() {
        bool wasApplied = false;
        while (g_gammaMonitorRunning.load()) {
            HWND robloxWindow = WinRTCapture::FindRobloxWindow();
            HWND fg = GetForegroundWindow();
            bool isRobloxActive = (robloxWindow && (fg == robloxWindow || GetParent(fg) == robloxWindow));
            bool shouldApply = g_gammaEnabled.load() && isRobloxActive;
            float target = g_gammaTarget.load();
            if (shouldApply) {
                SetGamma(target);
                wasApplied = true;
            } else if (wasApplied) {
                SetGamma(1.0f);
                wasApplied = false;
            }
            Sleep(50);
        }
        SetGamma(1.0f);
    });
}

static void StopGammaMonitor() {
    g_gammaEnabled.store(false);
    if (g_gammaMonitorRunning.exchange(false)) {
        if (g_gammaMonitorThread.joinable()) g_gammaMonitorThread.join();
    }
    SetGamma(1.0f);
    g_gammaRunning = false;
}

// Lightspeed reflex globals
DWORD g_lightspeedM1PressTime = 0;
static bool g_lightspeedSequenceRunning = false;

// Add these:
extern std::atomic<bool> g_forceFreshBlurFrame;

struct ModCard {
    RECT rect;
    std::wstring title;
    std::wstring description;
    bool hovered = false;
    bool pressed = false;
    bool enabled = false;
    float borderOpacity = 0.0f;  // For smooth border fade animation
    bool animatingBorder = false;
    float iconOpacity = 0.0f;
    bool animatingIcon = false;
    float hoverLift = 0.0f;
};

class PistachioCreamMacro {
private:
    HWND m_hwnd = nullptr;
    ID2D1Factory* m_d2dFactory = nullptr;
    ID2D1DCRenderTarget* m_renderTarget = nullptr;
    HDC m_memDC = nullptr;
    HBITMAP m_dibBitmap = nullptr;
    int m_bufferWidth = 0;
    int m_bufferHeight = 0;
    IDWriteFactory* m_writeFactory = nullptr;
    IDWriteTextFormat* m_titleFormat = nullptr;
    IDWriteTextFormat* m_descFormat = nullptr;
    IDWriteTextFormat* m_tagFormat = nullptr;
    IDWriteTextFormat* m_searchFormat = nullptr;
    IDWriteTextFormat* m_closeFormat = nullptr;
    
    ID2D1SolidColorBrush* m_backgroundBrush = nullptr;
    ID2D1SolidColorBrush* m_sidebarBrush = nullptr;
    ID2D1SolidColorBrush* m_cardBrush = nullptr;
    ID2D1SolidColorBrush* m_textBrush = nullptr;
    ID2D1SolidColorBrush* m_descBrush = nullptr;
    ID2D1SolidColorBrush* m_searchBrush = nullptr;
    ID2D1SolidColorBrush* m_whiteBrush = nullptr;
    ID2D1SolidColorBrush* m_iconBrush = nullptr;
    ID2D1SolidColorBrush* m_panel2Brush = nullptr;
    
    std::vector<ModCard> m_modCards;
    RECT m_closeButtonRect;
    RECT m_searchRect;
    RECT m_logoRect;
    RECT m_exitButtonRect;
    RECT m_panelsButtonRect;
    bool m_panelsButtonHovered = false;
    float m_panelsButtonScale = 0.85f;
    bool m_exitButtonHovered = false;
    float m_exitButtonScale = 0.85f;
    bool m_modsIconHovered = false;
    float m_modsIconScale = 1.15f;
    bool m_userIconHovered = false;
    float m_userIconScale = 1.0f;
    bool m_settingsIconHovered = false;
    float m_settingsIconScale = 1.0f;
    RECT m_settingsRect;
    RECT m_modsRect;
    RECT m_profileRect;
    RECT m_sliderTrackRect;
    RECT m_sliderKnobRect;
    RECT m_crosshairFileButtonRect;
    std::wstring m_crosshairFilePath = L"No file selected";
    std::wstring m_searchText = L"";
    bool m_searchFocused = false;
    int m_cursorPosition = 0;
    bool m_cursorVisible = true;
    int m_cursorBlinkTimer = 0;
    bool m_searchAnimatingBorder = false;
    float m_searchBorderOpacity = 0.05f; // Start at 0.05 (normal state)
    RECT m_cameraLockSetButtonRect{};
    std::wstring m_cameraLockKey = L"Shift";  // Default Camera Lock key
    USHORT m_cameraLockTriggerScanCode = 42;  // Scan code for LShift key
    bool m_cameraLockCapturing = false;
    RECT m_crosshairScaleTrackRect;
    RECT m_crosshairScaleKnobRect;
    bool m_draggingCrosshairScale = false;
    float m_crosshairScale = 1.0f;
    bool m_showSettings = false;
    bool m_showProfile = false;
    
    // Auto Wisp
    struct WispCharBox {
        wchar_t ch = 0;
        RECT rect{};
        bool focused = false;
    };
    struct WispSequence {
        std::vector<WispCharBox> charBoxes;
        std::wstring key = L"F1";
        USHORT scanCode = 0;
        RECT setKeyButtonRect{};
        RECT plusButtonRect{};
        RECT deleteButtonRect{};
        RECT refreshButtonRect{};
        bool capturingKey = false;
        bool isEditing = false; // True when user is typing the sequence
        int focusedCharIndex = -1; // Which char box is focused (-1 means none)
    };
    std::vector<WispSequence> m_wispSequences;
    int m_focusedWispSequenceIndex = -1;
    int m_wispCursorBlinkTimer = 0;
    bool m_wispCursorVisible = true;
    bool m_draggingSlider = false;
    float m_blurSlider = 0.3f;
    RECT m_motionBlurTrackRect;
    RECT m_motionBlurKnobRect;
    bool m_draggingMotionBlur = false;
    float m_motionBlurSlider = 0.25f;
    RECT m_quickTurnSensitivityTrackRect;
    RECT m_quickTurnSensitivityKnobRect;
    bool m_draggingQuickTurnSensitivity = false;
    float m_quickTurnSensitivitySlider = 0.5f;
    RECT m_gammaSliderTrackRect;
    RECT m_gammaSliderKnobRect;
    bool m_draggingGammaSlider = false;
    float m_gammaSlider = 0.2308f;
    RECT m_quickTurnDegreesTrackRect;
    RECT m_quickTurnDegreesKnobRect;
    bool m_draggingQuickTurnDegrees = false;
    float m_quickTurnDegreesSlider = 0.5f;
    
    std::atomic<bool> m_showUI{ false };
    std::atomic<bool> m_isAnimating{ false };
    std::atomic<float> m_opacity{ 1.0f };
    std::atomic<bool> m_running{ true };
    HHOOK m_hHook = NULL;
    static PistachioCreamMacro* s_instance;
    
    static constexpr float kContentWidth = 760.0f;
    static constexpr float kContentHeight = 550.0f;
    static constexpr float kWindowPadding = 120.0f;
    static constexpr float kWindowWidth = kContentWidth + kWindowPadding;
    static constexpr float kWindowHeight = kContentHeight + kWindowPadding;
    
    float m_scale = 0.3f;
    int m_animationStep = 0;
    bool m_animatingIn = false;
    bool m_animatingOut = false;
    bool m_exitAfterHide = false;
	RECT m_rollM1SetButtonRect{};
	std::wstring m_rollM1Key = L"T";  // Default trigger key for Roll M1
	USHORT m_rollM1TriggerScanCode = 20;  // Scan code for RollM1 trigger key
	RECT m_goldenTongueSetButtonRect{};
	std::wstring m_goldenTongueKey = L"U";  // Default trigger key for Golden Tongue
	USHORT m_goldenTongueTriggerScanCode = 22;  // Scan code for Golden Tongue trigger key
	bool m_rollCritical = false;  // Roll Critical toggle
	RECT m_rollCriticalToggleRect{};
	RECT m_rollCritSetButtonRect{};
	std::wstring m_rollCritKey = L"C";  // Default Roll Crit key
	USHORT m_rollCritScanCode = 46;  // Scan code for C key
	RECT m_rollSpitTriggerSetButtonRect{};
	RECT m_rollSpitSpitSetButtonRect{};
	std::wstring m_rollSpitTriggerKey = L"Y";
	USHORT m_rollSpitTriggerScanCode = 21;
	std::wstring m_rollSpitSpitKey = L"T";
	USHORT m_rollSpitSpitScanCode = 20;
	RECT m_rollCastSetButtonRect{};
	std::wstring m_rollCastKey = L"F1";  // Default Roll Cast key
	RECT m_dodgeSetButtonRect{};
	std::wstring m_dodgeKey = L"Q";  // Default dodge key for Roll M1
	USHORT m_dodgeScanCode = 16; // Default scan code for Q (matches Keystrokes default)
	RECT m_parrySetButtonRect{};
	std::wstring m_parryKey = L"F";  // Default block/parry key
	USHORT m_parryScanCode = 33;  // Scan code for F key
	RECT m_openMapSetButtonRect{};
	std::wstring m_openMapKey = L"M";  // Default Open Map key
	USHORT m_openMapTriggerScanCode = 50;  // Scan code for M key
	RECT m_criticalAttackSetButtonRect{};
	std::wstring m_criticalAttackKey = L"R";  // Default Critical Attack key
	USHORT m_criticalAttackTriggerScanCode = 19;  // Scan code for R key
	int m_criticalAttackMouseButtonIndex = 0;  // 0 = keyboard, 1-5 = mouse button
	RECT m_zoomSetButtonRect{};
	std::wstring m_zoomKey = L"Z";
	USHORT m_zoomTriggerScanCode = 44;
	RECT m_quickTurnSetButtonRect{};
	std::wstring m_quickTurnKey = L"B";
	USHORT m_quickTurnTriggerScanCode = 48;
	int m_quickTurnMouseButtonIndex = 0;
	bool m_quickTurnSensitivityAutoUpdate = true;
	bool m_quickTurnToggleMode = false;
	RECT m_quickTurnToggleRect{};

	bool m_holdM1Enabled = true;
	RECT m_holdM1ToggleRect{};

	bool m_rollParry = false;
	RECT m_rollParryToggleRect{};
	RECT m_rollParrySetButtonRect{};
	std::wstring m_rollParryKey = L"X";
	USHORT m_rollParryScanCode = 45;  // Scan code for X key
	bool m_betterParryAntiShakyBlock = false;
	RECT m_betterParryAntiShakyBlockToggleRect{};
	bool m_betterParryLightspeedReflex = true;
	RECT m_betterParryLightspeedReflexToggleRect{};
	bool m_betterParryShouldParry = true;
	RECT m_betterParryShouldParryToggleRect{};

	RECT m_hotbarSlotSetButtonRect[10]{};
	std::wstring m_hotbarSlotKeys[10] = {L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"0"};
	bool m_hotbarSlotEnabled[10] = {false, false, false, false, false, false, false, false, false, false};
	RECT m_hotbarSlotToggleRect[10]{};
	RECT m_hotbarSlotIconRect[10]{};
	bool m_hotbarSlotMapCastEnabled[10] = {false, false, false, false, false, false, false, false, false, false};
	RECT m_hotbarSlotMapCastToggleRect[10]{};
	RECT m_hotbarSlotMapCastIconRect[10]{};
	bool m_keyCaptureActive = false;
	bool m_rollM1Capturing = false;
	bool m_rollCritCapturing = false;
	bool m_goldenTongueCapturing = false;
	bool m_rollSpitTriggerCapturing = false;
	bool m_rollSpitSpitCapturing = false;
	bool m_rollCastCapturing = false;
	bool m_dodgeCapturing = false;
	bool m_parryCapturing = false;
	bool m_rollParryCapturing = false;
	bool m_openMapCapturing = false;
	bool m_criticalAttackCapturing = false;
	bool m_zoomCapturing = false;
	bool m_quickTurnCapturing = false;
	bool m_hotbarSlotCapturing[10] = {false, false, false, false, false, false, false, false, false, false};
	float m_settingsScrollOffset = 0.0f;
	float m_settingsMaxScrollOffset = 0.0f;
	float m_modsScrollOffset = 0.0f;
	float m_modsMaxScrollOffset = 0.0f;
	USHORT m_prevMouseFlags = 0;
	USHORT m_prevButtonFlags = 0;
	USHORT m_prevButtonData = 0;
	bool m_wKeyHeld = false;  // Track W key state for Roll M1
	bool m_rollM1TriggerKeyHeld = false;  // Track Roll M1 trigger key state
	bool m_rollSpitTriggerKeyHeld = false;
	bool m_quickTurnTriggerKeyHeld = false;
	bool m_goldenTongueTriggerKeyHeld = false;
	bool m_rollParryTriggerKeyHeld = false;

	static constexpr UINT kSettingsSaveTimerId = 7;
	static constexpr DWORD kSettingsSaveDebounceMs = 500;
	bool m_settingsDirty = false;
	DWORD m_lastSettingsChangeTick = 0;
	bool m_hudKeystrokesHasPos = false;
	int m_hudKeystrokesX = 0;
	int m_hudKeystrokesY = 0;
	float m_hudKeystrokesScale = 1.0f;
	bool m_hudCPSHasPos = false;
	int m_hudCPSX = 0;
	int m_hudCPSY = 0;
	float m_hudCPSScale = 1.0f;
	bool m_hudParryBarHasPos = false;
	int m_hudParryBarX = 0;
	int m_hudParryBarY = 0;
	float m_hudParryBarScale = 1.0f;
	
	static constexpr UINT kSettingsLoadedMessage = WM_APP + 41;
	
	struct SavedWispSequence {
		std::wstring key;
		USHORT scanCode = 0;
		std::wstring sequence;
	};
	
	struct SettingsSnapshot {
		std::vector<bool> modCardEnabled;
		float blurSlider = 0.0f;
		float motionBlurSlider = 0.0f;
		float quickTurnSensitivitySlider = 0.0f;
		float gammaSlider = 0.0f;
		float quickTurnDegreesSlider = 0.0f;
		float crosshairScale = 0.0f;
		std::wstring hotbarSlotKeys[10];
		bool hotbarSlotEnabled[10]{};
		bool hotbarSlotMapCastEnabled[10]{};
		std::vector<SavedWispSequence> wispSequences;
		std::wstring rollM1Key;
		USHORT rollM1TriggerScanCode = 0;
		std::wstring goldenTongueKey;
		USHORT goldenTongueTriggerScanCode = 0;
		std::wstring rollCritKey;
		USHORT rollCritScanCode = 0;
		std::wstring rollSpitTriggerKey;
		USHORT rollSpitTriggerScanCode = 0;
		std::wstring rollSpitSpitKey;
		USHORT rollSpitSpitScanCode = 0;
		std::wstring rollCastKey;
		std::wstring dodgeKey;
		USHORT dodgeScanCode = 0;
		std::wstring parryKey;
		USHORT parryScanCode = 0;
		std::wstring openMapKey;
		USHORT openMapTriggerScanCode = 0;
		std::wstring criticalAttackKey;
		USHORT criticalAttackTriggerScanCode = 0;
		int criticalAttackMouseButtonIndex = 0;
		std::wstring zoomKey;
		USHORT zoomTriggerScanCode = 0;
		std::wstring quickTurnKey;
		USHORT quickTurnTriggerScanCode = 0;
		int quickTurnMouseButtonIndex = 0;
		std::wstring rollParryKey;
		USHORT rollParryScanCode = 0;
		std::wstring cameraLockKey;
		USHORT cameraLockTriggerScanCode = 0;
		std::wstring crosshairFilePath;
		bool rollCritical = false;
		bool rollParry = false;
		bool betterParryAntiShakyBlock = false;
		bool betterParryLightspeedReflex = false;
		bool betterParryShouldParry = false;
		bool holdM1Enabled = false;
		bool quickTurnToggleMode = false;
		bool quickTurnSensitivityAutoUpdate = false;
		bool hudKeystrokesHasPos = false;
		int hudKeystrokesX = 0;
		int hudKeystrokesY = 0;
		float hudKeystrokesScale = 1.0f;
		bool hudCPSHasPos = false;
		int hudCPSX = 0;
		int hudCPSY = 0;
		float hudCPSScale = 1.0f;
		bool hudParryBarHasPos = false;
		int hudParryBarX = 0;
		int hudParryBarY = 0;
		float hudParryBarScale = 1.0f;
	};
	
	std::thread m_registryWorker;
	std::mutex m_registryMutex;
	std::condition_variable m_registryCv;
	bool m_registryStop = false;
	bool m_savePending = false;
	bool m_loadedPending = false;
	SettingsSnapshot m_loadedSnapshot;
	bool m_initialSettingsApplied = false;

	void MarkSettingsDirty() {
		m_settingsDirty = true;
		m_lastSettingsChangeTick = GetTickCount();
		if (m_hwnd) {
			SetTimer(m_hwnd, kSettingsSaveTimerId, 150, nullptr);
		}
	}
	
	SettingsSnapshot CaptureSettingsSnapshot() {
		SettingsSnapshot s;
		s.modCardEnabled.resize(m_modCards.size());
		for (size_t i = 0; i < m_modCards.size(); i++) s.modCardEnabled[i] = m_modCards[i].enabled;
		s.blurSlider = m_blurSlider;
		s.motionBlurSlider = m_motionBlurSlider;
		s.quickTurnSensitivitySlider = m_quickTurnSensitivitySlider;
		s.gammaSlider = m_gammaSlider;
		s.quickTurnDegreesSlider = m_quickTurnDegreesSlider;
		s.crosshairScale = m_crosshairScale;
		for (int i = 0; i < 10; i++) {
			s.hotbarSlotKeys[i] = m_hotbarSlotKeys[i];
			s.hotbarSlotEnabled[i] = m_hotbarSlotEnabled[i];
			s.hotbarSlotMapCastEnabled[i] = m_hotbarSlotMapCastEnabled[i];
		}
		s.wispSequences.clear();
		s.wispSequences.reserve(m_wispSequences.size());
		for (size_t i = 0; i < m_wispSequences.size(); i++) {
			SavedWispSequence ws;
			ws.key = m_wispSequences[i].key;
			ws.scanCode = m_wispSequences[i].scanCode;
			for (const auto& cb : m_wispSequences[i].charBoxes) {
				if (cb.ch != 0) ws.sequence.push_back(cb.ch);
			}
			s.wispSequences.push_back(std::move(ws));
		}
		s.rollM1Key = m_rollM1Key;
		s.rollM1TriggerScanCode = m_rollM1TriggerScanCode;
		s.goldenTongueKey = m_goldenTongueKey;
		s.goldenTongueTriggerScanCode = m_goldenTongueTriggerScanCode;
		s.rollCritKey = m_rollCritKey;
		s.rollCritScanCode = m_rollCritScanCode;
		s.rollSpitTriggerKey = m_rollSpitTriggerKey;
		s.rollSpitTriggerScanCode = m_rollSpitTriggerScanCode;
		s.rollSpitSpitKey = m_rollSpitSpitKey;
		s.rollSpitSpitScanCode = m_rollSpitSpitScanCode;
		s.rollCastKey = m_rollCastKey;
		s.dodgeKey = m_dodgeKey;
		s.dodgeScanCode = m_dodgeScanCode;
		s.parryKey = m_parryKey;
		s.parryScanCode = m_parryScanCode;
		s.openMapKey = m_openMapKey;
		s.openMapTriggerScanCode = m_openMapTriggerScanCode;
		s.criticalAttackKey = m_criticalAttackKey;
		s.criticalAttackTriggerScanCode = m_criticalAttackTriggerScanCode;
		s.criticalAttackMouseButtonIndex = m_criticalAttackMouseButtonIndex;
		s.zoomKey = m_zoomKey;
		s.zoomTriggerScanCode = m_zoomTriggerScanCode;
		s.quickTurnKey = m_quickTurnKey;
		s.quickTurnTriggerScanCode = m_quickTurnTriggerScanCode;
		s.quickTurnMouseButtonIndex = m_quickTurnMouseButtonIndex;
		s.rollParryKey = m_rollParryKey;
		s.rollParryScanCode = m_rollParryScanCode;
		s.cameraLockKey = m_cameraLockKey;
		s.cameraLockTriggerScanCode = m_cameraLockTriggerScanCode;
		s.crosshairFilePath = m_crosshairFilePath;
		s.rollCritical = m_rollCritical;
		s.rollParry = m_rollParry;
		s.betterParryAntiShakyBlock = m_betterParryAntiShakyBlock;
		s.betterParryLightspeedReflex = m_betterParryLightspeedReflex;
		s.betterParryShouldParry = m_betterParryShouldParry;
		s.holdM1Enabled = m_holdM1Enabled;
		s.quickTurnToggleMode = m_quickTurnToggleMode;
		s.quickTurnSensitivityAutoUpdate = m_quickTurnSensitivityAutoUpdate;
		s.hudKeystrokesHasPos = m_hudKeystrokesHasPos;
		s.hudKeystrokesX = m_hudKeystrokesX;
		s.hudKeystrokesY = m_hudKeystrokesY;
		s.hudKeystrokesScale = m_hudKeystrokesScale;
		s.hudCPSHasPos = m_hudCPSHasPos;
		s.hudCPSX = m_hudCPSX;
		s.hudCPSY = m_hudCPSY;
		s.hudCPSScale = m_hudCPSScale;
		s.hudParryBarHasPos = m_hudParryBarHasPos;
		s.hudParryBarX = m_hudParryBarX;
		s.hudParryBarY = m_hudParryBarY;
		s.hudParryBarScale = m_hudParryBarScale;
		return s;
	}
	
	void ApplySettingsSnapshot(const SettingsSnapshot& s) {
		for (size_t i = 0; i < m_modCards.size() && i < s.modCardEnabled.size(); i++) {
			m_modCards[i].enabled = s.modCardEnabled[i];
		}
		for (auto& card : m_modCards) {
			card.borderOpacity = card.enabled ? 1.0f : 0.0f;
			card.animatingBorder = false;
			card.iconOpacity = card.enabled ? 1.0f : 0.0f;
			card.animatingIcon = false;
		}
		m_blurSlider = s.blurSlider;
		m_motionBlurSlider = s.motionBlurSlider;
		m_quickTurnSensitivitySlider = s.quickTurnSensitivitySlider;
		m_gammaSlider = s.gammaSlider;
		m_quickTurnDegreesSlider = s.quickTurnDegreesSlider;
		m_crosshairScale = s.crosshairScale;
		for (int i = 0; i < 10; i++) {
			m_hotbarSlotKeys[i] = s.hotbarSlotKeys[i];
			m_hotbarSlotEnabled[i] = s.hotbarSlotEnabled[i];
			m_hotbarSlotMapCastEnabled[i] = s.hotbarSlotMapCastEnabled[i];
		}
		m_wispSequences.clear();
		m_wispSequences.reserve(s.wispSequences.size());
		for (const auto& ws : s.wispSequences) {
			WispSequence seq;
			seq.key = ws.key;
			seq.scanCode = ws.scanCode;
			for (wchar_t c : ws.sequence) {
				WispCharBox cb;
				cb.ch = c;
				seq.charBoxes.push_back(cb);
			}
			m_wispSequences.push_back(std::move(seq));
		}
		m_rollM1Key = s.rollM1Key;
		m_rollM1TriggerScanCode = s.rollM1TriggerScanCode;
		m_goldenTongueKey = s.goldenTongueKey;
		m_goldenTongueTriggerScanCode = s.goldenTongueTriggerScanCode;
		m_rollCritKey = s.rollCritKey;
		m_rollCritScanCode = s.rollCritScanCode;
		m_rollSpitTriggerKey = s.rollSpitTriggerKey;
		m_rollSpitTriggerScanCode = s.rollSpitTriggerScanCode;
		m_rollSpitSpitKey = s.rollSpitSpitKey;
		m_rollSpitSpitScanCode = s.rollSpitSpitScanCode;
		m_rollCastKey = s.rollCastKey;
		m_dodgeKey = s.dodgeKey;
		m_dodgeScanCode = s.dodgeScanCode;
		m_parryKey = s.parryKey;
		m_parryScanCode = s.parryScanCode;
		m_openMapKey = s.openMapKey;
		m_openMapTriggerScanCode = s.openMapTriggerScanCode;
		m_criticalAttackKey = s.criticalAttackKey;
		m_criticalAttackTriggerScanCode = s.criticalAttackTriggerScanCode;
		m_criticalAttackMouseButtonIndex = s.criticalAttackMouseButtonIndex;
		m_zoomKey = s.zoomKey;
		m_zoomTriggerScanCode = s.zoomTriggerScanCode;
		m_quickTurnKey = s.quickTurnKey;
		m_quickTurnTriggerScanCode = s.quickTurnTriggerScanCode;
		m_quickTurnMouseButtonIndex = s.quickTurnMouseButtonIndex;
		m_rollParryKey = s.rollParryKey;
		m_rollParryScanCode = s.rollParryScanCode;
		m_cameraLockKey = s.cameraLockKey;
		m_cameraLockTriggerScanCode = s.cameraLockTriggerScanCode;
		m_crosshairFilePath = s.crosshairFilePath;
		m_rollCritical = s.rollCritical;
		m_rollParry = s.rollParry;
		m_betterParryAntiShakyBlock = s.betterParryAntiShakyBlock;
		m_betterParryLightspeedReflex = s.betterParryLightspeedReflex;
		m_betterParryShouldParry = s.betterParryShouldParry;
		m_holdM1Enabled = s.holdM1Enabled;
		m_quickTurnToggleMode = s.quickTurnToggleMode;
		m_quickTurnSensitivityAutoUpdate = s.quickTurnSensitivityAutoUpdate;
		m_hudKeystrokesHasPos = s.hudKeystrokesHasPos;
		m_hudKeystrokesX = s.hudKeystrokesX;
		m_hudKeystrokesY = s.hudKeystrokesY;
		m_hudKeystrokesScale = s.hudKeystrokesScale;
		m_hudCPSHasPos = s.hudCPSHasPos;
		m_hudCPSX = s.hudCPSX;
		m_hudCPSY = s.hudCPSY;
		m_hudCPSScale = s.hudCPSScale;
		m_hudParryBarHasPos = s.hudParryBarHasPos;
		m_hudParryBarX = s.hudParryBarX;
		m_hudParryBarY = s.hudParryBarY;
		m_hudParryBarScale = s.hudParryBarScale;
	}
	
	void WriteSettingsSnapshotToRegistry(const SettingsSnapshot& s) {
		HKEY hKey;
		if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_MENU", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
			return;
		}
		for (size_t i = 0; i < s.modCardEnabled.size(); i++) {
			wchar_t valueName[64];
			swprintf_s(valueName, L"ModCard_%zu_Enabled", i);
			DWORD enabled = s.modCardEnabled[i] ? 1 : 0;
			RegSetValueExW(hKey, valueName, 0, REG_DWORD, (BYTE*)&enabled, sizeof(DWORD));
		}
		RegSetValueExW(hKey, L"BlurSlider", 0, REG_BINARY, (BYTE*)&s.blurSlider, sizeof(float));
		RegSetValueExW(hKey, L"MotionBlurSlider", 0, REG_BINARY, (BYTE*)&s.motionBlurSlider, sizeof(float));
		RegSetValueExW(hKey, L"QuickTurnSensitivitySlider", 0, REG_BINARY, (BYTE*)&s.quickTurnSensitivitySlider, sizeof(float));
		RegSetValueExW(hKey, L"GammaSlider", 0, REG_BINARY, (BYTE*)&s.gammaSlider, sizeof(float));
		RegSetValueExW(hKey, L"QuickTurnDegreesSlider", 0, REG_BINARY, (BYTE*)&s.quickTurnDegreesSlider, sizeof(float));
		RegSetValueExW(hKey, L"CrosshairScale", 0, REG_BINARY, (BYTE*)&s.crosshairScale, sizeof(float));
		for (int i = 0; i < 10; i++) {
			wchar_t keyName[64];
			swprintf_s(keyName, L"HotbarSlot_%d_Key", i);
			RegSetValueExW(hKey, keyName, 0, REG_SZ, (BYTE*)s.hotbarSlotKeys[i].c_str(), (DWORD)((s.hotbarSlotKeys[i].length() + 1) * sizeof(wchar_t)));
			swprintf_s(keyName, L"HotbarSlot_%d_Enabled", i);
			DWORD enabled = s.hotbarSlotEnabled[i] ? 1 : 0;
			RegSetValueExW(hKey, keyName, 0, REG_DWORD, (BYTE*)&enabled, sizeof(DWORD));
			swprintf_s(keyName, L"HotbarSlot_%d_MapCastEnabled", i);
			DWORD mapCastEnabled = s.hotbarSlotMapCastEnabled[i] ? 1 : 0;
			RegSetValueExW(hKey, keyName, 0, REG_DWORD, (BYTE*)&mapCastEnabled, sizeof(DWORD));
		}
		DWORD wispSequenceCount = (DWORD)s.wispSequences.size();
		RegSetValueExW(hKey, L"WispSequenceCount", 0, REG_DWORD, (BYTE*)&wispSequenceCount, sizeof(DWORD));
		for (size_t i = 0; i < s.wispSequences.size(); i++) {
			wchar_t baseName[64];
			swprintf_s(baseName, L"WispSequence_%zu", i);
			std::wstring keyName = std::wstring(baseName) + L"_Key";
			RegSetValueExW(hKey, keyName.c_str(), 0, REG_SZ, (BYTE*)s.wispSequences[i].key.c_str(), (DWORD)((s.wispSequences[i].key.length() + 1) * sizeof(wchar_t)));
			keyName = std::wstring(baseName) + L"_ScanCode";
			DWORD seqScan = (DWORD)s.wispSequences[i].scanCode;
			RegSetValueExW(hKey, keyName.c_str(), 0, REG_DWORD, (BYTE*)&seqScan, sizeof(DWORD));
			keyName = std::wstring(baseName) + L"_Sequence";
			RegSetValueExW(hKey, keyName.c_str(), 0, REG_SZ, (BYTE*)s.wispSequences[i].sequence.c_str(), (DWORD)((s.wispSequences[i].sequence.length() + 1) * sizeof(wchar_t)));
		}
		RegSetValueExW(hKey, L"RollM1Key", 0, REG_SZ, (BYTE*)s.rollM1Key.c_str(), (DWORD)((s.rollM1Key.length() + 1) * sizeof(wchar_t)));
		DWORD rollM1Sc = (DWORD)s.rollM1TriggerScanCode;
		RegSetValueExW(hKey, L"RollM1ScanCode", 0, REG_DWORD, (BYTE*)&rollM1Sc, sizeof(DWORD));
		RegSetValueExW(hKey, L"GoldenTongueKey", 0, REG_SZ, (BYTE*)s.goldenTongueKey.c_str(), (DWORD)((s.goldenTongueKey.length() + 1) * sizeof(wchar_t)));
		DWORD goldenSc = (DWORD)s.goldenTongueTriggerScanCode;
		RegSetValueExW(hKey, L"GoldenTongueScanCode", 0, REG_DWORD, (BYTE*)&goldenSc, sizeof(DWORD));
		RegSetValueExW(hKey, L"RollCritKey", 0, REG_SZ, (BYTE*)s.rollCritKey.c_str(), (DWORD)((s.rollCritKey.length() + 1) * sizeof(wchar_t)));
		DWORD rollCritSc = (DWORD)s.rollCritScanCode;
		RegSetValueExW(hKey, L"RollCritScanCode", 0, REG_DWORD, (BYTE*)&rollCritSc, sizeof(DWORD));
		RegSetValueExW(hKey, L"RollSpitTriggerKey", 0, REG_SZ, (BYTE*)s.rollSpitTriggerKey.c_str(), (DWORD)((s.rollSpitTriggerKey.length() + 1) * sizeof(wchar_t)));
		DWORD rollSpitTrigSc = (DWORD)s.rollSpitTriggerScanCode;
		RegSetValueExW(hKey, L"RollSpitTriggerScanCode", 0, REG_DWORD, (BYTE*)&rollSpitTrigSc, sizeof(DWORD));
		RegSetValueExW(hKey, L"RollSpitSpitKey", 0, REG_SZ, (BYTE*)s.rollSpitSpitKey.c_str(), (DWORD)((s.rollSpitSpitKey.length() + 1) * sizeof(wchar_t)));
		DWORD rollSpitSpitSc = (DWORD)s.rollSpitSpitScanCode;
		RegSetValueExW(hKey, L"RollSpitSpitScanCode", 0, REG_DWORD, (BYTE*)&rollSpitSpitSc, sizeof(DWORD));
		RegSetValueExW(hKey, L"RollCastKey", 0, REG_SZ, (BYTE*)s.rollCastKey.c_str(), (DWORD)((s.rollCastKey.length() + 1) * sizeof(wchar_t)));
		RegSetValueExW(hKey, L"DodgeKey", 0, REG_SZ, (BYTE*)s.dodgeKey.c_str(), (DWORD)((s.dodgeKey.length() + 1) * sizeof(wchar_t)));
		DWORD dodgeSc = (DWORD)s.dodgeScanCode;
		RegSetValueExW(hKey, L"DodgeScanCode", 0, REG_DWORD, (BYTE*)&dodgeSc, sizeof(DWORD));
		RegSetValueExW(hKey, L"ParryKey", 0, REG_SZ, (BYTE*)s.parryKey.c_str(), (DWORD)((s.parryKey.length() + 1) * sizeof(wchar_t)));
		DWORD parrySc = (DWORD)s.parryScanCode;
		RegSetValueExW(hKey, L"ParryScanCode", 0, REG_DWORD, (BYTE*)&parrySc, sizeof(DWORD));
		RegSetValueExW(hKey, L"OpenMapKey", 0, REG_SZ, (BYTE*)s.openMapKey.c_str(), (DWORD)((s.openMapKey.length() + 1) * sizeof(wchar_t)));
		DWORD openMapSc = (DWORD)s.openMapTriggerScanCode;
		RegSetValueExW(hKey, L"OpenMapScanCode", 0, REG_DWORD, (BYTE*)&openMapSc, sizeof(DWORD));
		RegSetValueExW(hKey, L"CriticalAttackKey", 0, REG_SZ, (BYTE*)s.criticalAttackKey.c_str(), (DWORD)((s.criticalAttackKey.length() + 1) * sizeof(wchar_t)));
		DWORD critSc = (DWORD)s.criticalAttackTriggerScanCode;
		RegSetValueExW(hKey, L"CriticalAttackScanCode", 0, REG_DWORD, (BYTE*)&critSc, sizeof(DWORD));
		DWORD critMouseBtn = (DWORD)s.criticalAttackMouseButtonIndex;
		RegSetValueExW(hKey, L"CriticalAttackMouseButton", 0, REG_DWORD, (BYTE*)&critMouseBtn, sizeof(DWORD));
		RegSetValueExW(hKey, L"ZoomKey", 0, REG_SZ, (BYTE*)s.zoomKey.c_str(), (DWORD)((s.zoomKey.length() + 1) * sizeof(wchar_t)));
		DWORD zoomSc = (DWORD)s.zoomTriggerScanCode;
		RegSetValueExW(hKey, L"ZoomScanCode", 0, REG_DWORD, (BYTE*)&zoomSc, sizeof(DWORD));
		RegSetValueExW(hKey, L"QuickTurnKey", 0, REG_SZ, (BYTE*)s.quickTurnKey.c_str(), (DWORD)((s.quickTurnKey.length() + 1) * sizeof(wchar_t)));
		DWORD quickTurnSc = (DWORD)s.quickTurnTriggerScanCode;
		RegSetValueExW(hKey, L"QuickTurnScanCode", 0, REG_DWORD, (BYTE*)&quickTurnSc, sizeof(DWORD));
		DWORD quickTurnMouseBtn = (DWORD)s.quickTurnMouseButtonIndex;
		RegSetValueExW(hKey, L"QuickTurnMouseButton", 0, REG_DWORD, (BYTE*)&quickTurnMouseBtn, sizeof(DWORD));
		RegSetValueExW(hKey, L"RollParryKey", 0, REG_SZ, (BYTE*)s.rollParryKey.c_str(), (DWORD)((s.rollParryKey.length() + 1) * sizeof(wchar_t)));
		DWORD rollParrySc = (DWORD)s.rollParryScanCode;
		RegSetValueExW(hKey, L"RollParryScanCode", 0, REG_DWORD, (BYTE*)&rollParrySc, sizeof(DWORD));
		RegSetValueExW(hKey, L"CameraLockKey", 0, REG_SZ, (BYTE*)s.cameraLockKey.c_str(), (DWORD)((s.cameraLockKey.length() + 1) * sizeof(wchar_t)));
		DWORD camLockSc = (DWORD)s.cameraLockTriggerScanCode;
		RegSetValueExW(hKey, L"CameraLockScanCode", 0, REG_DWORD, (BYTE*)&camLockSc, sizeof(DWORD));
		RegSetValueExW(hKey, L"CrosshairFilePath", 0, REG_SZ, (BYTE*)s.crosshairFilePath.c_str(), (DWORD)((s.crosshairFilePath.length() + 1) * sizeof(wchar_t)));
		DWORD rollCritical = s.rollCritical ? 1 : 0;
		RegSetValueExW(hKey, L"RollCritical", 0, REG_DWORD, (BYTE*)&rollCritical, sizeof(DWORD));
		DWORD rollParry = s.rollParry ? 1 : 0;
		RegSetValueExW(hKey, L"RollParry", 0, REG_DWORD, (BYTE*)&rollParry, sizeof(DWORD));
		DWORD betterParryAntiShakyBlock = s.betterParryAntiShakyBlock ? 1 : 0;
		RegSetValueExW(hKey, L"BetterParryAntiShakyBlock", 0, REG_DWORD, (BYTE*)&betterParryAntiShakyBlock, sizeof(DWORD));
		DWORD betterParryLightspeedReflex = s.betterParryLightspeedReflex ? 1 : 0;
		RegSetValueExW(hKey, L"BetterParryLightspeedReflex", 0, REG_DWORD, (BYTE*)&betterParryLightspeedReflex, sizeof(DWORD));
		DWORD betterParryShouldParry = s.betterParryShouldParry ? 1 : 0;
		RegSetValueExW(hKey, L"BetterParryShouldParry", 0, REG_DWORD, (BYTE*)&betterParryShouldParry, sizeof(DWORD));
		DWORD holdM1Enabled = s.holdM1Enabled ? 1 : 0;
		RegSetValueExW(hKey, L"HoldM1Enabled", 0, REG_DWORD, (BYTE*)&holdM1Enabled, sizeof(DWORD));
		DWORD quickTurnToggleMode = s.quickTurnToggleMode ? 1 : 0;
		RegSetValueExW(hKey, L"QuickTurnToggleMode", 0, REG_DWORD, (BYTE*)&quickTurnToggleMode, sizeof(DWORD));
		DWORD quickTurnSensitivityAutoUpdate = s.quickTurnSensitivityAutoUpdate ? 1 : 0;
		RegSetValueExW(hKey, L"QuickTurnSensitivityAutoUpdate", 0, REG_DWORD, (BYTE*)&quickTurnSensitivityAutoUpdate, sizeof(DWORD));
		DWORD hudKeystrokesHasPos = s.hudKeystrokesHasPos ? 1 : 0;
		RegSetValueExW(hKey, L"HudKeystrokesHasPos", 0, REG_DWORD, (BYTE*)&hudKeystrokesHasPos, sizeof(DWORD));
		DWORD hudKeystrokesX = (DWORD)s.hudKeystrokesX;
		DWORD hudKeystrokesY = (DWORD)s.hudKeystrokesY;
		RegSetValueExW(hKey, L"HudKeystrokesX", 0, REG_DWORD, (BYTE*)&hudKeystrokesX, sizeof(DWORD));
		RegSetValueExW(hKey, L"HudKeystrokesY", 0, REG_DWORD, (BYTE*)&hudKeystrokesY, sizeof(DWORD));
		RegSetValueExW(hKey, L"HudKeystrokesScale", 0, REG_BINARY, (BYTE*)&s.hudKeystrokesScale, sizeof(float));
		DWORD hudCPSHasPos = s.hudCPSHasPos ? 1 : 0;
		RegSetValueExW(hKey, L"HudCPSHasPos", 0, REG_DWORD, (BYTE*)&hudCPSHasPos, sizeof(DWORD));
		DWORD hudCPSX = (DWORD)s.hudCPSX;
		DWORD hudCPSY = (DWORD)s.hudCPSY;
		RegSetValueExW(hKey, L"HudCPSX", 0, REG_DWORD, (BYTE*)&hudCPSX, sizeof(DWORD));
		RegSetValueExW(hKey, L"HudCPSY", 0, REG_DWORD, (BYTE*)&hudCPSY, sizeof(DWORD));
		RegSetValueExW(hKey, L"HudCPSScale", 0, REG_BINARY, (BYTE*)&s.hudCPSScale, sizeof(float));
		DWORD hudParryBarHasPos = s.hudParryBarHasPos ? 1 : 0;
		RegSetValueExW(hKey, L"HudParryBarHasPos", 0, REG_DWORD, (BYTE*)&hudParryBarHasPos, sizeof(DWORD));
		DWORD hudParryBarX = (DWORD)s.hudParryBarX;
		DWORD hudParryBarY = (DWORD)s.hudParryBarY;
		RegSetValueExW(hKey, L"HudParryBarX", 0, REG_DWORD, (BYTE*)&hudParryBarX, sizeof(DWORD));
		RegSetValueExW(hKey, L"HudParryBarY", 0, REG_DWORD, (BYTE*)&hudParryBarY, sizeof(DWORD));
		RegSetValueExW(hKey, L"HudParryBarScale", 0, REG_BINARY, (BYTE*)&s.hudParryBarScale, sizeof(float));
		RegCloseKey(hKey);
	}
	
	SettingsSnapshot ReadSettingsSnapshotFromRegistry(const SettingsSnapshot& defaults) {
		SettingsSnapshot s = defaults;
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_MENU", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
			return s;
		}
		wchar_t buffer[1024];
		DWORD bufferSize;
		DWORD type;
		DWORD dwordValue;
		for (size_t i = 0; i < s.modCardEnabled.size(); i++) {
			wchar_t valueName[64];
			swprintf_s(valueName, L"ModCard_%zu_Enabled", i);
			bufferSize = sizeof(dwordValue);
			if (RegQueryValueExW(hKey, valueName, NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) {
				s.modCardEnabled[i] = (dwordValue != 0);
			}
		}
		bufferSize = sizeof(float);
		if (RegQueryValueExW(hKey, L"BlurSlider", NULL, &type, (LPBYTE)&s.blurSlider, &bufferSize) == ERROR_SUCCESS && type == REG_BINARY) {}
		if (RegQueryValueExW(hKey, L"MotionBlurSlider", NULL, &type, (LPBYTE)&s.motionBlurSlider, &bufferSize) == ERROR_SUCCESS && type == REG_BINARY) {}
		if (RegQueryValueExW(hKey, L"QuickTurnSensitivitySlider", NULL, &type, (LPBYTE)&s.quickTurnSensitivitySlider, &bufferSize) == ERROR_SUCCESS && type == REG_BINARY) {}
		if (RegQueryValueExW(hKey, L"GammaSlider", NULL, &type, (LPBYTE)&s.gammaSlider, &bufferSize) == ERROR_SUCCESS && type == REG_BINARY) {}
		if (RegQueryValueExW(hKey, L"QuickTurnDegreesSlider", NULL, &type, (LPBYTE)&s.quickTurnDegreesSlider, &bufferSize) == ERROR_SUCCESS && type == REG_BINARY) {}
		if (RegQueryValueExW(hKey, L"CrosshairScale", NULL, &type, (LPBYTE)&s.crosshairScale, &bufferSize) == ERROR_SUCCESS && type == REG_BINARY) {}
		for (int i = 0; i < 10; i++) {
			wchar_t keyName[64];
			swprintf_s(keyName, L"HotbarSlot_%d_Key", i);
			bufferSize = sizeof(buffer);
			if (RegQueryValueExW(hKey, keyName, NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) {
				s.hotbarSlotKeys[i] = buffer;
			}
			swprintf_s(keyName, L"HotbarSlot_%d_Enabled", i);
			bufferSize = sizeof(dwordValue);
			if (RegQueryValueExW(hKey, keyName, NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) {
				s.hotbarSlotEnabled[i] = (dwordValue != 0);
			}
			swprintf_s(keyName, L"HotbarSlot_%d_MapCastEnabled", i);
			bufferSize = sizeof(dwordValue);
			if (RegQueryValueExW(hKey, keyName, NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) {
				s.hotbarSlotMapCastEnabled[i] = (dwordValue != 0);
			}
		}
		bufferSize = sizeof(dwordValue);
		DWORD wispSequenceCount = 1;
		if (RegQueryValueExW(hKey, L"WispSequenceCount", NULL, &type, (LPBYTE)&wispSequenceCount, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) {
			if (wispSequenceCount > 0 && wispSequenceCount <= 100) {
				s.wispSequences.clear();
				for (DWORD i = 0; i < wispSequenceCount; i++) {
					SavedWispSequence seq;
					wchar_t baseName[64];
					swprintf_s(baseName, L"WispSequence_%lu", i);
					std::wstring keyName = std::wstring(baseName) + L"_Key";
					bufferSize = sizeof(buffer);
					if (RegQueryValueExW(hKey, keyName.c_str(), NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) {
						seq.key = buffer;
					}
					keyName = std::wstring(baseName) + L"_ScanCode";
					bufferSize = sizeof(DWORD);
					DWORD scanCodeDword = 0;
					if (RegQueryValueExW(hKey, keyName.c_str(), NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) {
						seq.scanCode = (USHORT)scanCodeDword;
					}
					keyName = std::wstring(baseName) + L"_Sequence";
					bufferSize = sizeof(buffer);
					if (RegQueryValueExW(hKey, keyName.c_str(), NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) {
						seq.sequence = buffer;
					}
					s.wispSequences.push_back(std::move(seq));
				}
			}
		}
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"RollM1Key", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.rollM1Key = buffer;
		bufferSize = sizeof(DWORD);
		DWORD scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"RollM1ScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.rollM1TriggerScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"GoldenTongueKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.goldenTongueKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"GoldenTongueScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.goldenTongueTriggerScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"RollCritKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.rollCritKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"RollCritScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.rollCritScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"RollSpitTriggerKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.rollSpitTriggerKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"RollSpitTriggerScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.rollSpitTriggerScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"RollSpitSpitKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.rollSpitSpitKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"RollSpitSpitScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.rollSpitSpitScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"RollCastKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.rollCastKey = buffer;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"DodgeKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.dodgeKey = buffer;
		bufferSize = sizeof(DWORD);
		DWORD dodgeScDword = 0;
		if (RegQueryValueExW(hKey, L"DodgeScanCode", NULL, &type, (LPBYTE)&dodgeScDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.dodgeScanCode = (USHORT)dodgeScDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"ParryKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.parryKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"ParryScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.parryScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"OpenMapKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.openMapKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"OpenMapScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.openMapTriggerScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"CriticalAttackKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.criticalAttackKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"CriticalAttackScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.criticalAttackTriggerScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(DWORD);
		DWORD mouseButtonDword = 0;
		if (RegQueryValueExW(hKey, L"CriticalAttackMouseButton", NULL, &type, (LPBYTE)&mouseButtonDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.criticalAttackMouseButtonIndex = (int)mouseButtonDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"ZoomKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.zoomKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"ZoomScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.zoomTriggerScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"QuickTurnKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.quickTurnKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"QuickTurnScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.quickTurnTriggerScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(DWORD);
		mouseButtonDword = 0;
		if (RegQueryValueExW(hKey, L"QuickTurnMouseButton", NULL, &type, (LPBYTE)&mouseButtonDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.quickTurnMouseButtonIndex = (int)mouseButtonDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"RollParryKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.rollParryKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"RollParryScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.rollParryScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"CameraLockKey", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.cameraLockKey = buffer;
		bufferSize = sizeof(DWORD);
		scanCodeDword = 0;
		if (RegQueryValueExW(hKey, L"CameraLockScanCode", NULL, &type, (LPBYTE)&scanCodeDword, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.cameraLockTriggerScanCode = (USHORT)scanCodeDword;
		bufferSize = sizeof(buffer);
		if (RegQueryValueExW(hKey, L"CrosshairFilePath", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) s.crosshairFilePath = buffer;
		bufferSize = sizeof(dwordValue);
		if (RegQueryValueExW(hKey, L"RollCritical", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.rollCritical = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"RollParry", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.rollParry = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"BetterParryAntiShakyBlock", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.betterParryAntiShakyBlock = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"BetterParryLightspeedReflex", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.betterParryLightspeedReflex = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"BetterParryShouldParry", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.betterParryShouldParry = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"HoldM1Enabled", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.holdM1Enabled = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"QuickTurnToggleMode", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.quickTurnToggleMode = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"QuickTurnSensitivityAutoUpdate", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.quickTurnSensitivityAutoUpdate = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"HudKeystrokesHasPos", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.hudKeystrokesHasPos = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"HudKeystrokesX", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.hudKeystrokesX = (int)dwordValue;
		if (RegQueryValueExW(hKey, L"HudKeystrokesY", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.hudKeystrokesY = (int)dwordValue;
		bufferSize = sizeof(float);
		if (RegQueryValueExW(hKey, L"HudKeystrokesScale", NULL, &type, (LPBYTE)&s.hudKeystrokesScale, &bufferSize) == ERROR_SUCCESS && type == REG_BINARY) {}
		bufferSize = sizeof(dwordValue);
		if (RegQueryValueExW(hKey, L"HudCPSHasPos", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.hudCPSHasPos = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"HudCPSX", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.hudCPSX = (int)dwordValue;
		if (RegQueryValueExW(hKey, L"HudCPSY", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.hudCPSY = (int)dwordValue;
		bufferSize = sizeof(float);
		if (RegQueryValueExW(hKey, L"HudCPSScale", NULL, &type, (LPBYTE)&s.hudCPSScale, &bufferSize) == ERROR_SUCCESS && type == REG_BINARY) {}
		bufferSize = sizeof(dwordValue);
		if (RegQueryValueExW(hKey, L"HudParryBarHasPos", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.hudParryBarHasPos = (dwordValue != 0);
		if (RegQueryValueExW(hKey, L"HudParryBarX", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.hudParryBarX = (int)dwordValue;
		if (RegQueryValueExW(hKey, L"HudParryBarY", NULL, &type, (LPBYTE)&dwordValue, &bufferSize) == ERROR_SUCCESS && type == REG_DWORD) s.hudParryBarY = (int)dwordValue;
		bufferSize = sizeof(float);
		if (RegQueryValueExW(hKey, L"HudParryBarScale", NULL, &type, (LPBYTE)&s.hudParryBarScale, &bufferSize) == ERROR_SUCCESS && type == REG_BINARY) {}
		RegCloseKey(hKey);
		return s;
	}
	
	void QueueSettingsSave() {
		// Just signal the worker thread - it will capture the snapshot itself
		// This avoids expensive snapshot capture on the UI thread
		std::lock_guard<std::mutex> lk(m_registryMutex);
		m_savePending = true;
		m_registryCv.notify_one();
	}
	
	void StartRegistryWorker() {
		if (m_registryWorker.joinable()) return;
		m_registryStop = false;
		SettingsSnapshot defaults = CaptureSettingsSnapshot();
		m_registryWorker = std::thread([this, defaults]() {
			SettingsSnapshot loaded = ReadSettingsSnapshotFromRegistry(defaults);
			{
				std::lock_guard<std::mutex> lk(m_registryMutex);
				m_loadedSnapshot = std::move(loaded);
				m_loadedPending = true;
			}
			if (m_hwnd) PostMessageW(m_hwnd, kSettingsLoadedMessage, 0, 0);
			for (;;) {
				bool shouldSave = false;
				{
					std::unique_lock<std::mutex> lk(m_registryMutex);
					m_registryCv.wait(lk, [this]() { return m_registryStop || m_savePending; });
					if (m_savePending) {
						m_savePending = false;
						shouldSave = true;
					} else if (m_registryStop) {
						break;
					}
					if (m_registryStop && !shouldSave && !m_savePending) break;
				}
				if (shouldSave) {
					// Capture snapshot on worker thread to avoid blocking UI
					SettingsSnapshot toSave = CaptureSettingsSnapshot();
					WriteSettingsSnapshotToRegistry(toSave);
				}
				if (m_registryStop) {
					std::lock_guard<std::mutex> lk(m_registryMutex);
					if (!m_savePending) break;
				}
			}
		});
	}
	
	void StopRegistryWorker(bool flush) {
		if (flush) {
			QueueSettingsSave();
		}
		{
			std::lock_guard<std::mutex> lk(m_registryMutex);
			m_registryStop = true;
		}
		m_registryCv.notify_one();
		if (m_registryWorker.joinable()) {
			m_registryWorker.join();
		}
	}
	
	void HandleSettingsLoadedMessage() {
		SettingsSnapshot snap;
		bool have = false;
		{
			std::lock_guard<std::mutex> lk(m_registryMutex);
			if (m_loadedPending) {
				snap = std::move(m_loadedSnapshot);
				m_loadedPending = false;
				have = true;
			}
		}
		if (!have) return;
		ApplySettingsSnapshot(snap);
		if (!m_initialSettingsApplied) {
			ApplyLoadedSettingsToRuntime();
			StartModsFromSavedState();
			m_initialSettingsApplied = true;
		} else {
			ApplyLoadedSettingsToRuntime();
		}
		if (m_hwnd) InvalidateRect(m_hwnd, nullptr, FALSE);
	}

	void ApplyLoadedSettingsToRuntime() {
		// Apply slider-driven runtime values immediately
		SetBlurIntensity(m_blurSlider);
		MotionBlur::SetMotionBlurIntensity(powf(m_motionBlurSlider, 0.35f) * 1.5f);

		// Gamma target (actual apply handled by gamma monitor when enabled)
		float gammaValue = 0.1f + m_gammaSlider * 3.9f;
		g_gammaTarget.store(gammaValue);

		// Crosshair
		Crosshair::UpdateCrosshairScale(m_crosshairScale);
		if (!m_crosshairFilePath.empty() && _wcsicmp(m_crosshairFilePath.c_str(), L"No file selected") != 0) {
			Crosshair::UpdateCrosshairImage(m_crosshairFilePath);
		}

		// Hotkey forwarding / other modules that rely on cached scan codes
		Keystrokes::SetParryScanCode(m_parryScanCode);
		Keystrokes::SetParryKeyName(m_parryKey);
		Keystrokes::SetDodgeKeyName(m_dodgeKey);
		Keystrokes::SetDodgeScanCode(m_dodgeScanCode);
		RollM1::SetRollKey(m_dodgeKey);
		RollM1::SetRollKeyScanCode(m_dodgeScanCode);
		RollSpit::SetRollKeyScanCode(m_dodgeScanCode);
		RollCast::SetDodgeKeyScanCode(m_dodgeScanCode);
		RollCritical::SetDodgeKeyScanCode(m_dodgeScanCode);
		RollParry::SetDodgeKeyScanCode(m_dodgeScanCode);
		RollParry::SetParryKeyScanCode(m_parryScanCode);
		MapCast::SetOpenMapKeyScanCode(m_openMapTriggerScanCode);
		RollSpit::SetSpitKeyScanCode(m_rollSpitSpitScanCode);
		RollCritical::SetCriticalKeyScanCode(m_criticalAttackTriggerScanCode);
		RollCritical::SetCriticalKeyMouseButton(m_criticalAttackMouseButtonIndex);
		Zoom::SetTriggerScanCode(m_zoomTriggerScanCode);
		HoldM1::SetIntelligentHoldM1Enabled(m_holdM1Enabled);
		if (m_criticalAttackTriggerScanCode != 0) {
			UINT vkCode = MapVirtualKeyW(m_criticalAttackTriggerScanCode, MAPVK_VSC_TO_VK);
			if (vkCode != 0) {
				HoldM1::SetCriticalAttackVKCode(vkCode);
			}
		}
		if (m_hudKeystrokesScale > 0.0f) Keystrokes::SetUserScale(m_hudKeystrokesScale);
		if (m_hudKeystrokesHasPos) Keystrokes::SetKeystrokesPosition(m_hudKeystrokesX, m_hudKeystrokesY);
		if (m_hudCPSScale > 0.0f) CPS::SetUserScale(m_hudCPSScale);
		if (m_hudCPSHasPos) CPS::SetCPSPosition(m_hudCPSX, m_hudCPSY);
		if (m_hudParryBarScale > 0.0f) ParryBar::SetUserScale(m_hudParryBarScale);
		if (m_hudParryBarHasPos) ParryBar::SetParryBarPosition(m_hudParryBarX, m_hudParryBarY);
	}

	void ProcessPendingStarts() {
		// Mirrors the "start pending mods" logic previously triggered only after hide animation completes.
		if (g_rollM1PendingStart && !g_rollM1Running) {
			std::thread([]() {
				HWND robloxWindow = WinRTCapture::FindRobloxWindow();
				if (!robloxWindow) return;
				for (;;) {
					if (!g_rollM1PendingStart || g_rollM1Running) return;
					HWND fg = GetForegroundWindow();
					if (fg == robloxWindow || GetParent(fg) == robloxWindow) break;
					Sleep(50);
				}
				if (!g_rollM1PendingStart || g_rollM1Running) return;
				RollM1::StartRollM1();
				RollM1::SetEnabled(true);
				g_rollM1Running = true;
				g_rollM1PendingStart = false;
			}).detach();
		}

		if (g_rollCriticalPendingStart && !g_rollCriticalRunning) {
			std::thread([]() {
				HWND robloxWindow = WinRTCapture::FindRobloxWindow();
				if (!robloxWindow) return;
				for (;;) {
					if (!g_rollCriticalPendingStart || g_rollCriticalRunning) return;
					HWND fg = GetForegroundWindow();
					if (fg == robloxWindow || GetParent(fg) == robloxWindow) break;
					Sleep(50);
				}
				if (!g_rollCriticalPendingStart || g_rollCriticalRunning) return;
				RollCritical::StartRollCritical();
				RollCritical::SetEnabled(true);
				g_rollCriticalRunning = true;
				g_rollCriticalPendingStart = false;
			}).detach();
		}

		if (g_rollParryPendingStart && !g_rollParryRunning) {
			std::thread([]() {
				HWND robloxWindow = WinRTCapture::FindRobloxWindow();
				if (!robloxWindow) return;
				for (;;) {
					if (!g_rollParryPendingStart || g_rollParryRunning) return;
					HWND fg = GetForegroundWindow();
					if (fg == robloxWindow || GetParent(fg) == robloxWindow) break;
					Sleep(50);
				}
				if (!g_rollParryPendingStart || g_rollParryRunning) return;
				RollParry::StartRollParry();
				RollParry::SetEnabled(true);
				g_rollParryRunning = true;
				g_rollParryPendingStart = false;
			}).detach();
		}

		if (g_rollSpitPendingStart && !g_rollSpitRunning) {
			std::thread([]() {
				HWND robloxWindow = WinRTCapture::FindRobloxWindow();
				if (!robloxWindow) return;
				for (;;) {
					if (!g_rollSpitPendingStart || g_rollSpitRunning) return;
					HWND fg = GetForegroundWindow();
					if (fg == robloxWindow || GetParent(fg) == robloxWindow) break;
					Sleep(50);
				}
				if (!g_rollSpitPendingStart || g_rollSpitRunning) return;
				RollSpit::StartRollSpit();
				RollSpit::SetEnabled(true);
				g_rollSpitRunning = true;
				g_rollSpitPendingStart = false;
			}).detach();
		}

		if (g_parryBarPendingStart && !g_parryBarRunning) {
			std::thread([]() {
				HWND robloxWindow = WinRTCapture::FindRobloxWindow();
				if (!robloxWindow) return;
				for (;;) {
					if (!g_parryBarPendingStart || g_parryBarRunning) return;
					HWND fg = GetForegroundWindow();
					if (fg == robloxWindow || GetParent(fg) == robloxWindow) break;
					Sleep(50);
				}
				if (!g_parryBarPendingStart || g_parryBarRunning) return;
				ParryBar::InitializePixelReader();
				bool parryBarModEnabled = (s_instance->m_modCards.size() > 1 && s_instance->m_modCards[1].enabled);
				ParryBar::StartPixelReading(parryBarModEnabled);
				g_parryBarRunning = true;
				g_parryBarPendingStart = false;
			}).detach();
		}

		if (g_keystrokesPendingStart && !g_keystrokesRunning) {
			Keystrokes::StartKeystrokes();
			g_keystrokesRunning = true;
			g_keystrokesPendingStart = false;
		}

		if (g_cpsPendingStart && !g_cpsRunning) {
			CPS::StartCPS();
			g_cpsRunning = true;
			g_cpsPendingStart = false;
		}

		if (g_rollCastPendingStart && !g_rollCastRunning) {
			RollCast::StartRollCast();
			RollCast::SetEnabled(true);
			g_rollCastRunning = true;
			g_rollCastPendingStart = false;
		}

		if (g_holdM1PendingStart && !g_holdM1Running) {
			HoldM1::StartPixelScanning();
			HoldM1::SetHoldM1Enabled(true);
			HoldM1::SetIntelligentHoldM1Enabled(s_instance->m_holdM1Enabled);
			g_holdM1Running = true;
			g_holdM1PendingStart = false;
		}

		if (g_mapCastPendingStart && !g_mapCastRunning) {
			MapCast::StartMapCast();
			MapCast::SetEnabled(true);
			MapCast::SetOpenMapKeyScanCode(s_instance->m_openMapTriggerScanCode);
			g_mapCastRunning = true;
			g_mapCastPendingStart = false;
		}

		if (g_crosshairPendingStart && !g_crosshairRunning) {
			Crosshair::StartCrosshair();
			Crosshair::SetEnabled(true);
			g_crosshairRunning = true;
			g_crosshairPendingStart = false;
		}

		if (g_motionBlurPendingStart && !g_motionBlurRunning) {
			std::thread([]() { MotionBlur::StartMotionBlur(); }).detach();
			g_motionBlurRunning = true;
			g_motionBlurPendingStart = false;
		}

		if (g_zoomPendingStart && !g_zoomRunning) {
			Zoom::Initialize();
			Zoom::SetTriggerScanCode(s_instance->m_zoomTriggerScanCode);
			g_zoomRunning = true;
			g_zoomPendingStart = false;
		}

		if (g_goldenTonguePendingStart && !g_goldenTongueRunning) {
			std::thread([]() {
				HWND robloxWindow = WinRTCapture::FindRobloxWindow();
				if (!robloxWindow) return;
				for (;;) {
					if (!g_goldenTonguePendingStart || g_goldenTongueRunning) return;
					HWND fg = GetForegroundWindow();
					if (fg == robloxWindow || GetParent(fg) == robloxWindow) break;
					Sleep(50);
				}
				if (!g_goldenTonguePendingStart || g_goldenTongueRunning) return;
				GoldenTongue::StartGoldenTongue();
				GoldenTongue::SetEnabled(true);
				g_goldenTongueRunning = true;
				g_goldenTonguePendingStart = false;
			}).detach();
		}

		if (g_quickTurnPendingStart && !g_quickTurnRunning) {
			std::thread([]() {
				HWND robloxWindow = WinRTCapture::FindRobloxWindow();
				if (!robloxWindow) return;
				for (;;) {
					if (!g_quickTurnPendingStart || g_quickTurnRunning) return;
					HWND fg = GetForegroundWindow();
					if (fg == robloxWindow || GetParent(fg) == robloxWindow) break;
					Sleep(50);
				}
				if (!g_quickTurnPendingStart || g_quickTurnRunning) return;
				QuickTurn::StartQuickTurn();
				QuickTurn::SetEnabled(true);
				QuickTurn::SetSensitivity(s_instance->m_quickTurnSensitivitySlider);
				float degreesValue = s_instance->m_quickTurnDegreesSlider * 360.0f;
				QuickTurn::SetTargetDegrees(degreesValue);
				g_quickTurnRunning = true;
				g_quickTurnPendingStart = false;
			}).detach();
		}
	}

	void StartModsFromSavedState() {
		// Convert saved "enabled" state into pending-start flags (so mods work without requiring a hide animation).
		if (m_modCards.size() > 0 && m_modCards[0].enabled) g_rollM1PendingStart = true;
		if (m_modCards.size() > 2 && m_modCards[2].enabled) g_mapCastPendingStart = true;
		if (m_modCards.size() > 3 && m_modCards[3].enabled) g_keystrokesPendingStart = true;
		if (m_modCards.size() > 5 && m_modCards[5].enabled) g_cpsPendingStart = true;
		if (m_modCards.size() > 6 && m_modCards[6].enabled) g_rollCastPendingStart = true;
		if (m_modCards.size() > 7 && m_modCards[7].enabled) g_holdM1PendingStart = true;
		if (m_modCards.size() > 8 && m_modCards[8].enabled) g_crosshairPendingStart = true;
		if (m_modCards.size() > 9 && m_modCards[9].enabled) g_rollSpitPendingStart = true;
		if (m_modCards.size() > 10 && m_modCards[10].enabled) g_motionBlurPendingStart = true;
		if (m_modCards.size() > 11 && m_modCards[11].enabled) g_zoomPendingStart = true;
		if (m_modCards.size() > 12 && m_modCards[12].enabled) g_quickTurnPendingStart = true;
		if (m_modCards.size() > 14 && m_modCards[14].enabled) g_goldenTonguePendingStart = true;

		// RollCritical and RollParry are settings toggles (not modcards)
		if (m_rollCritical) g_rollCriticalPendingStart = true;
		if (m_rollParry) g_rollParryPendingStart = true;

		// ParryBar pixel reader is needed if Parry Bar card is enabled OR BetterParry needs it (anti-shaky)
		bool parryBarCardEnabled = (m_modCards.size() > 1 && m_modCards[1].enabled);
		bool betterParryEnabled = (m_modCards.size() > 13 && m_modCards[13].enabled);
		if (parryBarCardEnabled || (betterParryEnabled && m_betterParryAntiShakyBlock)) {
			g_parryBarPendingStart = true;
		}

		// Gamma is immediate (not pending-start based)
		if (m_modCards.size() > 15 && m_modCards[15].enabled) {
			g_gammaRunning = true;
			g_gammaEnabled.store(true);
			float gammaValue = 0.1f + m_gammaSlider * 3.9f;
			g_gammaTarget.store(gammaValue);
			StartGammaMonitor();
		}

		ProcessPendingStarts();
	}

public:
    PistachioCreamMacro() {
        s_instance = this;
        m_quickTurnSensitivitySlider = ReadRobloxMouseSensitivity();
        InitializeDirect2D();
        CreateMainWindow();
        InitializeModCards();
		StartRegistryWorker();
        SetupKeyboardHook();
        InitializeBlurLibrary(m_hwnd);
        StartBlurRendering();
        ParryBar::StartGateCheck();
        Crosshair::StartShiftLockCheck();
		HoldM1::StartBlockIndicatorCheck();
		RegisterRawInput();
		SetTimer(m_hwnd, 5, 5000, nullptr);
		Traytip::StartTraytip();
    }
    
    ~PistachioCreamMacro() {
        m_running = false;
		StopRegistryWorker(true);
        ParryBar::StopGateCheck();
        Crosshair::StopShiftLockCheck();
        HoldM1::StopBlockIndicatorCheck();
        Traytip::StopTraytip();
        if (m_hHook) {
            UnhookWindowsHookEx(m_hHook);
        }
        CleanupDirect2D();
    }
	
	static PistachioCreamMacro* Instance() { return s_instance; }

	void UpdateHudKeystrokesLayout(int x, int y, float scale, bool hasPos) {
		if (hasPos) {
			m_hudKeystrokesHasPos = true;
			m_hudKeystrokesX = x;
			m_hudKeystrokesY = y;
		}
		if (scale > 0.0f) m_hudKeystrokesScale = scale;
		MarkSettingsDirty();
	}

	void UpdateHudCPSLayout(int x, int y, float scale, bool hasPos) {
		if (hasPos) {
			m_hudCPSHasPos = true;
			m_hudCPSX = x;
			m_hudCPSY = y;
		}
		if (scale > 0.0f) m_hudCPSScale = scale;
		MarkSettingsDirty();
	}

	void UpdateHudParryBarLayout(int x, int y, float scale, bool hasPos) {
		if (hasPos) {
			m_hudParryBarHasPos = true;
			m_hudParryBarX = x;
			m_hudParryBarY = y;
		}
		if (scale > 0.0f) m_hudParryBarScale = scale;
		MarkSettingsDirty();
	}

private:
    float ReadRobloxMouseSensitivity() {
        char localAppData[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData) != S_OK) {
            return 0.50f;
        }
        
        std::string filePath = std::string(localAppData) + "\\Roblox\\GlobalBasicSettings_13.xml";
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return 0.50f;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find("<float name=\"MouseSensitivity\">");
            if (pos != std::string::npos) {
                size_t start = pos + 31;
                size_t end = line.find("</float>", start);
                if (end != std::string::npos) {
                    std::string valueStr = line.substr(start, end - start);
                    try {
                        return std::stof(valueStr);
                    } catch (...) {
                        return 0.50f;
                    }
                }
            }
        }
        return 0.50f;
    }
    
    void InitializeDirect2D() {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2dFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_writeFactory), 
                           reinterpret_cast<IUnknown**>(&m_writeFactory));
        
        m_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       14.0f, L"en-us", &m_titleFormat);
        
        m_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       11.0f, L"en-us", &m_descFormat);

        m_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       9.0f, L"en-us", &m_tagFormat);
        
        m_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       14.0f, L"en-us", &m_searchFormat);
        
        m_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                       DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                       18.0f, L"en-us", &m_closeFormat);
        
        m_titleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_titleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_descFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_descFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_tagFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_tagFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_tagFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        m_searchFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_searchFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        m_closeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_closeFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

	void RegisterRawInput() {
		RAWINPUTDEVICE rid[2] = {};
		rid[0].usUsagePage = 0x01;
		rid[0].usUsage = 0x06;
		rid[0].dwFlags = RIDEV_INPUTSINK;
		rid[0].hwndTarget = m_hwnd;
		rid[1].usUsagePage = 0x01;
		rid[1].usUsage = 0x02;
		rid[1].dwFlags = RIDEV_INPUTSINK;
		rid[1].hwndTarget = m_hwnd;
		RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE));
	}

	std::wstring GetKeyNameFromVkSc(USHORT vk, USHORT sc) {
		UINT scan = sc;
		if (scan == 0) scan = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
		WCHAR name[64] = {};
		LONG lparam = (scan << 16);
		if (vk == VK_LEFT || vk == VK_UP || vk == VK_RIGHT || vk == VK_DOWN ||
			vk == VK_PRIOR || vk == VK_NEXT || vk == VK_END || vk == VK_HOME ||
			vk == VK_INSERT || vk == VK_DELETE) {
			lparam |= 1 << 24;
		}
		if (GetKeyNameTextW(lparam, name, 64) > 0) {
			return std::wstring(name);
		}
		wchar_t buf[32];
		swprintf(buf, 32, L"VK%u", (unsigned)vk);
		return buf;
	}

	// Helper to check if raw input key matches a key name
	bool IsKeyMatch(USHORT vk, USHORT sc, const std::wstring& keyName) {
		if (keyName.empty()) return false;
		
		// Single character keys (A-Z, 0-9)
		if (keyName.length() == 1) {
			wchar_t keyChar = std::toupper(keyName[0]);
			if (keyChar >= 'A' && keyChar <= 'Z') {
				return vk == (USHORT)keyChar;
			}
			if (keyChar >= '0' && keyChar <= '9') {
				return vk == (USHORT)keyChar;
			}
		}
		
		// Get the name of the pressed key and compare
		std::wstring pressedKeyName = GetKeyNameFromVkSc(vk, sc);
		return _wcsicmp(pressedKeyName.c_str(), keyName.c_str()) == 0;
	}

	std::wstring RawInputToString(RAWINPUT* ri) {
		if (ri->header.dwType == RIM_TYPEKEYBOARD) {
			const RAWKEYBOARD& kb = ri->data.keyboard;
			return GetKeyNameFromVkSc(kb.VKey, kb.MakeCode);
		} else if (ri->header.dwType == RIM_TYPEMOUSE) {
			USHORT buttonFlags = ri->data.mouse.usButtonFlags;
			for (int bit = 0; bit < 16; ++bit) {
				USHORT flag = (USHORT)(1 << bit);
				if (flag == 0x0400 || flag == 0x0800) continue;
				if ((buttonFlags & flag) && !(bit & 1)) {
					int buttonIndex = (bit >> 1) + 1;
					return L"M" + std::to_wstring(buttonIndex);
				}
			}
		}
		return L"";
	}
	
	bool MouseButtonUpMatchesKey(USHORT buttonFlags, const std::wstring& key) {
		for (int bit = 0; bit < 16; ++bit) {
			USHORT flag = (USHORT)(1 << bit);
			if (flag == 0x0400 || flag == 0x0800) continue;
			if ((buttonFlags & flag) && (bit & 1)) {
				int buttonIndex = (bit >> 1) + 1;
				std::wstring s = L"M" + std::to_wstring(buttonIndex);
				return _wcsicmp(s.c_str(), key.c_str()) == 0;
			}
		}
		return false;
	}

	void HandleRawInput(HRAWINPUT hraw) {
		UINT size = 0;
		GetRawInputData(hraw, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
		if (size == 0) return;
		std::vector<BYTE> buffer(size);
		if (GetRawInputData(hraw, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size) return;
		RAWINPUT* ri = reinterpret_cast<RAWINPUT*>(buffer.data());
		std::wstring eventString = RawInputToString(ri);
		
		if (ri->header.dwType == RIM_TYPEKEYBOARD) {
			const RAWKEYBOARD& kb = ri->data.keyboard;
			bool keyDown = (kb.Flags & RI_KEY_BREAK) == 0;
			USHORT v = kb.VKey;
			bool isRightShift = (v == VK_RSHIFT) || (v == VK_SHIFT && kb.MakeCode == 0x36);
			
			if (g_keystrokesRunning) {
				Keystrokes::SetKeyState(kb.MakeCode, keyDown);
			}
			
			if (v == 'W' && (g_rollM1Running || g_rollSpitRunning || g_rollCriticalRunning || g_rollParryRunning)) {
				if (keyDown) {
					m_wKeyHeld = true;
					if (g_rollM1Running) RollM1::SetWKeyHeld(true);
					if (g_rollSpitRunning) RollSpit::SetWKeyHeld(true);
					if (g_rollCriticalRunning) RollCritical::SetWKeyHeld(true);
					if (g_rollParryRunning) RollParry::SetWKeyHeld(true);
				} else {
					m_wKeyHeld = false;
					if (g_rollM1Running) RollM1::SetWKeyHeld(false);
					if (g_rollSpitRunning) RollSpit::SetWKeyHeld(false);
					if (g_rollCriticalRunning) RollCritical::SetWKeyHeld(false);
					if (g_rollParryRunning) RollParry::SetWKeyHeld(false);
				}
			}
			
			if (g_rollM1Running && !m_keyCaptureActive && !eventString.empty() && m_rollM1TriggerScanCode == 0) {
				if (_wcsicmp(eventString.c_str(), m_rollM1Key.c_str()) == 0) {
					if (keyDown && !m_rollM1TriggerKeyHeld) {
						m_rollM1TriggerKeyHeld = true;
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								if (robloxInFocus) {
									RollM1::TriggerRoll();
								}
							}
						}
					} else if (!keyDown) {
						m_rollM1TriggerKeyHeld = false;
					}
				}
			}
			
			if (g_goldenTongueRunning && !m_keyCaptureActive && !eventString.empty() && m_goldenTongueTriggerScanCode == 0) {
				if (_wcsicmp(eventString.c_str(), m_goldenTongueKey.c_str()) == 0) {
					if (keyDown && !m_goldenTongueTriggerKeyHeld) {
						m_goldenTongueTriggerKeyHeld = true;
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								if (robloxInFocus) {
									GoldenTongue::TriggerGoldenTongue();
								}
							}
						}
					} else if (!keyDown) {
						m_goldenTongueTriggerKeyHeld = false;
					}
				}
			}
			
			if (g_quickTurnRunning && !m_keyCaptureActive && !eventString.empty() && m_quickTurnTriggerScanCode == 0) {
				if (_wcsicmp(eventString.c_str(), m_quickTurnKey.c_str()) == 0) {
					if (keyDown && !m_quickTurnTriggerKeyHeld) {
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								bool shiftLockActive = Crosshair::IsShiftLock();
								if (robloxInFocus && shiftLockActive) {
									m_quickTurnTriggerKeyHeld = true;
									QuickTurn::TriggerTurn();
								}
							}
						}
					} else if (!keyDown) {
						if (m_quickTurnTriggerKeyHeld) {
							if (m_quickTurnToggleMode) {
								// In toggle mode: reset state so next press can turn again
								QuickTurn::ForceResetState();
								m_quickTurnTriggerKeyHeld = false;
							} else {
								// In hold mode: turn back on release
								if (IsRobloxFound()) {
									HWND robloxWindow = WinRTCapture::FindRobloxWindow();
									if (robloxWindow) {
										HWND foregroundWindow = GetForegroundWindow();
										bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
										bool shiftLockActive = Crosshair::IsShiftLock();
										if (robloxInFocus && shiftLockActive) {
											QuickTurn::ReleaseTurn();
										} else {
											QuickTurn::ForceResetState();
										}
									}
								}
								m_quickTurnTriggerKeyHeld = false;
							}
						}
					}
				}
			}
			
			if (g_rollSpitRunning && !m_keyCaptureActive && !eventString.empty() && m_rollSpitTriggerScanCode == 0) {
				if (_wcsicmp(eventString.c_str(), m_rollSpitTriggerKey.c_str()) == 0) {
					if (keyDown && !m_rollSpitTriggerKeyHeld) {
						m_rollSpitTriggerKeyHeld = true;
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								if (robloxInFocus) {
									RollSpit::TriggerRollSpit();
								}
							}
						}
					} else if (!keyDown) {
						m_rollSpitTriggerKeyHeld = false;
					}
				}
			}
			
			if (!m_keyCaptureActive && keyDown) {
				bool autoWispEnabled = (m_modCards.size() > 4 && m_modCards[4].enabled);
				if (autoWispEnabled && IsRobloxFound()) {
					HWND robloxWindow = WinRTCapture::FindRobloxWindow();
					if (robloxWindow) {
						HWND foregroundWindow = GetForegroundWindow();
						bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
						if (robloxInFocus) {
							for (auto& seq : m_wispSequences) {
								bool triggerMatch = false;
								if (seq.scanCode == 0 && !eventString.empty() && !seq.key.empty()) {
									triggerMatch = (_wcsicmp(eventString.c_str(), seq.key.c_str()) == 0);
								} else if (seq.scanCode != 0) {
									triggerMatch = (kb.MakeCode == seq.scanCode);
								}
								
								if (triggerMatch) {
									std::vector<wchar_t> sequenceChars;
									for (const auto& charBox : seq.charBoxes) {
										if (charBox.ch != 0) {
											sequenceChars.push_back(charBox.ch);
										}
									}
									if (!sequenceChars.empty()) {
										std::thread([sequenceChars]() {
											for (size_t i = 0; i < sequenceChars.size(); i++) {
												wchar_t ch = sequenceChars[i];
												WORD vk = 0;
												if (ch >= 'A' && ch <= 'Z') {
													vk = (WORD)ch;
												} else if (ch >= 'a' && ch <= 'z') {
													vk = (WORD)(ch - 'a' + 'A');
												}
												if (vk != 0) {
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
												std::this_thread::sleep_for(std::chrono::milliseconds(150));
											}
										}).detach();
									}
									break;
								}
							}
						}
					}
				}
			}
			
			if (g_rollCastRunning && !m_keyCaptureActive && !eventString.empty()) {
				// F1 is the toggle key to pause/unpause Roll Cast
				if (_wcsicmp(eventString.c_str(), m_rollCastKey.c_str()) == 0) {
					if (keyDown) {
						bool wasEnabled = RollCast::IsEnabled();
						RollCast::SetEnabled(!wasEnabled);
						bool nowEnabled = RollCast::IsEnabled();
						if (nowEnabled) {
							Traytip::traytrip("Roll Cast", "Roll Cast is on");
						} else {
							Traytip::traytrip("Roll Cast", "Roll Cast is off");
						}
					}
				}
				// Check if any hotbar slot key is pressed (only when Roll Cast is enabled/unpaused)
				else if (RollCast::IsEnabled()) {
					for (int i = 0; i < 10; i++) {
						if (_wcsicmp(eventString.c_str(), m_hotbarSlotKeys[i].c_str()) == 0 && m_hotbarSlotEnabled[i]) {
							if (keyDown && IsRobloxFound()) {
								HWND robloxWindow = WinRTCapture::FindRobloxWindow();
								if (robloxWindow) {
									HWND foregroundWindow = GetForegroundWindow();
									bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
									if (robloxInFocus) {
									// Trigger roll cast (no scan code needed, just send dodge key)
									RollCast::RollCast(0);
									break;
									}
								}
							}
						}
					}
				}
			}
			
			if (g_mapCastRunning && !m_keyCaptureActive && !eventString.empty()) {
				// Check if any hotbar slot key is pressed to trigger MapCast
				if (MapCast::IsEnabled()) {
					for (int i = 0; i < 10; i++) {
						if (_wcsicmp(eventString.c_str(), m_hotbarSlotKeys[i].c_str()) == 0 && m_hotbarSlotMapCastEnabled[i]) {
							if (keyDown && IsRobloxFound()) {
								HWND robloxWindow = WinRTCapture::FindRobloxWindow();
								if (robloxWindow) {
									HWND foregroundWindow = GetForegroundWindow();
									bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
									if (robloxInFocus) {
										MapCast::TriggerMapCast();
										break;
									}
								}
							}
						}
					}
				}
			}

			if (g_zoomRunning && !m_keyCaptureActive && m_zoomTriggerScanCode != 0) {
				if (WinRTCapture::IsCaptureInitialized()) {
					int clientWidth = WinRTCapture::GetCaptureWidth();
					int clientHeight = WinRTCapture::GetCaptureHeight();
					int screenWidth = GetSystemMetrics(SM_CXSCREEN);
					int screenHeight = GetSystemMetrics(SM_CYSCREEN);
					bool isFullscreen = (clientWidth == screenWidth && clientHeight == screenHeight);
					if (isFullscreen) {
						Zoom::HandleRawKey(kb.MakeCode, keyDown);
					}
				}
			}
			
			if (keyDown) {
				bool anyWispCapturing = false;
			for (const auto& seq : m_wispSequences) {
				if (seq.capturingKey) {
					anyWispCapturing = true;
					break;
				}
			}
			if (m_showUI && (isRightShift || v == VK_ESCAPE) && !(m_keyCaptureActive && (m_rollM1Capturing || m_rollCritCapturing || m_rollSpitTriggerCapturing || m_rollSpitSpitCapturing || m_quickTurnCapturing || m_rollParryCapturing || m_goldenTongueCapturing || anyWispCapturing))) {
					ToggleWindow();
					return;
				}
				if (m_keyCaptureActive && m_rollM1Capturing) {
					if (isRightShift) {
						return;
					}
					m_rollM1Key = eventString;
					m_rollM1TriggerScanCode = kb.MakeCode;
					m_keyCaptureActive = false;
					m_rollM1Capturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				
				// Handle Auto Wisp key capture
				for (auto& seq : m_wispSequences) {
					if (m_keyCaptureActive && seq.capturingKey) {
						if (isRightShift) {
							return;
						}
						seq.key = eventString;
						seq.scanCode = kb.MakeCode;
						m_keyCaptureActive = false;
						seq.capturingKey = false;
						MarkSettingsDirty();
						InvalidateRect(m_hwnd, nullptr, FALSE);
						return;
					}
				}
				
				if (m_keyCaptureActive && m_goldenTongueCapturing) {
					if (isRightShift) {
						return;
					}
					m_goldenTongueKey = eventString;
					m_goldenTongueTriggerScanCode = kb.MakeCode;
					m_keyCaptureActive = false;
					m_goldenTongueCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_rollCritCapturing) {
					if (isRightShift) {
						return;
					}
					m_rollCritKey = eventString;
					m_rollCritScanCode = kb.MakeCode;
					m_keyCaptureActive = false;
					m_rollCritCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_rollSpitTriggerCapturing) {
					if (isRightShift) {
						return;
					}
					m_rollSpitTriggerKey = eventString;
					m_rollSpitTriggerScanCode = kb.MakeCode;
					m_keyCaptureActive = false;
					m_rollSpitTriggerCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_rollSpitSpitCapturing) {
					if (isRightShift) {
						return;
					}
					m_rollSpitSpitKey = eventString;
					m_rollSpitSpitScanCode = kb.MakeCode;
					RollSpit::SetSpitKeyScanCode(kb.MakeCode);
					m_keyCaptureActive = false;
					m_rollSpitSpitCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_rollCastCapturing) {
					if (isRightShift) {
						return;
					}
					m_rollCastKey = eventString;
					m_keyCaptureActive = false;
					m_rollCastCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_dodgeCapturing) {
					if (isRightShift) {
						return;
					}
					m_dodgeKey = eventString;
					RollM1::SetRollKey(m_dodgeKey);
					RollM1::SetRollKeyScanCode(kb.MakeCode);
					RollSpit::SetRollKeyScanCode(kb.MakeCode);
					m_dodgeScanCode = kb.MakeCode;
					Keystrokes::SetDodgeScanCode(kb.MakeCode);
					Keystrokes::SetDodgeKeyName(m_dodgeKey);
					RollCast::SetDodgeKeyScanCode(kb.MakeCode);
					RollCritical::SetDodgeKeyScanCode(kb.MakeCode);
					RollParry::SetDodgeKeyScanCode(kb.MakeCode);
					m_keyCaptureActive = false;
					m_dodgeCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_parryCapturing) {
					if (isRightShift) {
						return;
					}
					m_parryKey = eventString;
					m_parryScanCode = kb.MakeCode;
					Keystrokes::SetParryScanCode(kb.MakeCode);
					Keystrokes::SetParryKeyName(m_parryKey);
					RollParry::SetParryKeyScanCode(kb.MakeCode);
					m_keyCaptureActive = false;
					m_parryCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_rollParryCapturing) {
					if (isRightShift) {
						return;
					}
					m_rollParryKey = eventString;
					m_rollParryScanCode = kb.MakeCode;
					m_keyCaptureActive = false;
					m_rollParryCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_openMapCapturing) {
					if (isRightShift) {
						return;
					}
					m_openMapKey = eventString;
					m_openMapTriggerScanCode = kb.MakeCode;
					MapCast::SetOpenMapKeyScanCode(kb.MakeCode);
					m_keyCaptureActive = false;
					m_openMapCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_criticalAttackCapturing) {
					if (isRightShift) {
						return;
					}
					m_criticalAttackKey = eventString;
					m_criticalAttackTriggerScanCode = kb.MakeCode;
					m_criticalAttackMouseButtonIndex = 0;
					RollCritical::SetCriticalKeyScanCode(kb.MakeCode);
					RollCritical::SetCriticalKeyMouseButton(0);
					UINT vkCode = MapVirtualKeyW(kb.MakeCode, MAPVK_VSC_TO_VK);
					if (vkCode != 0) {
						HoldM1::SetCriticalAttackVKCode(vkCode);
					}
					m_keyCaptureActive = false;
					m_criticalAttackCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				
				if (m_criticalAttackTriggerScanCode != 0 && kb.MakeCode == m_criticalAttackTriggerScanCode) {
					if (keyDown) {
						HoldM1::HandleCriticalAttackKeyDown();
					} else {
						HoldM1::HandleCriticalAttackKeyUp();
					}
				}
				
				// Check if any hotbar slot key (1-10) is pressed to trigger Mantra
				for (int i = 0; i < 10; i++) {
					if (!m_hotbarSlotKeys[i].empty() && _wcsicmp(eventString.c_str(), m_hotbarSlotKeys[i].c_str()) == 0) {
						if (keyDown) {
							HoldM1::HandleMantraKeyDown();
						}
						break;
					}
				}
				if (m_keyCaptureActive && m_zoomCapturing) {
					if (isRightShift) {
						return;
					}
					m_zoomKey = eventString;
					m_zoomTriggerScanCode = kb.MakeCode;
					Zoom::SetTriggerScanCode(m_zoomTriggerScanCode);
					m_keyCaptureActive = false;
					m_zoomCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				if (m_keyCaptureActive && m_cameraLockCapturing) {
					if (isRightShift) {
						return;
					}
					m_cameraLockKey = eventString;
					m_cameraLockTriggerScanCode = kb.MakeCode;
					m_keyCaptureActive = false;
					m_cameraLockCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				// Handle Auto Wisp key capture
				for (auto& seq : m_wispSequences) {
					if (m_keyCaptureActive && seq.capturingKey) {
						if (isRightShift) {
							return;
						}
						seq.key = eventString;
						seq.scanCode = kb.MakeCode;
						m_keyCaptureActive = false;
						seq.capturingKey = false;
						MarkSettingsDirty();
						InvalidateRect(m_hwnd, nullptr, FALSE);
						return;
					}
				}
				
				if (m_keyCaptureActive && m_quickTurnCapturing) {
					if (isRightShift) {
						return;
					}
					m_quickTurnKey = eventString;
					m_quickTurnTriggerScanCode = kb.MakeCode;
					m_keyCaptureActive = false;
					m_quickTurnCapturing = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
				}
				
				if (g_crosshairRunning && !m_keyCaptureActive && kb.MakeCode == m_cameraLockTriggerScanCode) {
					if (IsRobloxFound()) {
						HWND robloxWindow = WinRTCapture::FindRobloxWindow();
						if (robloxWindow) {
							HWND foregroundWindow = GetForegroundWindow();
							bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
							if (robloxInFocus) {
								Crosshair::TriggerRestoration();
								if (!Crosshair::IsCameraLockTimeWindowActive()) {
									DWORD currentTime = GetTickCount();
									Crosshair::SetCameraLockTimeWindow(true, currentTime);
								}
							}
						}
					}
				}
				for (int i = 0; i < 10; i++) {
					if (m_keyCaptureActive && m_hotbarSlotCapturing[i]) {
						if (isRightShift) {
							return;
						}
						m_hotbarSlotKeys[i] = eventString;
						m_keyCaptureActive = false;
						m_hotbarSlotCapturing[i] = false;
						MarkSettingsDirty();
						InvalidateRect(m_hwnd, nullptr, FALSE);
						break;
					}
				}
			}
		} else if (ri->header.dwType == RIM_TYPEMOUSE) {
			USHORT buttonFlags = ri->data.mouse.usButtonFlags;
			if (m_rollM1TriggerScanCode == 0 && MouseButtonUpMatchesKey(buttonFlags, m_rollM1Key)) m_rollM1TriggerKeyHeld = false;
			if (m_rollSpitTriggerScanCode == 0 && MouseButtonUpMatchesKey(buttonFlags, m_rollSpitTriggerKey)) m_rollSpitTriggerKeyHeld = false;
			if (m_goldenTongueTriggerScanCode == 0 && MouseButtonUpMatchesKey(buttonFlags, m_goldenTongueKey)) m_goldenTongueTriggerKeyHeld = false;
			if (m_quickTurnTriggerScanCode == 0 && m_quickTurnMouseButtonIndex == 0 && MouseButtonUpMatchesKey(buttonFlags, m_quickTurnKey)) m_quickTurnTriggerKeyHeld = false;
			if (m_rollParryScanCode == 0 && MouseButtonUpMatchesKey(buttonFlags, m_rollParryKey)) m_rollParryTriggerKeyHeld = false;
			if (g_keystrokesRunning) {
				if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) Keystrokes::SetKeyState(VK_LBUTTON, true);
				if (buttonFlags & RI_MOUSE_LEFT_BUTTON_UP) Keystrokes::SetKeyState(VK_LBUTTON, false);
				if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) Keystrokes::SetKeyState(VK_RBUTTON, true);
				if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP) Keystrokes::SetKeyState(VK_RBUTTON, false);
			}
			if (g_cpsRunning) {
				if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) CPS::RegisterLeftClick();
				if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) CPS::RegisterRightClick();
			}
			
			// Track M1 for lightspeed reflex
			if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
				g_lightspeedM1PressTime = GetTickCount();
			}

			if (g_holdM1Running) {
				if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) HoldM1::HandleMouseDown();
				if (buttonFlags & RI_MOUSE_LEFT_BUTTON_UP) HoldM1::HandleMouseUp();
			}
			
			if (m_criticalAttackTriggerScanCode == 0 && !eventString.empty()) {
				if (_wcsicmp(eventString.c_str(), m_criticalAttackKey.c_str()) == 0) {
					HoldM1::HandleCriticalAttackKeyDown();
				}
			}
			
			// Check if any hotbar slot key (1-10) is pressed to trigger Mantra (mouse buttons)
			if (!eventString.empty()) {
				for (int i = 0; i < 10; i++) {
					if (!m_hotbarSlotKeys[i].empty() && _wcsicmp(eventString.c_str(), m_hotbarSlotKeys[i].c_str()) == 0) {
						HoldM1::HandleMantraKeyDown();
						break;
					}
				}
			}
			
				if (g_zoomRunning && (buttonFlags & 0x0400)) {
					if (WinRTCapture::IsCaptureInitialized()) {
						int clientWidth = WinRTCapture::GetCaptureWidth();
						int clientHeight = WinRTCapture::GetCaptureHeight();
						int screenWidth = GetSystemMetrics(SM_CXSCREEN);
						int screenHeight = GetSystemMetrics(SM_CYSCREEN);
						bool isFullscreen = (clientWidth == screenWidth && clientHeight == screenHeight);
						if (isFullscreen) {
							short delta = (short)ri->data.mouse.usButtonData;
							Zoom::OnMouseWheel(delta);
						}
					}
				}
			
			if (g_rollM1Running && !m_keyCaptureActive && !eventString.empty()) {
				if (_wcsicmp(eventString.c_str(), m_rollM1Key.c_str()) == 0) {
					if (!m_rollM1TriggerKeyHeld) {
						m_rollM1TriggerKeyHeld = true;
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								if (robloxInFocus) {
									RollM1::TriggerRoll();
								}
							}
						}
					}
				} else {
					m_rollM1TriggerKeyHeld = false;
				}
			}
			
			if (g_goldenTongueRunning && !m_keyCaptureActive && !eventString.empty()) {
				if (_wcsicmp(eventString.c_str(), m_goldenTongueKey.c_str()) == 0) {
					if (!m_goldenTongueTriggerKeyHeld) {
						m_goldenTongueTriggerKeyHeld = true;
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								if (robloxInFocus) {
									GoldenTongue::TriggerGoldenTongue();
								}
							}
						}
					}
				} else {
					m_goldenTongueTriggerKeyHeld = false;
				}
			}
			
			if (!m_keyCaptureActive && !eventString.empty()) {
				bool autoWispEnabled = (m_modCards.size() > 4 && m_modCards[4].enabled);
				if (autoWispEnabled && IsRobloxFound()) {
					HWND robloxWindow = WinRTCapture::FindRobloxWindow();
					if (robloxWindow) {
						HWND foregroundWindow = GetForegroundWindow();
						bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
						if (robloxInFocus) {
							USHORT buttonFlags = ri->data.mouse.usButtonFlags;
							bool isButtonDown = false;
							for (int bit = 0; bit < 16; ++bit) {
								USHORT flag = (USHORT)(1 << bit);
								if (flag == 0x0400 || flag == 0x0800) continue;
								if ((buttonFlags & flag) && !(bit & 1)) {
									isButtonDown = true;
									break;
								}
							}
							if (isButtonDown) {
								for (auto& seq : m_wispSequences) {
									if (seq.scanCode == 0 && !seq.key.empty() && _wcsicmp(eventString.c_str(), seq.key.c_str()) == 0) {
									std::vector<wchar_t> sequenceChars;
									for (const auto& charBox : seq.charBoxes) {
										if (charBox.ch != 0) {
											sequenceChars.push_back(charBox.ch);
										}
									}
									if (!sequenceChars.empty()) {
										std::thread([sequenceChars]() {
											for (size_t i = 0; i < sequenceChars.size(); i++) {
												wchar_t ch = sequenceChars[i];
												WORD vk = 0;
												if (ch >= 'A' && ch <= 'Z') {
													vk = (WORD)ch;
												} else if (ch >= 'a' && ch <= 'z') {
													vk = (WORD)(ch - 'a' + 'A');
												}
												if (vk != 0) {
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
												std::this_thread::sleep_for(std::chrono::milliseconds(150));
											}
										}).detach();
									}
									break;
								}
							}
							}
						}
					}
				}
			}
			
			if (g_quickTurnRunning && !m_keyCaptureActive && m_quickTurnMouseButtonIndex != 0) {
				USHORT buttonFlags = ri->data.mouse.usButtonFlags;
				int downBit = (m_quickTurnMouseButtonIndex - 1) * 2;
				int upBit = downBit + 1;
				USHORT downFlag = (USHORT)(1 << downBit);
				USHORT upFlag = (USHORT)(1 << upBit);
				
				if (buttonFlags & downFlag) {
					if (!m_quickTurnTriggerKeyHeld) {
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								bool shiftLockActive = Crosshair::IsShiftLock();
								if (robloxInFocus && shiftLockActive) {
									m_quickTurnTriggerKeyHeld = true;
									QuickTurn::TriggerTurn();
								}
							}
						}
					}
				}
				if (buttonFlags & upFlag) {
					if (m_quickTurnTriggerKeyHeld) {
						if (m_quickTurnToggleMode) {
							// In toggle mode: reset state so next press can turn again
							QuickTurn::ForceResetState();
							m_quickTurnTriggerKeyHeld = false;
						} else {
							// In hold mode: turn back on release
							if (IsRobloxFound()) {
								HWND robloxWindow = WinRTCapture::FindRobloxWindow();
								if (robloxWindow) {
									HWND foregroundWindow = GetForegroundWindow();
									bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
									bool shiftLockActive = Crosshair::IsShiftLock();
									if (robloxInFocus && shiftLockActive) {
										QuickTurn::ReleaseTurn();
									}
								}
							}
							m_quickTurnTriggerKeyHeld = false;
						}
					}
				}
			}
			
			if (g_quickTurnRunning && !m_keyCaptureActive && !eventString.empty() && m_quickTurnMouseButtonIndex == 0) {
				if (_wcsicmp(eventString.c_str(), m_quickTurnKey.c_str()) == 0) {
					if (!m_quickTurnTriggerKeyHeld) {
						m_quickTurnTriggerKeyHeld = true;
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								bool shiftLockActive = Crosshair::IsShiftLock();
								if (robloxInFocus && shiftLockActive) {
									QuickTurn::TriggerTurn();
								}
							}
						}
					}
				} else {
					if (m_quickTurnTriggerKeyHeld && m_quickTurnToggleMode) {
						// In toggle mode, don't reset on other key presses
					} else {
						m_quickTurnTriggerKeyHeld = false;
					}
				}
			}
			
			if (g_rollSpitRunning && !m_keyCaptureActive && !eventString.empty() && m_rollSpitTriggerScanCode == 0) {
				if (_wcsicmp(eventString.c_str(), m_rollSpitTriggerKey.c_str()) == 0) {
					if (!m_rollSpitTriggerKeyHeld) {
						m_rollSpitTriggerKeyHeld = true;
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								if (robloxInFocus) {
									RollSpit::TriggerRollSpit();
								}
							}
						}
					}
				} else {
					m_rollSpitTriggerKeyHeld = false;
				}
			}
			
			if (g_rollParryRunning && !m_keyCaptureActive && !eventString.empty() && m_rollParryScanCode == 0) {
				bool betterParryModEnabled = false;
				if (m_modCards.size() > 13) {
					betterParryModEnabled = m_modCards[13].enabled;
				}
				if (betterParryModEnabled && _wcsicmp(eventString.c_str(), m_rollParryKey.c_str()) == 0) {
					if (!m_rollParryTriggerKeyHeld) {
						m_rollParryTriggerKeyHeld = true;
						if (IsRobloxFound()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							if (robloxWindow) {
								HWND foregroundWindow = GetForegroundWindow();
								bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
								if (robloxInFocus) {
									RollParry::TriggerRollParry();
								}
							}
						}
					}
				} else {
					m_rollParryTriggerKeyHeld = false;
				}
			}
			
			if (m_keyCaptureActive && m_rollM1Capturing && !eventString.empty()) {
				m_rollM1Key = eventString;
				m_rollM1TriggerScanCode = 0;
				m_keyCaptureActive = false;
				m_rollM1Capturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}

			if (m_keyCaptureActive && m_rollCritCapturing && !eventString.empty()) {
				m_rollCritKey = eventString;
				m_rollCritScanCode = 0;
				m_keyCaptureActive = false;
				m_rollCritCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			
			if (m_keyCaptureActive && m_goldenTongueCapturing && !eventString.empty()) {
				m_goldenTongueKey = eventString;
				m_goldenTongueTriggerScanCode = 0;
				m_keyCaptureActive = false;
				m_goldenTongueCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			
			if (m_keyCaptureActive && m_parryCapturing && !eventString.empty()) {
				m_parryKey = eventString;
				m_parryScanCode = 0;
				Keystrokes::SetParryScanCode(0);
				Keystrokes::SetParryKeyName(m_parryKey);
				RollParry::SetParryKeyScanCode(0);
				m_keyCaptureActive = false;
				m_parryCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}

			if (m_keyCaptureActive && m_rollParryCapturing && !eventString.empty()) {
				m_rollParryKey = eventString;
				m_rollParryScanCode = 0;
				m_keyCaptureActive = false;
				m_rollParryCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}

			if (m_keyCaptureActive && m_quickTurnCapturing) {
				USHORT buttonFlags = ri->data.mouse.usButtonFlags;
				for (int bit = 0; bit < 16; ++bit) {
					USHORT flag = (USHORT)(1 << bit);
					if (flag == 0x0400 || flag == 0x0800) continue;
					if ((buttonFlags & flag) && !(bit & 1)) {
						int buttonIndex = (bit >> 1) + 1;
						m_quickTurnKey = L"M" + std::to_wstring(buttonIndex);
						m_quickTurnTriggerScanCode = 0;
						m_quickTurnMouseButtonIndex = buttonIndex;
						m_keyCaptureActive = false;
						m_quickTurnCapturing = false;
						InvalidateRect(m_hwnd, nullptr, FALSE);
						break;
					}
				}
			}
			
			if (m_keyCaptureActive && m_quickTurnCapturing && !eventString.empty() && ri->header.dwType == RIM_TYPEKEYBOARD) {
				const RAWKEYBOARD& kb = ri->data.keyboard;
				m_quickTurnKey = eventString;
				m_quickTurnTriggerScanCode = kb.MakeCode;
				m_quickTurnMouseButtonIndex = 0;
				m_keyCaptureActive = false;
				m_quickTurnCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			
			if (m_keyCaptureActive && m_rollSpitTriggerCapturing && !eventString.empty()) {
				m_rollSpitTriggerKey = eventString;
				m_rollSpitTriggerScanCode = 0;
				m_keyCaptureActive = false;
				m_rollSpitTriggerCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			
			if (m_keyCaptureActive && m_rollSpitSpitCapturing && !eventString.empty()) {
				m_rollSpitSpitKey = eventString;
				m_rollSpitSpitScanCode = 0;
				m_keyCaptureActive = false;
				m_rollSpitSpitCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			if (m_keyCaptureActive && m_rollCastCapturing && !eventString.empty()) {
				m_rollCastKey = eventString;
				m_keyCaptureActive = false;
				m_rollCastCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			if (m_keyCaptureActive && m_dodgeCapturing && !eventString.empty()) {
				m_dodgeKey = eventString;
				RollM1::SetRollKey(m_dodgeKey);
				RollM1::SetRollKeyScanCode(0);
				m_dodgeScanCode = 0;
				Keystrokes::SetDodgeScanCode(0);
				Keystrokes::SetDodgeKeyName(m_dodgeKey);
				RollCast::SetDodgeKeyScanCode(0);
				RollCritical::SetDodgeKeyScanCode(0);
				RollParry::SetDodgeKeyScanCode(0);
				m_keyCaptureActive = false;
				m_dodgeCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			if (m_keyCaptureActive && m_parryCapturing && !eventString.empty()) {
				m_parryKey = eventString;
				m_parryScanCode = 0;
				Keystrokes::SetParryScanCode(0);
				Keystrokes::SetParryKeyName(m_parryKey);
				RollParry::SetParryKeyScanCode(0);
				m_keyCaptureActive = false;
				m_parryCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			if (m_keyCaptureActive && m_openMapCapturing && !eventString.empty()) {
				m_openMapKey = eventString;
				m_openMapTriggerScanCode = 0; // Clear scan code for mouse buttons
				MapCast::SetOpenMapKeyScanCode(0);
				m_keyCaptureActive = false;
				m_openMapCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			if (m_keyCaptureActive && m_criticalAttackCapturing) {
				USHORT buttonFlags = ri->data.mouse.usButtonFlags;
				for (int bit = 0; bit < 16; ++bit) {
					USHORT flag = (USHORT)(1 << bit);
					if (flag == 0x0400 || flag == 0x0800) continue;
					if ((buttonFlags & flag) && !(bit & 1)) {
						int buttonIndex = (bit >> 1) + 1;
						m_criticalAttackKey = L"M" + std::to_wstring(buttonIndex);
						m_criticalAttackTriggerScanCode = 0;
						m_criticalAttackMouseButtonIndex = buttonIndex;
						RollCritical::SetCriticalKeyScanCode(0);
						RollCritical::SetCriticalKeyMouseButton(buttonIndex);
						m_keyCaptureActive = false;
						m_criticalAttackCapturing = false;
						InvalidateRect(m_hwnd, nullptr, FALSE);
						break;
					}
				}
			}

			if (m_keyCaptureActive && m_criticalAttackCapturing && !eventString.empty() && ri->header.dwType == RIM_TYPEKEYBOARD) {
				const RAWKEYBOARD& kb = ri->data.keyboard;
				m_criticalAttackKey = eventString;
				m_criticalAttackTriggerScanCode = 0; // Clear scan code for mouse buttons
				m_criticalAttackMouseButtonIndex = 0;
				RollCritical::SetCriticalKeyScanCode(0);
				RollCritical::SetCriticalKeyMouseButton(0);
				m_keyCaptureActive = false;
				m_criticalAttackCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			if (m_keyCaptureActive && m_zoomCapturing && !eventString.empty()) {
				m_zoomKey = eventString;
				m_zoomTriggerScanCode = 0;
				m_keyCaptureActive = false;
				m_zoomCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			if (m_keyCaptureActive && m_cameraLockCapturing && !eventString.empty()) {
				m_cameraLockKey = eventString;
				m_cameraLockTriggerScanCode = 0; // Clear scan code for mouse buttons
				m_keyCaptureActive = false;
				m_cameraLockCapturing = false;
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			
			for (auto& seq : m_wispSequences) {
				if (m_keyCaptureActive && seq.capturingKey && !eventString.empty()) {
					seq.key = eventString;
					seq.scanCode = 0;
					m_keyCaptureActive = false;
					seq.capturingKey = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
					break;
				}
			}
			
			if (g_crosshairRunning && !m_keyCaptureActive && !eventString.empty()) {
				if (_wcsicmp(eventString.c_str(), m_cameraLockKey.c_str()) == 0) {
					if (IsRobloxFound()) {
						HWND robloxWindow = WinRTCapture::FindRobloxWindow();
						if (robloxWindow) {
							HWND foregroundWindow = GetForegroundWindow();
							bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
							if (robloxInFocus) {
								DWORD currentTime = GetTickCount();
								Crosshair::SetCameraLockTimeWindow(true, currentTime);
							}
						}
					}
				}
			}
			for (int i = 0; i < 10; i++) {
				if (m_keyCaptureActive && m_hotbarSlotCapturing[i] && !eventString.empty()) {
					m_hotbarSlotKeys[i] = eventString;
					m_keyCaptureActive = false;
					m_hotbarSlotCapturing[i] = false;
					MarkSettingsDirty();
					InvalidateRect(m_hwnd, nullptr, FALSE);
					break;
				}
			}
		}
	}
    
    void CreateMainWindow() {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"PistachioCreamMacro";
        
        RegisterClassExW(&wc);
        
        m_hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOPARENTNOTIFY,
            L"PistachioCreamMacro",
            L"Pistachio Cream Macro",
            WS_POPUP,
            0, 0, static_cast<int>(kWindowWidth), static_cast<int>(kWindowHeight),
            nullptr, nullptr, GetModuleHandle(nullptr), this
        );
        if (!m_hwnd) return;
        
        m_bufferWidth = static_cast<int>(kWindowWidth);
        m_bufferHeight = static_cast<int>(kWindowHeight);
        m_memDC = CreateCompatibleDC(nullptr);
        if (!m_memDC) return;
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = m_bufferWidth;
        bmi.bmiHeader.biHeight = -m_bufferHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        m_dibBitmap = CreateDIBSection(m_memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!m_dibBitmap) return;
        SelectObject(m_memDC, m_dibBitmap);
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            0, 0,
            D2D1_RENDER_TARGET_USAGE_NONE,
            D2D1_FEATURE_LEVEL_DEFAULT
        );
        if (FAILED(m_d2dFactory->CreateDCRenderTarget(&props, &m_renderTarget))) return;
        m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        
        CreateBrushes();
    }
    bool ModMatchesSearch(const ModCard& card) {
        if (m_searchText.empty()) return true;
        
        std::wstring searchLower = m_searchText;
        std::wstring titleLower = card.title;
        std::wstring descLower = card.description;
        
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::towlower);
        std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::towlower);
        std::transform(descLower.begin(), descLower.end(), descLower.begin(), ::towlower);
        
        return titleLower.find(searchLower) == 0;
    }
    
    bool SettingsMatchesSearch(const std::wstring& settingName) {
        if (m_searchText.empty()) return true;
        
        std::wstring searchLower = m_searchText;
        std::wstring settingLower = settingName;
        
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::towlower);
        std::transform(settingLower.begin(), settingLower.end(), settingLower.begin(), ::towlower);
        
        return settingLower.find(searchLower) == 0;
    }
    
    void CreateBrushes() {
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0f0f10), &m_backgroundBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b), &m_sidebarBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x151516), &m_cardBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0xffffff), &m_textBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &m_descBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &m_searchBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF), &m_whiteBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x2a2a2d), &m_iconBrush);
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0f0f10), &m_panel2Brush);
    }
    
    void InitializeModCards() {
        // Initialize Auto Wisp with one empty sequence
        if (m_wispSequences.empty()) {
            m_wispSequences.push_back(WispSequence());
        }
        // Content area: 700px wide (760 - 60 sidebar)
        // Cards: 160px wide with 12px spacing between them
        // Total grid width: 4 * 160 + 3 * 12 = 640 + 36 = 676px
        // Center this in the 700px content area: (700 - 676) / 2 = 12px margin
        int contentAreaWidth = 700; // 760 - 60 (sidebar)
        int cardWidth = 160;
        int cardHeight = 140;
        int spacing = 12;
        int gridWidth = 4 * cardWidth + 3 * spacing; // 676px
        int gridStartX = 60 + (contentAreaWidth - gridWidth) / 2; // Center the grid
        int startY = 76; // 60px header + 16px margin
        
        m_modCards = {
            {{gridStartX, startY, gridStartX + cardWidth, startY + cardHeight}, L"Roll M1", L"Auto-roll after M1 attacks"},
            {{gridStartX + cardWidth + spacing, startY, gridStartX + cardWidth + spacing + cardWidth, startY + cardHeight}, L"Parry Bar", L"Displays parry interface"},
            {{gridStartX + (cardWidth + spacing) * 2, startY, gridStartX + (cardWidth + spacing) * 2 + cardWidth, startY + cardHeight}, L"Map Cast", L"Auto-map after mantra cast"},
            {{gridStartX + (cardWidth + spacing) * 3, startY, gridStartX + (cardWidth + spacing) * 3 + cardWidth, startY + cardHeight}, L"Keystrokes", L"Key press display"},
            // Second row: 16px spacing from first row
            {{gridStartX, startY + cardHeight + spacing, gridStartX + cardWidth, startY + cardHeight + spacing + cardHeight}, L"Auto Wisp", L"Auto-wisp key sequences"},
            {{gridStartX + cardWidth + spacing, startY + cardHeight + spacing, gridStartX + cardWidth + spacing + cardWidth, startY + cardHeight + spacing + cardHeight}, L"CPS", L"Click counter"},
            {{gridStartX + (cardWidth + spacing) * 2, startY + cardHeight + spacing, gridStartX + (cardWidth + spacing) * 2 + cardWidth, startY + cardHeight + spacing + cardHeight}, L"Roll Cast", L"Auto-roll after mantra cast"},
            {{gridStartX + (cardWidth + spacing) * 3, startY + cardHeight + spacing, gridStartX + (cardWidth + spacing) * 3 + cardWidth, startY + cardHeight + spacing + cardHeight}, L"Hold M1", L"Auto-clicks M1 on hold"},
            // Third row
            {{gridStartX, startY + (cardHeight + spacing) * 2, gridStartX + cardWidth, startY + (cardHeight + spacing) * 2 + cardHeight}, L"Crosshair", L"Custom shiftlock replacement"},
            {{gridStartX + cardWidth + spacing, startY + (cardHeight + spacing) * 2, gridStartX + cardWidth + spacing + cardWidth, startY + (cardHeight + spacing) * 2 + cardHeight}, L"Roll Spit", L"Auto-roll after taunt usage"},
            {{gridStartX + (cardWidth + spacing) * 2, startY + (cardHeight + spacing) * 2, gridStartX + (cardWidth + spacing) * 2 + cardWidth, startY + (cardHeight + spacing) * 2 + cardHeight}, L"Motion Blur", L"Enable motion blur in your game"},
            {{gridStartX + (cardWidth + spacing) * 3, startY + (cardHeight + spacing) * 2, gridStartX + (cardWidth + spacing) * 3 + cardWidth, startY + (cardHeight + spacing) * 2 + cardHeight}, L"Zoom", L"Magnifier zoom"},
            {{gridStartX, startY + (cardHeight + spacing) * 3, gridStartX + cardWidth, startY + (cardHeight + spacing) * 3 + cardHeight}, L"Quick Turn", L"Turn 180\u00B0 on key press"},
            {{gridStartX + cardWidth + spacing, startY + (cardHeight + spacing) * 3, gridStartX + cardWidth + spacing + cardWidth, startY + (cardHeight + spacing) * 3 + cardHeight}, L"Better Parry", L"Feint on parry during M1"},
            {{gridStartX + (cardWidth + spacing) * 2, startY + (cardHeight + spacing) * 3, gridStartX + (cardWidth + spacing) * 2 + cardWidth, startY + (cardHeight + spacing) * 3 + cardHeight}, L"Golden Tongue", L"Auto-trigger golden tongue"},
            {{gridStartX + (cardWidth + spacing) * 3, startY + (cardHeight + spacing) * 3, gridStartX + (cardWidth + spacing) * 3 + cardWidth, startY + (cardHeight + spacing) * 3 + cardHeight}, L"Gamma", L"Adjust in-game brightness"}
        };
        
        // Close button: 35x35, positioned in top-right with 15px margin from right edge
        m_closeButtonRect = {760 - 35 - 15, 12, 760 - 15, 12 + 35};
        // Search bar: 300px wide, centered in header (60px sidebar + 16px margin to right edge)
        int searchWidth = 300;
        int searchHeight = 35;
        int headerWidth = 700; // 760 - 60 (sidebar)
        int searchX = 60 + (headerWidth - searchWidth) / 2; // Center in header
        m_searchRect = {searchX, 12, searchX + searchWidth, 12 + searchHeight};
        m_logoRect = {10, 12, 50, 52};
        
        int exitIconSize = 30;
        int exitIconMargin = 10;
        m_exitButtonRect = {exitIconMargin, (int)kContentHeight - exitIconSize - exitIconMargin, exitIconMargin + exitIconSize, (int)kContentHeight - exitIconMargin};
        int panelsIconSize = 18;
        int panelsIconMargin = exitIconMargin + (exitIconSize - panelsIconSize) / 2 - 1;
        int panelsIconSpacing = 14;
        m_panelsButtonRect = {panelsIconMargin, (int)kContentHeight - exitIconSize - exitIconMargin - panelsIconSize - panelsIconSpacing, panelsIconMargin + panelsIconSize, (int)kContentHeight - exitIconSize - exitIconMargin - panelsIconSpacing};
    }
    
    void CleanupDirect2D() {
        if (m_backgroundBrush) m_backgroundBrush->Release();
        if (m_sidebarBrush) m_sidebarBrush->Release();
        if (m_cardBrush) m_cardBrush->Release();
        if (m_textBrush) m_textBrush->Release();
        if (m_descBrush) m_descBrush->Release();
        if (m_searchBrush) m_searchBrush->Release();
        if (m_whiteBrush) m_whiteBrush->Release();
        if (m_iconBrush) m_iconBrush->Release();
        if (m_titleFormat) m_titleFormat->Release();
        if (m_descFormat) m_descFormat->Release();
        if (m_tagFormat) m_tagFormat->Release();
        if (m_searchFormat) m_searchFormat->Release();
        if (m_closeFormat) m_closeFormat->Release();
        if (m_renderTarget) { m_renderTarget->Release(); m_renderTarget = nullptr; }
        if (m_dibBitmap) { DeleteObject(m_dibBitmap); m_dibBitmap = nullptr; }
        if (m_memDC) { DeleteDC(m_memDC); m_memDC = nullptr; }
        if (m_writeFactory) { m_writeFactory->Release(); m_writeFactory = nullptr; }
        if (m_d2dFactory) { m_d2dFactory->Release(); m_d2dFactory = nullptr; }
    }
    
    float GetContentOffsetX() const {
        return (kWindowWidth - kContentWidth) * 0.5f;
    }
    
    float GetContentOffsetY() const {
        return (kWindowHeight - kContentHeight) * 0.5f;
    }
    
    D2D1::Matrix3x2F GetBaseRenderTransform() const {
        float offsetX = GetContentOffsetX();
        float offsetY = GetContentOffsetY();
        D2D1::Matrix3x2F translate = D2D1::Matrix3x2F::Translation(offsetX, offsetY);
        float cx = offsetX + kContentWidth * 0.5f;
        float cy = offsetY + kContentHeight * 0.5f;
        D2D1::Matrix3x2F scale = D2D1::Matrix3x2F::Scale(m_scale, m_scale, D2D1::Point2F(cx, cy));
        return scale * translate;
    }
    
    POINT MapPointToLogical(const POINT& pt) const {
        float offsetX = GetContentOffsetX();
        float offsetY = GetContentOffsetY();
        float cx = kContentWidth * 0.5f;
        float cy = kContentHeight * 0.5f;
        float scale = m_scale;
        if (scale < 0.001f) scale = 0.001f;
        float localX = pt.x - offsetX;
        float localY = pt.y - offsetY;
        float logicalX = (localX - cx) / scale + cx;
        float logicalY = (localY - cy) / scale + cy;
        POINT result;
        result.x = (LONG)(logicalX + 0.5f);
        result.y = (LONG)(logicalY + 0.5f);
        return result;
    }
    
    bool IsPointWithinVisibleContent(const POINT& pt) const {
        float offsetX = GetContentOffsetX();
        float offsetY = GetContentOffsetY();
        float scaledWidth = kContentWidth * m_scale;
        float scaledHeight = kContentHeight * m_scale;
        float left = offsetX + (kContentWidth - scaledWidth) * 0.5f;
        float top = offsetY + (kContentHeight - scaledHeight) * 0.5f;
        float right = left + scaledWidth;
        float bottom = top + scaledHeight;
        return pt.x >= left && pt.x <= right && pt.y >= top && pt.y <= bottom;
    }
    
    void Render() {
        if (!m_renderTarget || !m_memDC) return;
        RECT drawRect = { 0, 0, m_bufferWidth, m_bufferHeight };
        m_renderTarget->BindDC(m_memDC, &drawRect);
        m_renderTarget->BeginDraw();
        m_renderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
        D2D1::Matrix3x2F baseTransform = GetBaseRenderTransform();
        m_renderTarget->SetTransform(baseTransform);
        
        ID2D1SolidColorBrush* darkBg = nullptr;
        if (SUCCEEDED(m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 1.0f), &darkBg))) {
            D2D1_ROUNDED_RECT mainRect = D2D1::RoundedRect(
                D2D1::RectF(0.0f, 0.0f, kContentWidth, kContentHeight),
                14.0f,
                14.0f
            );
            m_renderTarget->FillRoundedRectangle(mainRect, darkBg);
            darkBg->Release();
        }
        
        RenderSidebar();
        RenderHeader();
        RenderModCards();
        
        DrawPanelsIconAnimated(m_panelsButtonRect, true, 0.0f, m_panelsButtonHovered, m_panelsButtonScale);
        {
            float midY = (m_panelsButtonRect.bottom + m_exitButtonRect.top) * 0.5f + 2.0f;
            float cx = (m_exitButtonRect.left + m_exitButtonRect.right) * 0.5f - 1.0f;
            float halfLen = 11.0f;
            ID2D1SolidColorBrush* sepBrush = nullptr;
            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &sepBrush);
            if (sepBrush) {
                m_renderTarget->DrawLine(D2D1::Point2F(cx - halfLen, midY), D2D1::Point2F(cx + halfLen, midY), sepBrush, 2.0f);
                sepBrush->Release();
            }
        }
        DrawExitIconAnimated(m_exitButtonRect, true, 1.0f, m_exitButtonHovered, m_exitButtonScale);
        
        m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        m_renderTarget->EndDraw();
        
        HDC screenDC = GetDC(nullptr);
        if (screenDC) {
            POINT ptSrc = { 0, 0 };
            RECT wr;
            GetWindowRect(m_hwnd, &wr);
            POINT ptDst = { wr.left, wr.top };
            SIZE size = { m_bufferWidth, m_bufferHeight };
            float opacity = m_opacity.load();
            if (opacity < 0.0f) opacity = 0.0f;
            if (opacity > 1.0f) opacity = 1.0f;
            BLENDFUNCTION blend = { AC_SRC_OVER, 0, static_cast<BYTE>(opacity * 255.0f), AC_SRC_ALPHA };
            UpdateLayeredWindow(m_hwnd, screenDC, &ptDst, &size, m_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
            ReleaseDC(nullptr, screenDC);
        }
    }
    
    void RenderSidebar() {
        RECT sidebarRect = {0, 0, 60, 550};
        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(sidebarRect.left, sidebarRect.top, sidebarRect.right, sidebarRect.bottom), 12, 12), m_sidebarBrush);
        
        ID2D1SolidColorBrush* borderBrush = nullptr;
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.03f), &borderBrush);
        m_renderTarget->DrawLine(D2D1::Point2F(60, 0), D2D1::Point2F(60, 550), borderBrush, 1.0f);
        borderBrush->Release();
        
        RECT logoRect = {10, 12, 50, 52};
        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(logoRect.left, logoRect.top, logoRect.right, logoRect.bottom), 10, 10), m_whiteBrush);
        
        for (int i = 0; i < 3; i++) {
            RECT itemRect = {10, 67 + i * 50, 50, 107 + i * 50};
            if (i == 0) m_modsRect = itemRect;
            if (i == 1) m_profileRect = itemRect;
            if (i == 2) m_settingsRect = itemRect;
            bool isMods = (i == 0);
            bool isProfile = (i == 1);
            bool selected = (isMods && !m_showSettings && !m_showProfile) || (isProfile && m_showProfile) || (!isMods && !isProfile && i == 2 && m_showSettings);
            ID2D1SolidColorBrush* bgBrush = nullptr;
            if (selected) {
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0xffffff), &bgBrush);
            } else {
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0f0f10), &bgBrush);
            }
            m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(itemRect.left, itemRect.top, itemRect.right, itemRect.bottom), 10, 10), bgBrush);
            bgBrush->Release();
        }
        
        // P icon (top icon above mods) - always white
        RECT pIconRect = {9, 30, 33, 54};
        DrawPIconAnimated(pIconRect, true, 1.0f);
        
        // Mods icon (top button) - grid icon
        RECT modsIconRect = {19, 77, 39, 97};
        bool modsSelected = !m_showSettings && !m_showProfile;
        float modsCenterX = (modsIconRect.left + modsIconRect.right) * 0.5f;
        float modsCenterY = (modsIconRect.top + modsIconRect.bottom) * 0.5f;
        D2D1::Matrix3x2F modsOriginalTransform;
        m_renderTarget->GetTransform(&modsOriginalTransform);
        D2D1::Matrix3x2F modsScaleTransform = D2D1::Matrix3x2F::Scale(m_modsIconScale, m_modsIconScale, D2D1::Point2F(modsCenterX, modsCenterY));
        m_renderTarget->SetTransform(modsScaleTransform * modsOriginalTransform);
        ID2D1SolidColorBrush* modsStroke = nullptr;
        D2D1_COLOR_F modsColor = modsSelected ? D2D1::ColorF(0x000000) : D2D1::ColorF(0x888888);
        m_renderTarget->CreateSolidColorBrush(modsColor, &modsStroke);
        D2D1_ANTIALIAS_MODE prevAA = m_renderTarget->GetAntialiasMode();
        m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        float rr = 2.0f;
        float l = (float)modsIconRect.left;
        float t = (float)modsIconRect.top;
        float r = (float)modsIconRect.right;
        float b = (float)modsIconRect.bottom;
        float cx = (modsIconRect.left + modsIconRect.right) * 0.5f;
        float cy = (modsIconRect.top + modsIconRect.bottom) * 0.5f;
        float strokeWidth = modsSelected ? 2.0f : 1.7f;
        ID2D1StrokeStyle* roundedStroke = nullptr;
        D2D1_STROKE_STYLE_PROPERTIES sprops = {};
        sprops.startCap = D2D1_CAP_STYLE_ROUND;
        sprops.endCap = D2D1_CAP_STYLE_ROUND;
        sprops.lineJoin = D2D1_LINE_JOIN_ROUND;
        sprops.miterLimit = 4.0f;
        sprops.dashStyle = D2D1_DASH_STYLE_SOLID;
        sprops.dashOffset = 0.0f;
        m_d2dFactory->CreateStrokeStyle(&sprops, nullptr, 0, &roundedStroke);
        float margin = 0.5f;
        float gap = 3.0f;
        float tileSize = (float)(int)(((r - l) - (2.0f * margin + gap)) * 0.5f);
        float tileR = 2.0f;
        float tl = l + floorf(margin + 0.5f);
        float tt = t + floorf(margin + 0.5f);
        float tr = tl + tileSize + gap;
        float tb = tt + tileSize + gap;
        {
            float left = tl, top = tt, right = tl + tileSize, bottom = tt + tileSize;
            ID2D1PathGeometry* g = nullptr; m_d2dFactory->CreatePathGeometry(&g);
            ID2D1GeometrySink* s = nullptr; g->Open(&s);
            s->SetSegmentFlags(D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN);
            s->BeginFigure(D2D1::Point2F(right, top), D2D1_FIGURE_BEGIN_HOLLOW);
            s->AddLine(D2D1::Point2F(left + tileR, top));
            s->AddArc(D2D1::ArcSegment(D2D1::Point2F(left, top + tileR), D2D1::SizeF(tileR, tileR), 0.0f, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            s->AddLine(D2D1::Point2F(left, bottom));
            s->AddLine(D2D1::Point2F(right, bottom));
            s->AddLine(D2D1::Point2F(right, top));
            s->EndFigure(D2D1_FIGURE_END_CLOSED);
            s->Close(); s->Release();
            m_renderTarget->DrawGeometry(g, modsStroke, strokeWidth, roundedStroke);
            g->Release();
        }
        {
            float left = tr, top = tt, right = tr + tileSize, bottom = tt + tileSize;
            ID2D1PathGeometry* g = nullptr; m_d2dFactory->CreatePathGeometry(&g);
            ID2D1GeometrySink* s = nullptr; g->Open(&s);
            s->SetSegmentFlags(D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN);
            s->BeginFigure(D2D1::Point2F(left, top), D2D1_FIGURE_BEGIN_HOLLOW);
            s->AddLine(D2D1::Point2F(right - tileR, top));
            s->AddArc(D2D1::ArcSegment(D2D1::Point2F(right, top + tileR), D2D1::SizeF(tileR, tileR), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            s->AddLine(D2D1::Point2F(right, bottom));
            s->AddLine(D2D1::Point2F(left, bottom));
            s->AddLine(D2D1::Point2F(left, top));
            s->EndFigure(D2D1_FIGURE_END_CLOSED);
            s->Close(); s->Release();
            m_renderTarget->DrawGeometry(g, modsStroke, strokeWidth, roundedStroke);
            g->Release();
        }
        {
            float left = tl, top = tb, right = tl + tileSize, bottom = tb + tileSize;
            ID2D1PathGeometry* g = nullptr; m_d2dFactory->CreatePathGeometry(&g);
            ID2D1GeometrySink* s = nullptr; g->Open(&s);
            s->SetSegmentFlags(D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN);
            s->BeginFigure(D2D1::Point2F(right, bottom), D2D1_FIGURE_BEGIN_HOLLOW);
            s->AddLine(D2D1::Point2F(left + tileR, bottom));
            s->AddArc(D2D1::ArcSegment(D2D1::Point2F(left, bottom - tileR), D2D1::SizeF(tileR, tileR), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            s->AddLine(D2D1::Point2F(left, top));
            s->AddLine(D2D1::Point2F(right, top));
            s->AddLine(D2D1::Point2F(right, bottom));
            s->EndFigure(D2D1_FIGURE_END_CLOSED);
            s->Close(); s->Release();
            m_renderTarget->DrawGeometry(g, modsStroke, strokeWidth, roundedStroke);
            g->Release();
        }
        {
            float left = tr, top = tb, right = tr + tileSize, bottom = tb + tileSize;
            ID2D1PathGeometry* g = nullptr; m_d2dFactory->CreatePathGeometry(&g);
            ID2D1GeometrySink* s = nullptr; g->Open(&s);
            s->SetSegmentFlags(D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN);
            s->BeginFigure(D2D1::Point2F(left, top), D2D1_FIGURE_BEGIN_HOLLOW);
            s->AddLine(D2D1::Point2F(right, top));
            s->AddLine(D2D1::Point2F(right, bottom - tileR));
            s->AddArc(D2D1::ArcSegment(D2D1::Point2F(right - tileR, bottom), D2D1::SizeF(tileR, tileR), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            s->AddLine(D2D1::Point2F(left, bottom));
            s->AddLine(D2D1::Point2F(left, top));
            s->EndFigure(D2D1_FIGURE_END_CLOSED);
            s->Close(); s->Release();
            m_renderTarget->DrawGeometry(g, modsStroke, strokeWidth, roundedStroke);
            g->Release();
        }
        if (roundedStroke) roundedStroke->Release();
        m_renderTarget->SetAntialiasMode(prevAA);
        if (modsStroke) modsStroke->Release();
        m_renderTarget->SetTransform(modsOriginalTransform);

        // User icon in the second item (middle)
        RECT userIconRect = {20, 127, 40, 147};
        bool userSelected = m_showProfile;
        float userCenterX = (userIconRect.left + userIconRect.right) * 0.5f;
        float userCenterY = (userIconRect.top + userIconRect.bottom) * 0.5f;
        float verticalOffset = (m_userIconScale - 1.0f) / 0.15f * 10.0f;
        float adjustedUserCenterY = userCenterY + verticalOffset;
        D2D1::Matrix3x2F userOriginalTransform;
        m_renderTarget->GetTransform(&userOriginalTransform);
        D2D1::Matrix3x2F userScaleTransform = D2D1::Matrix3x2F::Scale(m_userIconScale, m_userIconScale, D2D1::Point2F(userCenterX, adjustedUserCenterY));
        m_renderTarget->SetTransform(userScaleTransform * userOriginalTransform);
        ID2D1SolidColorBrush* userStroke = nullptr;
        D2D1_COLOR_F userColor = userSelected ? D2D1::ColorF(0x000000) : D2D1::ColorF(0x888888);
        m_renderTarget->CreateSolidColorBrush(userColor, &userStroke);
        DrawUserIcon(userIconRect, userStroke);
        if (userStroke) userStroke->Release();
        m_renderTarget->SetTransform(userOriginalTransform);

        // Settings icon in the third item (bottom)
        RECT settingsIconRect = {23, 167, 39, 183};
        float settingsCenterX = (settingsIconRect.left + settingsIconRect.right) * 0.5f;
        float baseCenterY = (settingsIconRect.top + settingsIconRect.bottom) * 0.5f;
        float settingsVerticalOffset = (m_settingsIconScale - 1.0f) / 0.15f * 10.0f;
        float settingsCenterY = baseCenterY + settingsVerticalOffset;
        D2D1::Matrix3x2F settingsOriginalTransform;
        m_renderTarget->GetTransform(&settingsOriginalTransform);
        D2D1::Matrix3x2F settingsScaleTransform = D2D1::Matrix3x2F::Scale(m_settingsIconScale, m_settingsIconScale, D2D1::Point2F(settingsCenterX, settingsCenterY));
        m_renderTarget->SetTransform(settingsScaleTransform * settingsOriginalTransform);
        ID2D1SolidColorBrush* gearStroke = nullptr;
        D2D1_COLOR_F gearColor = m_showSettings ? D2D1::ColorF(0x000000) : D2D1::ColorF(0x888888);
        m_renderTarget->CreateSolidColorBrush(gearColor, &gearStroke);
        ID2D1SolidColorBrush* saved = m_searchBrush;
        m_searchBrush = gearStroke;
        DrawSettingsIcon(settingsIconRect);
        m_searchBrush = saved;
        if (gearStroke) gearStroke->Release();
        m_renderTarget->SetTransform(settingsOriginalTransform);
    }
    void RenderHeader() {
        RECT headerRect = {52, 0, 760, 60};
        FillRightRoundedRect(headerRect, 12.0f);
        
        ID2D1SolidColorBrush* headerBorder = nullptr;
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.03f), &headerBorder);
        m_renderTarget->DrawLine(D2D1::Point2F(60, 60), D2D1::Point2F(760, 60), headerBorder, 1.0f);
        headerBorder->Release();
        
        ID2D1SolidColorBrush* searchBg = nullptr;
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &searchBg);
        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_searchRect.left, m_searchRect.top, m_searchRect.right, m_searchRect.bottom), 8, 8), searchBg);
        
        ID2D1SolidColorBrush* searchOutline = nullptr;
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, m_searchBorderOpacity), &searchOutline);
        m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_searchRect.left, m_searchRect.top, m_searchRect.right, m_searchRect.bottom), 8, 8), searchOutline, 1.0f);
        
        searchBg->Release();
        searchOutline->Release();
        
        // Search icon (magnifying glass) - 20x20 with 12px left margin, vertically centered
        int searchBarHeight = m_searchRect.bottom - m_searchRect.top;
        int iconSize = 20; // Much bigger icon
        int iconTop = m_searchRect.top + (searchBarHeight - iconSize) / 2; // Center 20px icon in search bar
        RECT searchIconRect = {m_searchRect.left + 12, iconTop, m_searchRect.left + 12 + iconSize, iconTop + iconSize};
        DrawSearchIcon(searchIconRect);
        
        if (m_searchFocused || !m_searchText.empty()) {
            // Draw the search text
            m_renderTarget->DrawText(m_searchText.c_str(), (UINT32)m_searchText.length(), m_searchFormat, D2D1::RectF(m_searchRect.left + 40, m_searchRect.top, m_searchRect.right, m_searchRect.bottom), m_searchBrush);
            
            // Draw cursor if focused
            if (m_searchFocused && m_cursorVisible) {
                ID2D1SolidColorBrush* cursorBrush = nullptr;
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF), &cursorBrush);
                
                // Calculate cursor position based on text width
                float cursorX = m_searchRect.left + 40;
                if (m_cursorPosition > 0) {
                    // Get text width up to cursor position
                    std::wstring textToCursor = m_searchText.substr(0, m_cursorPosition);
                    IDWriteTextLayout* layout = nullptr;
                    m_writeFactory->CreateTextLayout(textToCursor.c_str(), (UINT32)textToCursor.length(), m_searchFormat, 1000.0f, 1000.0f, &layout);
                    if (layout) {
                        DWRITE_TEXT_METRICS metrics;
                        layout->GetMetrics(&metrics);
                        cursorX += metrics.width;
                        layout->Release();
                    }
                }
                
                m_renderTarget->DrawLine(
                    D2D1::Point2F(cursorX, m_searchRect.top + 10),
                    D2D1::Point2F(cursorX, m_searchRect.bottom - 10),
                    cursorBrush, 1.0f
                );
                cursorBrush->Release();
            }
        } else {
            // Show placeholder when not focused and empty
            m_renderTarget->DrawText(L"Search...", 9, m_searchFormat, D2D1::RectF(m_searchRect.left + 40, m_searchRect.top, m_searchRect.right, m_searchRect.bottom), m_searchBrush);
        }
        
        // Close button
        DrawXIcon(m_closeButtonRect);
    }
    
    void DrawSearchIcon(RECT rect) {
        // Draw magnifying glass icon within the 20x20 rect bounds
        ID2D1PathGeometry* pathGeometry = nullptr;
        m_d2dFactory->CreatePathGeometry(&pathGeometry);
        
        ID2D1GeometrySink* sink = nullptr;
        pathGeometry->Open(&sink);
        
        // Draw within the 20x20 rect - use relative coordinates
        float left = (float)rect.left;
        float top = (float)rect.top;
        
        // Circle centered in the 20x20 area with 10px diameter
        float circleCenterX = left + 10.0f; // Center of 20px width
        float circleCenterY = top + 10.0f;  // Center of 20px height
        float circleRadius = 5.0f; // 10px diameter
        
        // Draw complete circle using two semicircles
        sink->BeginFigure(D2D1::Point2F(circleCenterX + circleRadius, circleCenterY), D2D1_FIGURE_BEGIN_HOLLOW);
        
        // First semicircle (top half)
        sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(circleCenterX - circleRadius, circleCenterY),
            D2D1::SizeF(circleRadius, circleRadius),
            0.0f,
            D2D1_SWEEP_DIRECTION_CLOCKWISE,
            D2D1_ARC_SIZE_SMALL
        ));
        
        // Second semicircle (bottom half)
        sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(circleCenterX + circleRadius, circleCenterY),
            D2D1::SizeF(circleRadius, circleRadius),
            0.0f,
            D2D1_SWEEP_DIRECTION_CLOCKWISE,
            D2D1_ARC_SIZE_SMALL
        ));
        
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        
        // Handle line from circle edge to bottom-right corner (shorter)
        float handleStartX = circleCenterX + circleRadius * 0.707f; // 45 degrees from circle
        float handleStartY = circleCenterY + circleRadius * 0.707f;
        float handleEndX = left + 16.4f; // Shorter handle
        float handleEndY = top + 16.4f; // Shorter handle
        
        sink->BeginFigure(D2D1::Point2F(handleStartX, handleStartY), D2D1_FIGURE_BEGIN_HOLLOW);
        sink->AddLine(D2D1::Point2F(handleEndX, handleEndY));
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        
        sink->Close();
        sink->Release();
        
        m_renderTarget->DrawGeometry(pathGeometry, m_textBrush, 2.0f);
        pathGeometry->Release();
    }

	void FillRightRoundedRect(RECT rect, float radius) {
		ID2D1PathGeometry* path = nullptr;
		if (FAILED(m_d2dFactory->CreatePathGeometry(&path))) return;
		ID2D1GeometrySink* sink = nullptr;
		if (FAILED(path->Open(&sink))) { if (path) path->Release(); return; }
		float l = (float)rect.left;
		float t = (float)rect.top;
		float r = (float)rect.right;
		float b = (float)rect.bottom;
		float rr = radius;
		sink->BeginFigure(D2D1::Point2F(l, t), D2D1_FIGURE_BEGIN_FILLED);
		sink->AddLine(D2D1::Point2F(r - rr, t));
		sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r, t + rr), D2D1::SizeF(rr, rr), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
		sink->AddLine(D2D1::Point2F(r, b - rr));
		sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(r - rr, b), D2D1::SizeF(rr, rr), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
		sink->AddLine(D2D1::Point2F(l, b));
		sink->EndFigure(D2D1_FIGURE_END_CLOSED);
		sink->Close();
		sink->Release();
		m_renderTarget->FillGeometry(path, m_sidebarBrush);
		path->Release();
	}
    
    void DrawUserIcon(RECT rect, ID2D1SolidColorBrush* brush) {
        const char* userIconSvg = R"(
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M19 21v-2a4 4 0 0 0-4-4H9a4 4 0 0 0-4 4v2"/>
    <circle cx="12" cy="7" r="4"/>
</svg>
)";
        
        char* mutableSvg = (char*)malloc(strlen(userIconSvg) + 1);
        strcpy(mutableSvg, userIconSvg);
        
        NSVGimage* image = nsvgParse(mutableSvg, "px", 96.0f);
        free(mutableSvg);
        
        if (!image) return;
        
        float rw = (float)(rect.right - rect.left);
        float rh = (float)(rect.bottom - rect.top);
        float scaleX = rw / image->width;
        float scaleY = rh / image->height;
        float iconScale = min(scaleX, scaleY) * 1.2f;
        
        float offsetX = rect.left + (rw - image->width * iconScale) * 0.5f;
        float offsetY = rect.top + (rh - image->height * iconScale) * 0.5f;
        
        ID2D1StrokeStyle* strokeStyle = nullptr;
        D2D1_STROKE_STYLE_PROPERTIES strokeProps = D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_LINE_JOIN_ROUND,
            10.0f,
            D2D1_DASH_STYLE_SOLID,
            0.0f
        );
        m_d2dFactory->CreateStrokeStyle(strokeProps, nullptr, 0, &strokeStyle);
        
        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
            if (!(shape->flags & NSVG_FLAGS_VISIBLE)) continue;
            
            for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
                ID2D1PathGeometry* geometry = nullptr;
                m_d2dFactory->CreatePathGeometry(&geometry);
                
                ID2D1GeometrySink* sink = nullptr;
                geometry->Open(&sink);
                sink->SetFillMode(shape->fillRule == NSVG_FILLRULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);
                
                bool hasStarted = false;
                for (int i = 0; i < path->npts - 1; i += 3) {
                    float* p = &path->pts[i * 2];
                    
                    D2D1_POINT_2F p1 = D2D1::Point2F(offsetX + p[0] * iconScale, offsetY + p[1] * iconScale);
                    D2D1_POINT_2F p2 = D2D1::Point2F(offsetX + p[2] * iconScale, offsetY + p[3] * iconScale);
                    D2D1_POINT_2F p3 = D2D1::Point2F(offsetX + p[4] * iconScale, offsetY + p[5] * iconScale);
                    D2D1_POINT_2F p4 = D2D1::Point2F(offsetX + p[6] * iconScale, offsetY + p[7] * iconScale);
                    
                    if (!hasStarted) {
                        sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_HOLLOW);
                        hasStarted = true;
                    }
                    
                    sink->AddBezier(D2D1::BezierSegment(p2, p3, p4));
                }
                
                if (hasStarted) {
                    sink->EndFigure(path->closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
                }
                
                sink->Close();
                sink->Release();
                
                if (shape->stroke.type != NSVG_PAINT_NONE && shape->strokeWidth > 0.0f) {
                    m_renderTarget->DrawGeometry(geometry, brush, shape->strokeWidth * iconScale, strokeStyle);
                }
                geometry->Release();
            }
        }
        
        if (strokeStyle) strokeStyle->Release();
        nsvgDelete(image);
    }

    void DrawSettingsIcon(RECT rect) {
        ID2D1PathGeometry* pathGeometry = nullptr;
        HRESULT hr = m_d2dFactory->CreatePathGeometry(&pathGeometry);
        if (FAILED(hr)) return;
        
        ID2D1GeometrySink* sink = nullptr;
        hr = pathGeometry->Open(&sink);
        if (FAILED(hr)) {
            pathGeometry->Release();
            return;
        }
        
        float centerX = (rect.left + rect.right) / 2.0f - 10.8f;
        float centerY = (rect.top + rect.bottom) / 2.0f + 12.0f;
        float scale = 0.82f;
        
        sink->BeginFigure(D2D1::Point2F(centerX + 12 * scale, centerY + 3.5 * scale), D2D1_FIGURE_BEGIN_FILLED);
        
        sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(centerX + 8.5 * scale, centerY),
            D2D1::SizeF(3.5 * scale, 3.5 * scale),
            0.0f,
            D2D1_SWEEP_DIRECTION_CLOCKWISE,
            D2D1_ARC_SIZE_SMALL
        ));
        
        sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(centerX + 12 * scale, centerY - 3.5 * scale),
            D2D1::SizeF(3.5 * scale, 3.5 * scale),
            0.0f,
            D2D1_SWEEP_DIRECTION_CLOCKWISE,
            D2D1_ARC_SIZE_SMALL
        ));
        
        sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(centerX + 15.5 * scale, centerY),
            D2D1::SizeF(3.5 * scale, 3.5 * scale),
            0.0f,
            D2D1_SWEEP_DIRECTION_CLOCKWISE,
            D2D1_ARC_SIZE_SMALL
        ));
            
            sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(centerX + 12 * scale, centerY + 3.5 * scale),
            D2D1::SizeF(3.5 * scale, 3.5 * scale),
                0.0f,
                D2D1_SWEEP_DIRECTION_CLOCKWISE,
                D2D1_ARC_SIZE_SMALL
            ));
        
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        
        sink->BeginFigure(D2D1::Point2F(centerX + 19.43 * scale, centerY + 0.97 * scale), D2D1_FIGURE_BEGIN_FILLED);
        
        sink->AddLine(D2D1::Point2F(centerX + 21.54 * scale, centerY - 2.63 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 21.78 * scale, centerY - 3.05 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 21.66 * scale, centerY - 3.27 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 19.66 * scale, centerY - 6.73 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 19.54 * scale, centerY - 6.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 19.27 * scale, centerY - 7.04 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 19.05 * scale, centerY - 6.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 16.56 * scale, centerY - 5.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 16.04 * scale, centerY - 6.34 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 15.5 * scale, centerY - 6.68 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.87 * scale, centerY - 6.93 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.5 * scale, centerY - 9.58 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.46 * scale, centerY - 9.82 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.25 * scale, centerY - 10.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.0 * scale, centerY - 10.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 10.0 * scale, centerY - 10.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 9.75 * scale, centerY - 10.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 9.54 * scale, centerY - 9.82 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 9.5 * scale, centerY - 9.58 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 9.13 * scale, centerY - 6.93 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 8.5 * scale, centerY - 6.68 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 7.96 * scale, centerY - 6.34 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 7.44 * scale, centerY - 5.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.95 * scale, centerY - 6.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.73 * scale, centerY - 7.04 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.46 * scale, centerY - 6.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.34 * scale, centerY - 6.73 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 2.34 * scale, centerY - 3.27 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 2.21 * scale, centerY - 3.05 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 2.27 * scale, centerY - 2.78 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 2.46 * scale, centerY - 2.63 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.57 * scale, centerY - 1.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.53 * scale, centerY - 0.66 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.5 * scale, centerY - 0.33 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.5 * scale, centerY + 0.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.5 * scale, centerY + 0.33 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.53 * scale, centerY + 0.66 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.57 * scale, centerY + 0.97 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 2.46 * scale, centerY + 2.63 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 2.27 * scale, centerY + 2.78 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 2.21 * scale, centerY + 3.05 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 2.34 * scale, centerY + 3.27 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.34 * scale, centerY + 6.73 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.46 * scale, centerY + 6.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.73 * scale, centerY + 7.03 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 4.95 * scale, centerY + 6.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 7.44 * scale, centerY + 5.94 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 7.96 * scale, centerY + 6.34 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 8.5 * scale, centerY + 6.68 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 9.13 * scale, centerY + 6.93 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 9.5 * scale, centerY + 9.58 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 9.54 * scale, centerY + 9.82 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 9.75 * scale, centerY + 10.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 10.0 * scale, centerY + 10.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.0 * scale, centerY + 10.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.25 * scale, centerY + 10.0 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.46 * scale, centerY + 9.82 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.5 * scale, centerY + 9.58 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 14.87 * scale, centerY + 6.93 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 15.5 * scale, centerY + 6.67 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 16.04 * scale, centerY + 6.34 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 16.56 * scale, centerY + 5.94 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 19.05 * scale, centerY + 6.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 19.27 * scale, centerY + 7.03 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 19.54 * scale, centerY + 6.95 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 19.66 * scale, centerY + 6.73 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 21.66 * scale, centerY + 3.27 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 21.78 * scale, centerY + 3.05 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 21.73 * scale, centerY + 2.78 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 21.54 * scale, centerY + 2.63 * scale));
        sink->AddLine(D2D1::Point2F(centerX + 19.43 * scale, centerY + 0.97 * scale));
        
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        
        sink->Close();
        sink->Release();
        
        m_renderTarget->DrawGeometry(pathGeometry, m_searchBrush, 2.0f);
        pathGeometry->Release();
    }
    
    void DrawXIcon(RECT rect, float size, ID2D1SolidColorBrush* brush) {
        ID2D1PathGeometry* pathGeometry = nullptr;
        HRESULT hr = m_d2dFactory->CreatePathGeometry(&pathGeometry);
        if (FAILED(hr)) return;
        
        ID2D1GeometrySink* sink = nullptr;
        hr = pathGeometry->Open(&sink);
        if (FAILED(hr)) {
            pathGeometry->Release();
            return;
        }
        
        // Calculate center and size for X icon
        float centerX = (rect.left + rect.right) / 2.0f;
        float centerY = (rect.top + rect.bottom) / 2.0f;
        float halfSize = size / 2.0f;
        
        // First diagonal line (top-left to bottom-right)
        sink->BeginFigure(D2D1::Point2F(centerX - halfSize, centerY - halfSize), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine(D2D1::Point2F(centerX + halfSize, centerY + halfSize));
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        
        // Second diagonal line (top-right to bottom-left)
        sink->BeginFigure(D2D1::Point2F(centerX + halfSize, centerY - halfSize), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine(D2D1::Point2F(centerX - halfSize, centerY + halfSize));
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        
        sink->Close();
        sink->Release();
        
        ID2D1SolidColorBrush* b = brush ? brush : m_searchBrush;
        m_renderTarget->DrawGeometry(pathGeometry, b, 2.0f);
        
        pathGeometry->Release();
    }

    void DrawXIcon(RECT rect, float size) {
        DrawXIcon(rect, size, m_searchBrush);
    }

    void DrawXIcon(RECT rect) {
        DrawXIcon(rect, 12.0f, m_searchBrush);
    }
    
    void DrawParryBarIcon(RECT rect) {
        DrawParryBarIconEnabled(rect, false);
    }
    
    void DrawParryBarIconEnabled(RECT rect, bool enabled) {
        DrawParryBarIconAnimated(rect, enabled, enabled ? 1.0f : 0.0f);
    }
    void DrawKeyboardIconAnimated(RECT rect, bool enabled, float animationProgress) {
        // Redesigned for a clean, minimalist look in a small space
        float rw = (float)(rect.right - rect.left);
        float rh = (float)(rect.bottom - rect.top);

        // Use a 32x32 virtual canvas for easy positioning
        float vbW = 32.0f;
        float vbH = 32.0f;
        float s = min(rw / vbW, rh / vbH) * 0.9f; // Use 90% of the icon box
        float ox = rect.left + (rw - vbW * s) * 0.5f;
        float oy = rect.top + (rh - vbH * s) * 0.5f;

        auto RR = [&](float x, float y, float w, float h, float r) {
            D2D1_POINT_2F p1 = { ox + x * s, oy + y * s };
            D2D1_POINT_2F p2 = { ox + (x + w) * s, oy + (y + h) * s };
            return D2D1::RoundedRect(D2D1::RectF(p1.x, p1.y, p2.x, p2.y), r * s, r * s);
        };

        float grayWeight = 1.0f - animationProgress;
        float darkWeight = animationProgress;
        float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        ID2D1SolidColorBrush* brush = nullptr;
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f), &brush);

        if (brush) {
            float keySize = 9.0f;
            float gap = 2.0f;
            float cornerRadius = 2.0f;
            float strokeWidth = 1.0f;

            // Centered layout calculations
            float totalW = keySize * 3 + gap * 2;
            float totalH = keySize * 2 + gap;
            float startX = (vbW - totalW) / 2.0f;
            float startY = (vbH - totalH) / 2.0f;

            // W key
            m_renderTarget->DrawRoundedRectangle(RR(startX + keySize + gap, startY, keySize, keySize, cornerRadius), brush, strokeWidth);
            // A key
            m_renderTarget->DrawRoundedRectangle(RR(startX, startY + keySize + gap, keySize, keySize, cornerRadius), brush, strokeWidth);
            // S key
            m_renderTarget->DrawRoundedRectangle(RR(startX + keySize + gap, startY + keySize + gap, keySize, keySize, cornerRadius), brush, strokeWidth);
            // D key
            m_renderTarget->DrawRoundedRectangle(RR(startX + (keySize + gap) * 2, startY + keySize + gap, keySize, keySize, cornerRadius), brush, strokeWidth);
            
            // Draw text inside the keys
            float fontPx = keySize * 0.7f * s; // Small, scalable font size
            IDWriteTextFormat* fmt = nullptr;
            m_writeFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fontPx, L"en-us", &fmt);

            if (fmt) {
                fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

                auto TR = [&](float x, float y, float w, float h) {
                    D2D1_POINT_2F p1 = { ox + x * s, oy + y * s };
                    D2D1_POINT_2F p2 = { ox + (x + w) * s, oy + (y + h) * s };
                    return D2D1::RectF(p1.x, p1.y, p2.x, p2.y);
                };

                m_renderTarget->DrawText(L"W", 1, fmt, TR(startX + keySize + gap, startY, keySize, keySize), brush);
                m_renderTarget->DrawText(L"A", 1, fmt, TR(startX, startY + keySize + gap, keySize, keySize), brush);
                m_renderTarget->DrawText(L"S", 1, fmt, TR(startX + keySize + gap, startY + keySize + gap, keySize, keySize), brush);
                m_renderTarget->DrawText(L"D", 1, fmt, TR(startX + (keySize + gap) * 2, startY + keySize + gap, keySize, keySize), brush);

                fmt->Release();
            }

            brush->Release();
        }
    }
    
    void DrawPIconAnimated(RECT rect, bool enabled, float animationProgress) {
        float rw = (float)(rect.right - rect.left);
        float rh = (float)(rect.bottom - rect.top);
        float vb = 1200.0f;
        float s = (min(rw, rh) / vb) * 0.8f;
        float ox = rect.left + (rw - vb * s) * 0.5f;
        float oy = rect.top + (rh - vb * s) * 0.5f;
        auto TP = [&](float x, float y) { return D2D1::Point2F(ox + x * s, oy + (1200.0f - y) * s); };
        auto createPathFromSvg = [&](const char* d, ID2D1PathGeometry** outGeom) -> bool {
            *outGeom = nullptr;
            ID2D1PathGeometry* g = nullptr;
            HRESULT hrLocal = m_d2dFactory->CreatePathGeometry(&g);
            if (FAILED(hrLocal) || !g) return false;
            ID2D1GeometrySink* sinkLocal = nullptr;
            hrLocal = g->Open(&sinkLocal);
            if (FAILED(hrLocal) || !sinkLocal) { if (g) g->Release(); return false; }
            sinkLocal->SetFillMode(D2D1_FILL_MODE_WINDING);
            *outGeom = g;
            auto parseTo = [&](ID2D1GeometrySink* targetSink, const char* pathD, auto TPfn) {
                const char* p = pathD;
                auto skip = [&]() { while (*p && (*p==' '||*p=='\t'||*p=='\n'||*p==',')) ++p; };
                auto isnum = [&](char c){ return (c=='-'||c=='+'||c=='.'||(c>='0'&&c<='9')); };
                auto readf = [&]() -> float { skip(); char* e=nullptr; float v=(float)strtod(p,&e); p=e; return v; };
                float cx=0, cy=0, sx=0, sy=0; char cmd=0;
                while (*p) {
                    skip(); if (!*p) break; char c=*p;
                    if ((c>='A'&&c<='Z')||(c>='a'&&c<='z')) { cmd=c; ++p; }
                    else if (!cmd) { ++p; continue; }
                    switch(cmd) {
                        case 'M': {
                            float x=readf(), y=readf(); cx=sx=x; cy=sy=y;
                            targetSink->BeginFigure(TPfn(cx,cy), D2D1_FIGURE_BEGIN_FILLED);
                            while (isnum(*p)) { x=readf(); y=readf(); cx=x; cy=y; targetSink->AddLine(TPfn(cx,cy)); }
                            break;
                        }
                        case 'm': {
                            float dx=readf(), dy=readf(); cx+=dx; cy+=dy; sx=cx; sy=cy;
                            targetSink->BeginFigure(TPfn(cx,cy), D2D1_FIGURE_BEGIN_FILLED);
                            while (isnum(*p)) { dx=readf(); dy=readf(); cx+=dx; cy+=dy; targetSink->AddLine(TPfn(cx,cy)); }
                            break;
                        }
                        case 'L': { while (isnum(*p)) { float x=readf(), y=readf(); cx=x; cy=y; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'l': { while (isnum(*p)) { float dx=readf(), dy=readf(); cx+=dx; cy+=dy; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'H': { while (isnum(*p)) { float x=readf(); cx=x; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'h': { while (isnum(*p)) { float dx=readf(); cx+=dx; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'V': { while (isnum(*p)) { float y=readf(); cy=y; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'v': { while (isnum(*p)) { float dy=readf(); cy+=dy; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'C': { while (isnum(*p)) { float x1=readf(), y1=readf(), x2=readf(), y2=readf(), x=readf(), y=readf(); D2D1_BEZIER_SEGMENT b={TPfn(x1,y1),TPfn(x2,y2),TPfn(x,y)}; targetSink->AddBezier(b); cx=x; cy=y; } break; }
                        case 'c': { while (isnum(*p)) { float dx1=readf(), dy1=readf(), dx2=readf(), dy2=readf(), dx=readf(), dy=readf(); D2D1_BEZIER_SEGMENT b={TPfn(cx+dx1,cy+dy1),TPfn(cx+dx2,cy+dy2),TPfn(cx+dx,cy+dy)}; targetSink->AddBezier(b); cx+=dx; cy+=dy; } break; }
                        case 'Z': case 'z': { targetSink->EndFigure(D2D1_FIGURE_END_CLOSED); cx=sx; cy=sy; cmd=0; break; }
                        default: { ++p; break; }
                    }
                }
            };
            parseTo(sinkLocal, d, TP);
            sinkLocal->Close();
            sinkLocal->Release();
            return true;
        };

        ID2D1PathGeometry* pathGeometry = nullptr;
        const char* pPath = R"(M678 1975 c-21 -11 -26 -24 -33 -86 12 -23 208 -231 227 -241 2 0 129 -4 283 -7 l280 -6 59 -34 c30 -18 40 -35 94 -148 7 -61 -28 -127 -82 -155 -52 -27 -183 -36 -516 -37 -277 -1 -310 -5 -343 -35 -20 -18 -25 -36 -30 -102 -10 -129 7 -393 27 -433 10 -18 62 -78 117 -132 86 -86 103 -99 133 -99 18 0 38 6 44 13 6 7 12 92 14 208 2 148 6 200 17 212 11 14 36 17 150 18 75 0 201 4 281 8 139 8 148 10 222 45 98 47 196 143 253 247 l40 74 0 155 0 155 -40 75 c-37 72 -51 90 -122 165 -43 44 -121 88 -218 121 l-80 28 -375 3 c-320 2 -379 1 -402 -12z)";

        if (!createPathFromSvg(pPath, &pathGeometry)) return;

        float finalRed = 0.0f;
        float finalGreen = 0.0f;
        float finalBlue = 0.0f;

        if (enabled) {
            finalRed = 0.0f;
            finalGreen = 0.0f;
            finalBlue = 0.0f;
        } else {
            finalRed = 0.5f;
            finalGreen = 0.5f;
            finalBlue = 0.5f;
        }

        ID2D1SolidColorBrush* iconBrush = nullptr;
        D2D1_COLOR_F iconColor = D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f);
        m_renderTarget->CreateSolidColorBrush(iconColor, &iconBrush);
        m_renderTarget->FillGeometry(pathGeometry, iconBrush);
        iconBrush->Release();
        pathGeometry->Release();
    }

    void DrawSvgWithNanoSVG(const char* svgContent, RECT rect, float animationProgress) {
        char* mutableSvg = (char*)malloc(strlen(svgContent) + 1);
        strcpy(mutableSvg, svgContent);
        
        NSVGimage* image = nsvgParse(mutableSvg, "px", 96.0f);
        free(mutableSvg);
        
        if (!image) return;
        
        float rw = (float)(rect.right - rect.left);
        float rh = (float)(rect.bottom - rect.top);
        float scaleX = rw / image->width;
        float scaleY = rh / image->height;
        float scale = min(scaleX, scaleY) * 1.2f;
        
        float offsetX = rect.left + (rw - image->width * scale) * 0.5f;
        float offsetY = rect.top + (rh - image->height * scale) * 0.5f;

        float grayWeight = 1.0f - animationProgress;
        float darkWeight = animationProgress;
        float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        
        ID2D1SolidColorBrush* brush = nullptr;
        D2D1_COLOR_F color = D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f);
        m_renderTarget->CreateSolidColorBrush(color, &brush);
        
        ID2D1StrokeStyle* strokeStyle = nullptr;
        D2D1_STROKE_STYLE_PROPERTIES strokeProps = D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_CAP_STYLE_ROUND,
            D2D1_LINE_JOIN_ROUND,
            10.0f,
            D2D1_DASH_STYLE_SOLID,
            0.0f
        );
        m_d2dFactory->CreateStrokeStyle(strokeProps, nullptr, 0, &strokeStyle);
        
        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
            if (!(shape->flags & NSVG_FLAGS_VISIBLE)) continue;
            brush->SetOpacity(shape->opacity);
            
            for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
                ID2D1PathGeometry* geometry = nullptr;
                m_d2dFactory->CreatePathGeometry(&geometry);
                
                ID2D1GeometrySink* sink = nullptr;
                geometry->Open(&sink);
                sink->SetFillMode(shape->fillRule == NSVG_FILLRULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);
                
                bool hasStarted = false;
                for (int i = 0; i < path->npts - 1; i += 3) {
                    float* p = &path->pts[i * 2];
                    
                    D2D1_POINT_2F p1 = D2D1::Point2F(offsetX + p[0] * scale, offsetY + p[1] * scale);
                    D2D1_POINT_2F p2 = D2D1::Point2F(offsetX + p[2] * scale, offsetY + p[3] * scale);
                    D2D1_POINT_2F p3 = D2D1::Point2F(offsetX + p[4] * scale, offsetY + p[5] * scale);
                    D2D1_POINT_2F p4 = D2D1::Point2F(offsetX + p[6] * scale, offsetY + p[7] * scale);
                    
                    if (!hasStarted) {
                        sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
                        hasStarted = true;
                    }
                    
                    sink->AddBezier(D2D1::BezierSegment(p2, p3, p4));
                }
                
                if (hasStarted) {
                    sink->EndFigure(path->closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
                }
                
                sink->Close();
                sink->Release();
                
                if (shape->fill.type != NSVG_PAINT_NONE) {
                m_renderTarget->FillGeometry(geometry, brush);
                }
                if (strokeStyle && shape->stroke.type != NSVG_PAINT_NONE && shape->strokeWidth > 0.0f) {
                    m_renderTarget->DrawGeometry(geometry, brush, shape->strokeWidth * scale, strokeStyle);
                }
                geometry->Release();
            }
        }
        brush->SetOpacity(1.0f);
        
        if (strokeStyle) strokeStyle->Release();
        brush->Release();
        nsvgDelete(image);
    }

    void DrawRollM1IconAnimated(RECT rect, bool enabled, float animationProgress) {
        const char* svgContent = 
            "<svg xmlns='http://www.w3.org/2000/svg' width='1011' height='923'>"
            "<path d='M542.409 506.91c1.086.863 5.136 2.479 9 3.59 15.31 4.408 26.622 8.71 39.525 15.034 15.762 7.726 24.29 13.734 29.949 21.1 2.237 2.913 4.424 5.746 4.86 6.296 2.854 3.603 4.415 14.342 3.032 20.86-1.14 5.374-4.773 13.377-8.008 17.64-1.044 1.375-5.371 6.006-9.615 10.29-32.704 33.018-121.461 66.964-180.218 68.926l-8.5.284-.806-11.473c-.443-6.31-.954-11.621-1.136-11.802-.31-.31-9.752 5.593-24.558 15.352-5.954 3.924-23.936 15.066-37.516 23.244-3.033 1.826-5.235 3.773-4.893 4.326.576.933 14.644 7.927 53.409 26.555 8.525 4.096 15.937 7.527 16.47 7.623.6.108.849-4.638.652-12.401l-.32-12.575 12.849-1.233c26.482-2.54 58.878-8.805 80.764-15.62 45.214-14.077 78.095-28.874 102.115-45.952 27.019-19.212 40.043-34.676 45.5-54.024 2.445-8.662 2.484-14.21.162-22.582-4.103-14.8-17.102-28.146-36.192-37.162-7.735-3.653-28.09-10.321-37-12.12-9.347-1.888-29.46-4.351-42-5.144-8.745-.553-9.343-.476-7.525.968M290.473 502.275c-1.085.701-5.348 2.831-9.473 4.735-12.045 5.557-20.904 11.746-30.058 20.995-6.707 6.776-9.459 10.47-12.719 17.071-5.525 11.189-6.641 16.925-5.881 30.225 1.332 23.32 11.909 42.665 37.233 68.098 5.489 5.513 8.405 8.123 20.296 18.166 6.736 5.69 15.608 12.133 19.129 13.894 2.954 1.476 2.251.454-4.5-6.551-10.888-11.297-14.55-15.169-17.575-18.588-5.786-6.538-15.456-19.2-19.701-25.797A2978.057 2978.057 0 0 0 260.423 614c-11.208-17.193-16.961-42.642-13.063-57.79 1.73-6.72 6.164-16.857 8.326-19.031.723-.727 1.315-1.627 1.315-2 .003-1.5 12.068-14.422 17.928-19.202 1.964-1.602 7.353-5.627 11.975-8.945 4.623-3.317 7.76-6.032 6.973-6.032-.788 0-2.32.574-3.404 1.275'/>"
            "<path d='M456.755 499.088c-5.123 5.119-13.505 5.119-18.628.001l-12.888-12.874c-5.122-5.117-5.122-13.492 0-18.61l220-219.768c5.123-5.117 15.216-9.84 22.426-10.496l53.924-4.903c7.212-.655 12.574 4.707 11.92 11.92l-4.896 53.82c-.655 7.211-5.383 17.297-10.506 22.416L498.094 540.389c-5.123 5.117-13.505 5.117-18.628 0l-12.905-12.892c-5.123-5.117-5.123-13.492 0-18.61l184.202-184.032a6.924 6.924 0 0 0 .003-9.804c-2.703-2.71-7.1-2.71-9.814-.002zm-108.8 80.141c-10.395 0-20.164 4.044-27.519 11.387-7.342 7.336-11.384 17.095-11.384 27.47 0 10.382 4.044 20.131 11.384 27.466 7.354 7.338 17.124 11.386 27.52 11.388 10.389 0 20.159-4.052 27.507-11.388 7.348-7.336 11.396-17.089 11.396-27.466 0-10.376-4.05-20.135-11.396-27.47-7.347-7.343-17.118-11.387-27.508-11.387zm54.625 9.47a62.788 62.788 0 0 0-10.771-14.438c-4.544-4.537-9.693-8.24-15.214-11.161-6.597-3.492-1.382-10.972-1.382-10.972 6.752-10.273 12.91-20.595 18-29.587l-34.176-34.149h-33.18c-6.38 0-11.553-5.177-11.553-11.56 0-6.379 5.173-11.556 11.554-11.556h37.956c3.072 0 6.008 1.217 8.175 3.382l124.897 124.768a11.566 11.566 0 0 1 3.386 8.176v38.141c0 6.386-5.174 11.552-11.556 11.552-6.389 0-11.563-5.165-11.563-11.552v-33.35l-34.04-33.997c-9.189 5.423-19.869 12.072-30.451 19.383-.001-.002-6.064 4.313-10.082-3.08z' style='stroke-width:1.46285'/>"
            "</svg>";
        
        DrawSvgWithNanoSVG(svgContent, rect, animationProgress);
    }
    
    void DrawGammaIconAnimated(RECT rect, bool enabled, float animationProgress) {
        const char* svgContent = 
            "<svg version='1.0' xmlns='http://www.w3.org/2000/svg' width='600.000000pt' height='600.000000pt' viewBox='0 0 600.000000 600.000000' preserveAspectRatio='xMidYMid meet'>"
            "<g transform='translate(300,300) scale(0.7) translate(-300,-300)'>"
            "<g transform='translate(0.000000,600.000000) scale(0.100000,-0.100000)' fill='#000000' stroke='none'>"
            "<path d='M2904 5332 c-58 -37 -64 -65 -64 -309 0 -248 6 -273 74 -307 50 -24 99 -15 143 29 l33 33 0 246 0 246 -33 36 c-42 46 -104 56 -153 26z m119 -54 c15 -15 27 -33 27 -40 3 -250 0 -430 -9 -447 -31 -58 -117 -55 -149 5 -14 27 -17 418 -4 454 12 30 29 46 57 52 36 7 50 3 78 -24z'/>"
            "<path d='M3914 5081 c-12 -5 -37 -29 -56 -53 -72 -88 -231 -323 -239 -352 -26 -90 63 -180 155 -155 45 12 57 27 201 234 100 144 115 169 115 196 0 43 -24 95 -54 114 -30 20 -93 28 -122 16z m92 -47 c13 -3 24 -12 24 -20 0 -8 4 -14 9 -14 12 0 5 -69 -11 -99 -7 -14 -29 -46 -49 -71 -19 -25 -44 -60 -55 -77 -10 -18 -23 -33 -28 -33 -5 0 -7 -3 -3 -6 3 -4 1 -12 -6 -19 -7 -7 -19 -23 -27 -35 -25 -38 -72 -90 -89 -99 -24 -12 -79 6 -95 32 -19 29 -27 75 -15 83 6 3 8 13 5 21 -3 8 -2 11 3 8 5 -3 12 3 15 12 3 10 17 30 31 46 14 15 23 27 20 27 -3 0 2 8 11 18 19 21 118 157 142 194 18 29 57 49 80 42 8 -2 25 -7 38 -10z'/>"
            "<path d='M1861 4972 c-40 -21 -71 -73 -71 -118 0 -17 12 -48 26 -69 60 -88 266 -333 289 -343 87 -40 181 26 178 125 -1 42 -10 55 -157 237 -153 190 -186 210 -265 168z m108 -44 c11 -7 21 -16 21 -19 0 -3 11 -18 24 -32 14 -15 37 -44 53 -64 15 -21 46 -57 68 -81 22 -23 39 -44 38 -47 -2 -2 9 -16 23 -30 33 -33 53 -82 45 -110 -10 -37 -43 -74 -59 -67 -11 4 -13 1 -7 -8 6 -9 4 -11 -6 -5 -7 5 -21 10 -30 10 -17 1 -76 43 -72 51 3 4 -27 42 -59 76 -10 10 -16 18 -14 18 6 0 -32 45 -74 88 -19 20 -28 32 -20 28 8 -4 2 4 -13 18 -15 14 -25 26 -21 26 4 0 0 8 -8 18 -38 44 -26 111 24 135 34 17 55 16 87 -5z'/>"
            "<path d='M2870 4563 c-539 -53 -988 -407 -1133 -893 -122 -405 -32 -826 244 -1150 86 -102 122 -155 181 -275 81 -162 177 -446 194 -574 l7 -51 623 0 624 0 0 88 c0 183 75 385 229 613 38 57 92 131 120 164 209 251 304 482 318 770 14 317 -73 583 -272 832 -262 327 -716 518 -1135 476z m305 -48 c76 -10 307 -78 332 -97 7 -5 18 -6 24 -2 7 4 9 3 6 -3 -4 -5 12 -16 33 -23 22 -7 40 -17 40 -22 0 -4 4 -8 10 -8 5 0 18 -7 27 -15 10 -8 23 -16 28 -16 6 0 30 -17 55 -37 24 -20 40 -30 35 -22 -5 8 5 2 21 -15 17 -16 57 -56 90 -87 32 -32 64 -63 69 -70 66 -81 115 -148 115 -157 0 -6 5 -11 10 -11 6 0 10 -6 10 -14 0 -8 4 -16 9 -18 11 -4 91 -188 91 -210 0 -9 5 -20 10 -23 6 -4 8 -11 4 -16 -3 -5 -1 -15 4 -22 6 -7 18 -59 27 -116 21 -124 16 -350 -10 -446 -8 -33 -17 -67 -20 -75 -2 -8 -4 -15 -5 -15 -1 0 -3 -11 -6 -25 -6 -31 -85 -202 -99 -212 -5 -4 -7 -8 -2 -8 8 0 -44 -83 -55 -88 -5 -2 -8 -8 -8 -13 0 -10 -26 -42 -77 -95 -18 -19 -33 -39 -33 -45 0 -5 -12 -22 -27 -36 -16 -15 -22 -23 -15 -19 13 8 -35 -61 -78 -112 -11 -13 -17 -29 -13 -34 3 -6 1 -8 -4 -5 -16 10 -150 -268 -173 -359 -11 -43 -23 -114 -27 -157 -3 -47 -12 -84 -20 -93 -12 -12 -103 -14 -561 -14 -301 0 -557 3 -569 6 -20 5 -27 21 -39 79 -2 11 -20 72 -40 135 -19 63 -36 120 -38 125 -1 6 -8 20 -14 33 -7 12 -11 22 -8 22 12 0 -104 250 -129 280 -8 8 -13 17 -12 20 2 9 -67 110 -75 110 -5 0 -8 4 -8 10 0 5 -12 22 -27 38 -39 41 -58 65 -60 76 -1 5 -5 12 -10 15 -15 11 -63 79 -63 89 0 6 -3 12 -7 14 -8 3 -70 118 -70 128 0 16 -16 50 -23 50 -5 0 -9 8 -8 18 0 9 -9 42 -20 74 -11 31 -18 59 -15 62 4 3 2 6 -3 6 -5 0 -9 10 -10 23 0 12 -5 49 -9 82 -16 114 -10 379 9 367 5 -3 7 2 3 10 -3 9 4 47 16 85 11 37 20 74 18 81 -1 6 2 12 7 12 6 0 12 10 16 23 3 13 18 50 33 83 14 32 27 61 27 64 1 3 19 32 41 65 42 64 41 63 80 111 13 17 21 35 17 39 -4 5 -2 5 5 1 8 -4 18 5 28 24 8 16 18 30 21 30 3 0 2 -6 -2 -12 -4 -7 23 17 61 55 39 37 73 67 77 67 3 0 13 6 20 13 46 45 266 167 301 167 8 0 17 3 21 8 4 4 11 7 14 7 4 1 36 11 72 23 36 12 84 23 108 25 23 1 42 5 42 10 0 4 10 5 23 2 12 -2 38 0 57 5 31 8 247 5 325 -5z'/>"
            "<path d='M2823 4403 c-29 -2 -53 -7 -53 -11 0 -4 -15 -8 -33 -9 -36 -1 -203 -57 -223 -74 -8 -6 -14 -9 -14 -5 0 12 -154 -77 -219 -127 -61 -46 -208 -187 -196 -187 3 0 -6 -12 -20 -27 -14 -16 -28 -36 -31 -45 -3 -10 -9 -18 -14 -18 -5 0 -17 -17 -27 -37 -9 -21 -22 -41 -27 -45 -6 -4 -7 -8 -2 -8 5 0 2 -8 -6 -17 -25 -30 -78 -179 -97 -276 -24 -120 -21 -366 6 -437 4 -8 8 -25 10 -37 4 -25 47 -141 71 -193 18 -39 98 -167 116 -185 35 -38 76 -85 71 -85 -2 0 9 -13 27 -30 17 -16 27 -30 22 -30 -5 0 -1 -5 9 -11 9 -5 17 -15 17 -20 0 -6 10 -25 22 -42 13 -18 23 -34 23 -37 1 -3 12 -23 26 -45 15 -22 30 -51 34 -65 4 -14 20 -52 35 -85 35 -76 70 -176 104 -290 15 -49 30 -97 35 -105 20 -38 44 -40 486 -39 502 0 464 -8 496 109 11 41 22 82 23 90 1 9 5 21 10 28 4 6 9 20 11 29 1 10 14 44 29 76 14 31 26 61 26 65 1 4 7 14 14 22 8 8 19 32 27 52 7 21 23 50 36 65 13 15 21 33 18 40 -2 8 -1 12 3 9 7 -4 107 134 120 165 2 4 24 31 50 59 26 29 47 55 47 58 0 4 13 21 29 37 16 17 37 48 47 70 10 22 19 36 19 30 0 -5 12 15 27 45 14 30 25 61 24 68 -1 6 2 12 8 12 14 0 29 46 61 190 21 95 29 290 15 363 -29 153 -60 250 -101 317 -11 19 -24 45 -28 56 -4 12 -18 34 -31 48 -13 14 -22 26 -19 26 7 0 -28 52 -52 76 -11 12 -29 32 -40 45 -22 27 -152 142 -204 180 -19 14 -82 49 -140 76 -96 47 -160 69 -295 102 -52 13 -291 22 -382 14z m343 -48 c495 -85 873 -476 912 -944 24 -289 -53 -524 -255 -777 -217 -273 -346 -520 -394 -752 l-13 -62 -446 0 -446 0 -18 73 c-28 110 -109 324 -165 437 -66 131 -109 197 -191 295 -117 140 -177 246 -223 390 -168 533 137 1107 688 1294 177 60 374 76 551 46z'/>"
            "<path d='M4620 4433 c-26 -10 -354 -240 -374 -262 -12 -13 -21 -41 -24 -74 -5 -59 16 -95 71 -123 50 -26 96 -13 189 53 46 33 126 89 178 125 103 71 120 94 120 162 0 81 -88 146 -160 119z m92 -60 c8 -10 19 -31 26 -46 10 -23 9 -32 -5 -55 -25 -39 -46 -56 -163 -135 -58 -39 -108 -75 -112 -81 -4 -6 -8 -7 -8 -3 0 4 -9 -1 -21 -12 -51 -47 -110 -51 -149 -9 -31 34 -26 82 13 121 42 42 103 87 117 87 6 0 13 7 16 15 4 8 12 15 19 15 7 0 15 7 19 15 3 8 12 15 19 15 8 0 20 7 27 15 7 9 15 14 17 11 2 -2 16 7 31 21 34 32 58 42 103 42 23 1 41 -5 51 -16z'/>"
            "<path d='M1189 4257 c-64 -43 -72 -143 -16 -195 37 -35 391 -192 432 -192 75 0 125 51 125 128 0 75 -25 95 -239 193 -213 98 -245 105 -302 66z m151 -42 c27 -14 56 -25 65 -25 8 0 15 -5 15 -12 0 -6 3 -9 6 -5 3 3 36 -9 73 -28 36 -18 87 -43 113 -54 58 -28 70 -42 74 -90 3 -42 -17 -81 -42 -81 -8 0 -14 -4 -14 -10 0 -5 -6 -7 -12 -5 -7 2 -25 6 -39 9 -25 4 -93 34 -287 128 -78 38 -120 77 -116 106 1 6 2 19 3 27 0 9 12 27 26 40 31 32 72 32 135 0z'/>"
            "<path d='M983 3435 c-89 -39 -98 -186 -13 -230 23 -12 73 -15 259 -15 l229 0 31 25 c77 66 60 175 -36 224 -34 17 -429 14 -470 -4z m448 -35 c46 -13 61 -30 62 -74 1 -38 -21 -89 -35 -80 -4 3 -8 0 -8 -5 0 -8 -73 -11 -225 -11 l-226 0 -24 25 c-50 49 -23 136 45 148 51 9 376 7 411 -3z'/>"
            "<path d='M4484 3431 c-89 -40 -98 -157 -18 -220 25 -20 39 -21 256 -21 254 0 274 4 304 62 21 40 14 120 -12 149 -38 43 -72 49 -286 49 -176 0 -208 -3 -244 -19z m483 -47 c39 -33 40 -99 3 -131 -24 -22 -31 -23 -238 -23 -134 0 -221 4 -234 11 -11 6 -26 26 -34 45 -12 29 -12 39 0 68 20 47 56 54 283 54 188 0 193 -1 220 -24z'/>"
            "<path d='M4399 2831 c-66 -36 -86 -123 -44 -188 9 -13 35 -35 58 -49 330 -196 332 -197 372 -197 22 0 50 5 63 12 55 29 78 120 47 180 -18 35 -375 249 -427 257 -21 2 -46 -3 -69 -15z m204 -94 c49 -29 97 -57 107 -62 10 -6 25 -17 34 -24 9 -7 16 -10 16 -5 0 5 4 4 8 -2 4 -6 26 -23 49 -37 44 -28 61 -66 48 -109 -9 -27 -40 -60 -49 -51 -3 4 -6 1 -6 -5 0 -14 -58 -6 -79 11 -9 7 -45 28 -81 47 -36 18 -70 37 -77 42 -24 19 -126 78 -133 78 -18 0 -67 72 -64 93 7 49 11 58 30 71 47 33 70 28 197 -47z'/>"
            "<path d='M1485 2760 c-93 -35 -340 -141 -363 -156 -26 -17 -52 -70 -52 -105 0 -35 32 -90 64 -111 47 -29 90 -19 306 73 l195 84 23 47 c20 40 22 52 13 85 -12 44 -42 76 -84 92 -37 14 -43 14 -102 -9z m102 -34 c28 -9 56 -75 39 -95 -9 -12 -8 -13 4 -6 21 13 -27 -46 -52 -64 -12 -8 -29 -15 -37 -16 -9 -1 -26 -7 -38 -13 -12 -7 -29 -12 -37 -12 -9 0 -16 -4 -16 -9 0 -5 -11 -12 -25 -15 -14 -4 -49 -17 -78 -30 -117 -53 -144 -60 -177 -46 -32 13 -56 51 -54 85 3 47 17 64 77 90 34 15 82 36 107 47 25 11 53 24 62 29 10 5 23 9 30 9 7 0 40 13 73 29 59 28 79 31 122 17z'/>"
            "<path d='M2430 1532 c-29 -13 -57 -37 -68 -59 -18 -33 -14 -94 8 -131 39 -63 34 -63 623 -60 532 3 532 3 563 25 65 46 71 140 15 197 l-31 31 -548 2 c-301 1 -554 -1 -562 -5z m1105 -52 c17 -11 26 -20 20 -20 -5 0 -4 -5 3 -12 29 -29 -1 -100 -49 -117 -46 -16 -1024 -15 -1067 1 -24 9 -35 22 -42 48 -5 19 -10 40 -10 46 0 20 40 63 63 67 12 2 254 5 537 6 498 1 516 0 545 -19z'/>"
            "<path d='M2509 1171 c-33 -32 -39 -45 -39 -80 0 -54 22 -96 63 -118 29 -16 69 -18 442 -18 324 0 417 3 442 14 82 34 97 146 29 208 l-36 33 -431 0 -431 0 -39 -39z m895 -13 c44 -19 56 -104 20 -136 -36 -32 -60 -33 -498 -25 -369 6 -370 6 -393 29 -25 25 -31 72 -10 93 10 11 10 13 0 7 -29 -16 -11 6 20 24 30 19 51 20 434 20 284 0 410 -3 427 -12z'/>"
            "<path d='M2636 859 c-32 -25 -56 -69 -56 -104 0 -35 24 -79 56 -104 26 -20 37 -21 345 -21 l317 0 31 26 c62 52 62 146 0 199 l-31 25 -317 0 c-308 0 -319 -1 -345 -21z m669 -47 c23 -23 26 -33 22 -70 -3 -24 -10 -49 -16 -55 -8 -8 -103 -14 -309 -17 -250 -4 -302 -3 -329 10 -18 8 -33 21 -33 28 0 6 -4 12 -9 12 -5 0 -8 18 -8 39 0 22 4 38 9 35 4 -3 8 1 8 9 0 7 14 20 30 28 25 11 85 13 319 11 l289 -3 27 -27z'/>"
            "</g>"
            "</g>"
            "</svg>";
        
        DrawSvgWithNanoSVG(svgContent, rect, animationProgress);
    }
    
    void DrawLightspeedReflexesIconAnimated(RECT rect, bool enabled, float animationProgress) {
        const char* svgContent = 
            "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 300 300'>"
            "<g fill='#888888'>"
            "<g transform='translate(150,150) scale(0.75) translate(-150,-150)'>"
            "<g transform='translate(150,150) scale(0.95) translate(-150,-150)'>"
            "<g fill='none' stroke='#888888' stroke-width='38' stroke-linecap='round' stroke-linejoin='round' transform='translate(0,300) scale(0.1,-0.1)'>"
            "<path d='M1414 2807 c-390 -268 -504 -323 -800 -390 -130 -29 -149 -38 -174 -80 -18 -29 -20 -51 -20 -212 0 -622 182 -1128 539 -1498 174 -179 465 -387 542 -387 33 0 156 69 269 150 408 294 656 687 760 1209 40 199 50 308 50 523 0 164 -2 186 -20 215 -26 43 -34 46 -202 87 -279 67 -428 138 -699 333 -138 99 -165 105 -245 50z m135 -38 l56 -42 -40 43 c-51 54 -25 41 96 -49 234 -175 426 -263 709 -325 130 -28 176 -55 191 -111 15 -53 5 -354 -16 -505 -9 -63 -19 -117 -22 -120 -3 -3 -3 20 1 50 29 218 36 546 14 595 -14 31 -34 34 -22 3 10 -28 12 -351 2 -468 -14 -171 -72 -437 -123 -563 -14 -35 -25 -68 -25 -73 0 -5 -14 -35 -30 -67 -17 -31 -28 -62 -25 -67 3 -6 1 -10 -5 -10 -6 0 -8 -4 -5 -10 3 -5 3 -10 -2 -10 -4 0 -21 -24 -37 -53 -122 -219 -299 -415 -511 -566 -141 -100 -217 -143 -258 -144 -36 -1 -106 38 -247 136 -436 306 -700 750 -784 1322 -31 208 -37 539 -12 598 5 9 18 19 30 22 11 3 67 14 122 25 252 50 487 162 747 357 54 40 108 73 119 73 12 0 46 -19 77 -41z'/>"
            "</g>"
            "</g>"
            "<g transform='translate(150,150) scale(0.33) translate(-145.113,-145.113)'>"
            "<g transform='translate(145.113,145.113) rotate(0) translate(-145.113,-145.113)'>"
            "<path d='M63.951,243.575c-1.945-3.578-4.401-6.907-7.363-9.869c-3.106-3.102-6.626-5.633-10.4-7.63 c-4.51-2.387-0.945-7.5-0.945-7.5c4.616-7.023,8.825-14.079,12.305-20.226l-23.363-23.344H11.504c-4.362,0-7.898-3.539-7.898-7.902 c0-4.361,3.536-7.9,7.898-7.9h25.947c2.1,0,4.107,0.832,5.588,2.312l85.379,85.291c1.483,1.483,2.315,3.495,2.315,5.589v26.073 c0,4.365-3.537,7.897-7.9,7.897c-4.367,0-7.904-3.531-7.904-7.897v-22.798l-23.27-23.24c-6.281,3.707-13.582,8.252-20.816,13.25 C70.842,245.679,66.698,248.629,63.951,243.575z'/>"
            "<path d='M26.61,237.102c-7.106,0-13.784,2.764-18.812,7.784c-5.019,5.015-7.782,11.686-7.782,18.778 c0,7.097,2.764,13.762,7.782,18.776c5.027,5.016,11.706,7.783,18.812,7.785c7.102,0,13.781-2.77,18.804-7.785 c5.023-5.015,7.79-11.682,7.79-18.776c0-7.093-2.768-13.764-7.79-18.778C40.392,239.866,33.712,237.102,26.61,237.102z'/>"
            "<path d='M100.985,182.318c-3.502,3.499-9.232,3.499-12.734,0.001l-8.81-8.801c-3.502-3.498-3.502-9.223,0-12.721L229.832,10.564 c3.502-3.498,10.401-6.727,15.33-7.175l36.862-3.352c4.93-0.448,8.596,3.218,8.148,8.148l-3.346,36.791 c-0.448,4.93-3.68,11.825-7.182,15.324l-150.4,150.251c-3.502,3.498-9.232,3.498-12.734,0l-8.822-8.813 c-3.502-3.498-3.502-9.223,0-12.722L233.608,63.213c1.854-1.848,1.856-4.852,0.003-6.702c-1.848-1.853-4.853-1.853-6.709-0.002 L100.985,182.318z'/>"
            "</g>"
            "<g transform='translate(145.113,145.113) scale(-1,1) translate(-145.113,-145.113)'>"
            "<path d='M63.951,243.575c-1.945-3.578-4.401-6.907-7.363-9.869c-3.106-3.102-6.626-5.633-10.4-7.63 c-4.51-2.387-0.945-7.5-0.945-7.5c4.616-7.023,8.825-14.079,12.305-20.226l-23.363-23.344H11.504c-4.362,0-7.898-3.539-7.898-7.902 c0-4.361,3.536-7.9,7.898-7.9h25.947c2.1,0,4.107,0.832,5.588,2.312l85.379,85.291c1.483,1.483,2.315,3.495,2.315,5.589v26.073 c0,4.365-3.537,7.897-7.9,7.897c-4.367,0-7.904-3.531-7.904-7.897v-22.798l-23.27-23.24c-6.281,3.707-13.582,8.252-20.816,13.25 C70.842,245.679,66.698,248.629,63.951,243.575z'/>"
            "<path d='M26.61,237.102c-7.106,0-13.784,2.764-18.812,7.784c-5.019,5.015-7.782,11.686-7.782,18.778 c0,7.097,2.764,13.762,7.782,18.776c5.027,5.016,11.706,7.783,18.812,7.785c7.102,0,13.781-2.77,18.804-7.785 c5.023-5.015,7.79-11.682,7.79-18.776c0-7.093-2.768-13.764-7.79-18.778C40.392,239.866,33.712,237.102,26.61,237.102z'/>"
            "<path d='M100.985,182.318c-3.502,3.499-9.232,3.499-12.734,0.001l-8.81-8.801c-3.502-3.498-3.502-9.223,0-12.721L229.832,10.564 c3.502-3.498,10.401-6.727,15.33-7.175l36.862-3.352c4.93-0.448,8.596,3.218,8.148,8.148l-3.346,36.791 c-0.448,4.93-3.68,11.825-7.182,15.324l-150.4,150.251c-3.502,3.498-9.232,3.498-12.734,0l-8.822-8.813 c-3.502-3.498-3.502-9.223,0-12.722L233.608,63.213c1.854-1.848,1.856-4.852,0.003-6.702c-1.848-1.853-4.853-1.853-6.709-0.002 L100.985,182.318z'/>"
            "</g>"
            "</g>"
            "</g>"
            "</svg>";
        
        DrawSvgWithNanoSVG(svgContent, rect, animationProgress);
    }
    
    void DrawPanelsIconAnimated(RECT rect, bool enabled, float animationProgress, bool hovered, float scale) {
        const char* svgContent = 
            "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' viewBox='0 0 24 24' fill='none' stroke='#000000' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>"
            "<rect width='18' height='18' x='3' y='3' rx='2'/>"
            "<path d='M3 9h18'/>"
            "<path d='M9 21V9'/>"
            "</svg>";
        
        char* mutableSvg = (char*)malloc(strlen(svgContent) + 1);
        strcpy(mutableSvg, svgContent);
        
        NSVGimage* image = nsvgParse(mutableSvg, "px", 96.0f);
        free(mutableSvg);
        
        if (!image) return;
        
        float centerX = (rect.left + rect.right) * 0.5f;
        float centerY = (rect.top + rect.bottom) * 0.5f;
        
        D2D1::Matrix3x2F originalTransform;
        m_renderTarget->GetTransform(&originalTransform);
        D2D1::Matrix3x2F scaleTransform = D2D1::Matrix3x2F::Scale(scale, scale, D2D1::Point2F(centerX, centerY));
        m_renderTarget->SetTransform(scaleTransform * originalTransform);
        
        float rw = (float)(rect.right - rect.left);
        float rh = (float)(rect.bottom - rect.top);
        float scaleX = rw / image->width;
        float scaleY = rh / image->height;
        float iconScale = min(scaleX, scaleY) * 1.2f;
        
        float offsetX = rect.left + (rw - image->width * iconScale) * 0.5f;
        float offsetY = rect.top + (rh - image->height * iconScale) * 0.5f;
        
        float grayWeight = 1.0f - animationProgress;
        float darkWeight = animationProgress;
        float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        
        ID2D1SolidColorBrush* brush = nullptr;
        D2D1_COLOR_F color = D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f);
        m_renderTarget->CreateSolidColorBrush(color, &brush);
        
        ID2D1StrokeStyle* strokeStyle = nullptr;
        m_d2dFactory->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_ROUND), nullptr, 0, &strokeStyle);
        
        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
            if (!(shape->flags & NSVG_FLAGS_VISIBLE)) continue;
            
            for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
                ID2D1PathGeometry* geometry = nullptr;
                m_d2dFactory->CreatePathGeometry(&geometry);
                
                ID2D1GeometrySink* sink = nullptr;
                geometry->Open(&sink);
                sink->SetFillMode(shape->fillRule == NSVG_FILLRULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);
                
                bool hasStarted = false;
                for (int i = 0; i < path->npts - 1; i += 3) {
                    float* p = &path->pts[i * 2];
                    
                    D2D1_POINT_2F p1 = D2D1::Point2F(offsetX + p[0] * iconScale, offsetY + p[1] * iconScale);
                    D2D1_POINT_2F p2 = D2D1::Point2F(offsetX + p[2] * iconScale, offsetY + p[3] * iconScale);
                    D2D1_POINT_2F p3 = D2D1::Point2F(offsetX + p[4] * iconScale, offsetY + p[5] * iconScale);
                    D2D1_POINT_2F p4 = D2D1::Point2F(offsetX + p[6] * iconScale, offsetY + p[7] * iconScale);
                    
                    if (!hasStarted) {
                        sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
                        hasStarted = true;
                    }
                    
                    sink->AddBezier(D2D1::BezierSegment(p2, p3, p4));
                }
                
                if (hasStarted) {
                    sink->EndFigure(path->closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
                }
                
                sink->Close();
                sink->Release();
                
                if (shape->fill.type != NSVG_PAINT_NONE) {
                    m_renderTarget->FillGeometry(geometry, brush);
                }
                if (shape->stroke.type != NSVG_PAINT_NONE) {
                    m_renderTarget->DrawGeometry(geometry, brush, shape->strokeWidth * iconScale, strokeStyle);
                }
                geometry->Release();
            }
        }
        
        if (strokeStyle) strokeStyle->Release();
        brush->Release();
        nsvgDelete(image);
        
        m_renderTarget->SetTransform(originalTransform);
    }
    
    void DrawAutoWispIconAnimated(RECT rect, bool enabled, float animationProgress) {
        float rw = (float)(rect.right - rect.left);
        float rh = (float)(rect.bottom - rect.top);
        float vb = 288.0f;
        float s = (min(rw, rh) / vb) * 0.07f;
        float ox = rect.left + (rw - vb * s) * 0.5f;
        float oy = rect.top + (rh - vb * s) * 0.5f;
        auto TP = [&](float x, float y) { return D2D1::Point2F(ox + x * s - 6.0f, oy + (vb - y) * s + 12.0f); };
        auto createPathFromSvg = [&](const char* d, ID2D1PathGeometry** outGeom) -> bool {
            *outGeom = nullptr;
            ID2D1PathGeometry* g = nullptr;
            HRESULT hrLocal = m_d2dFactory->CreatePathGeometry(&g);
            if (FAILED(hrLocal) || !g) return false;
            ID2D1GeometrySink* sinkLocal = nullptr;
            hrLocal = g->Open(&sinkLocal);
            if (FAILED(hrLocal) || !sinkLocal) { if (g) g->Release(); return false; }
            sinkLocal->SetFillMode(D2D1_FILL_MODE_WINDING);
            *outGeom = g;
            auto parseTo = [&](ID2D1GeometrySink* targetSink, const char* pathD, auto TPfn) {
                const char* p = pathD;
                auto skip = [&]() { while (*p && (*p==' '||*p=='\t'||*p=='\n'||*p==',')) ++p; };
                auto isnum = [&](char c){ return (c=='-'||c=='+'||c=='.'||(c>='0'&&c<='9')); };
                auto readf = [&]() -> float { skip(); char* e=nullptr; float v=(float)strtod(p,&e); p=e; return v; };
                float cx=0, cy=0, sx=0, sy=0; char cmd=0; bool figureOpen = false;
                while (*p) {
                    skip(); if (!*p) break; char c=*p;
                    if ((c>='A'&&c<='Z')||(c>='a'&&c<='z')) { cmd=c; ++p; }
                    else if (!cmd) { ++p; continue; }
                    switch(cmd) {
                        case 'M': {
                            float x=readf(), y=readf(); cx=sx=x; cy=sy=y;
                            targetSink->BeginFigure(TPfn(cx,cy), D2D1_FIGURE_BEGIN_FILLED);
                            figureOpen = true;
                            while (isnum(*p)) { x=readf(); y=readf(); cx=x; cy=y; targetSink->AddLine(TPfn(cx,cy)); }
                            break;
                        }
                        case 'm': {
                            float dx=readf(), dy=readf(); cx+=dx; cy+=dy; sx=cx; sy=cy;
                            targetSink->BeginFigure(TPfn(cx,cy), D2D1_FIGURE_BEGIN_FILLED);
                            figureOpen = true;
                            while (isnum(*p)) { dx=readf(); dy=readf(); cx+=dx; cy+=dy; targetSink->AddLine(TPfn(cx,cy)); }
                            break;
                        }
                        case 'L': { while (isnum(*p)) { float x=readf(), y=readf(); cx=x; cy=y; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'l': { while (isnum(*p)) { float dx=readf(), dy=readf(); cx+=dx; cy+=dy; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'H': { while (isnum(*p)) { float x=readf(); cx=x; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'h': { while (isnum(*p)) { float dx=readf(); cx+=dx; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'V': { while (isnum(*p)) { float y=readf(); cy=y; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'v': { while (isnum(*p)) { float dy=readf(); cy+=dy; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'C': { while (isnum(*p)) { float x1=readf(), y1=readf(), x2=readf(), y2=readf(), x=readf(), y=readf(); D2D1_BEZIER_SEGMENT b={TPfn(x1,y1),TPfn(x2,y2),TPfn(x,y)}; targetSink->AddBezier(b); cx=x; cy=y; } break; }
                        case 'c': { while (isnum(*p)) { float dx1=readf(), dy1=readf(), dx2=readf(), dy2=readf(), dx=readf(), dy=readf(); D2D1_BEZIER_SEGMENT b={TPfn(cx+dx1,cy+dy1),TPfn(cx+dx2,cy+dy2),TPfn(cx+dx,cy+dy)}; targetSink->AddBezier(b); cx+=dx; cy+=dy; } break; }
                        case 'Z': case 'z': { targetSink->EndFigure(D2D1_FIGURE_END_CLOSED); cx=sx; cy=sy; cmd=0; figureOpen = false; break; }
                        default: { ++p; break; }
                    }
                }
                if (figureOpen) {
                    targetSink->EndFigure(D2D1_FIGURE_END_CLOSED);
                }
            };
            parseTo(sinkLocal, d, TP);
            sinkLocal->Close();
            sinkLocal->Release();
            return true;
        };

        ID2D1PathGeometry* g1 = nullptr;
        ID2D1PathGeometry* g2 = nullptr;
        ID2D1PathGeometry* g3 = nullptr;
        ID2D1PathGeometry* g4 = nullptr;
        ID2D1PathGeometry* g5 = nullptr;
        ID2D1PathGeometry* g6 = nullptr;
        ID2D1PathGeometry* g7 = nullptr;

        const char* d1 = R"(M835 1395 c-33 -24 -73 -49 -88 -56 -26 -11 -31 -9 -61 20 -36 35 -55 38 -114 17 -44 -16 -242 -144 -242 -157 0 -16 130 -324 151 -358 l21 -34 71 76 c72 76 305 269 372 308 20 12 34 27 31 34 -4 10 -26 86 -52 178 -8 25 -22 20 -89 -28z)";
        const char* d2 = R"(M1330 1306 c-74 -28 -203 -97 -350 -189 -112 -71 -426 -354 -436 -394 -5 -17 8 -48 44 -111 27 -48 62 -111 76 -139 15 -29 31 -53 36 -53 6 0 43 24 83 53 84 62 226 154 280 181 20 11 41 24 46 30 4 6 30 65 56 131 26 66 88 203 139 305 50 101 93 188 94 192 5 12 -28 8 -68 -6z)";
        const char* d3 = R"(M739 2927 c-21 -18 -44 -46 -53 -64 -14 -28 -76 -191 -76 -200 0 -2 35 14 78 36 106 54 176 85 252 112 36 12 67 24 69 26 7 5 -72 83 -84 83 -7 0 -20 9 -30 20 -14 15 -30 20 -69 20 -43 -1 -57 -6 -87 -33z)";
        const char* d4 = R"(M916 2695 c-15 -8 -33 -15 -40 -15 -17 0 -208 -95 -238 -120 -17 -13 -28 -31 -28 -46 0 -28 -2 -28 42 -9 31 12 38 12 58 -2 22 -16 80 -125 80 -152 0 -32 -167 -165 -194 -155 -9 3 -16 2 -16 -3 0 -15 -88 -59 -143 -72 -50 -12 -216 -99 -253 -133 -14 -12 -20 -27 -17 -39 7 -25 -11 -46 -47 -54 -15 -3 -45 -21 -66 -39 -37 -32 -38 -34 -23 -57 8 -13 26 -50 38 -81 13 -32 26 -58 29 -58 2 0 39 33 81 73 42 40 131 109 196 153 247 165 505 265 807 314 73 12 134 22 135 23 1 2 -27 38 -62 82 -85 105 -163 260 -173 340 l-7 60 -65 3 c-43 1 -75 -3 -94 -13z)";
        const char* d5 = R"(M472 2383 c-17 -38 -38 -88 -48 -113 -9 -25 -25 -54 -36 -66 -10 -11 -18 -23 -18 -27 0 -8 22 -5 30 3 12 13 82 50 96 50 16 0 19 16 4 25 -6 4 -4 15 6 30 15 24 33 31 132 55 35 9 42 15 42 35 0 33 -13 38 -56 20 -46 -19 -81 -10 -103 26 l-18 29 -31 -67z)";
        const char* d6 = R"(M1230 2104 c-129 -19 -192 -35 -366 -89 -186 -59 -208 -68 -320 -139 -65 -41 -168 -112 -229 -158 -93 -71 -112 -91 -129 -131 l-20 -48 27 -54 c15 -31 27 -62 27 -70 0 -7 10 -26 23 -40 l22 -27 50 99 c66 130 179 248 212 221 15 -12 17 -40 3 -54 -8 -8 -100 -174 -97 -174 1 0 25 8 52 19 28 10 70 22 95 26 70 11 116 52 171 151 27 48 47 89 45 91 -1 2 -27 -8 -57 -22 -62 -29 -130 -33 -150 -9 -9 11 -10 21 -3 31 10 16 208 121 344 182 41 18 90 41 107 49 30 14 34 14 53 -3 11 -10 20 -23 20 -29 0 -27 -186 -275 -279 -372 -56 -58 -101 -110 -101 -115 0 -18 20 -8 95 45 101 73 112 76 145 39 25 -27 70 -161 70 -206 0 -9 4 -18 9 -21 4 -3 61 22 125 55 65 34 154 74 198 89 44 16 84 32 88 37 4 4 29 58 55 118 25 61 61 137 79 171 61 110 57 165 -12 183 -33 9 -43 20 -79 83 l-40 73 -104 1 c-57 1 -115 0 -129 -2z)";
        const char* d7 = R"(M973 486 c-39 -24 -78 -49 -85 -55 -7 -6 -41 -33 -75 -61 -35 -28 -63 -53 -63 -56 0 -13 112 -234 119 -234 4 0 33 71 66 158 32 86 71 183 87 215 16 31 28 62 26 67 -2 6 -35 -9 -75 -34z)";

        if (!createPathFromSvg(d1, &g1)) return;
        if (!createPathFromSvg(d2, &g2)) { g1->Release(); return; }
        if (!createPathFromSvg(d3, &g3)) { g1->Release(); g2->Release(); return; }
        if (!createPathFromSvg(d4, &g4)) { g1->Release(); g2->Release(); g3->Release(); return; }
        if (!createPathFromSvg(d5, &g5)) { g1->Release(); g2->Release(); g3->Release(); g4->Release(); return; }
        if (!createPathFromSvg(d6, &g6)) { g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); return; }
        if (!createPathFromSvg(d7, &g7)) { g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); return; }

        // Union g1 and g2 -> u12
        ID2D1PathGeometry* u12 = nullptr; m_d2dFactory->CreatePathGeometry(&u12);
        if (!u12) { g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        ID2D1GeometrySink* sinkU12 = nullptr; u12->Open(&sinkU12);
        if (!sinkU12) { u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        sinkU12->SetFillMode(D2D1_FILL_MODE_WINDING);
        g1->CombineWithGeometry(g2, D2D1_COMBINE_MODE_UNION, nullptr, sinkU12);
        sinkU12->Close(); sinkU12->Release();
        // Union u12 and g3 -> u123
        ID2D1PathGeometry* u123 = nullptr; m_d2dFactory->CreatePathGeometry(&u123);
        if (!u123) { u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        ID2D1GeometrySink* sinkU123 = nullptr; u123->Open(&sinkU123);
        if (!sinkU123) { u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        sinkU123->SetFillMode(D2D1_FILL_MODE_WINDING);
        u12->CombineWithGeometry(g3, D2D1_COMBINE_MODE_UNION, nullptr, sinkU123);
        sinkU123->Close(); sinkU123->Release();

        // Union u123 and g4 -> u1234
        ID2D1PathGeometry* u1234 = nullptr; m_d2dFactory->CreatePathGeometry(&u1234);
        if (!u1234) { u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        ID2D1GeometrySink* sinkU1234 = nullptr; u1234->Open(&sinkU1234);
        if (!sinkU1234) { u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        sinkU1234->SetFillMode(D2D1_FILL_MODE_WINDING);
        u123->CombineWithGeometry(g4, D2D1_COMBINE_MODE_UNION, nullptr, sinkU1234);
        sinkU1234->Close(); sinkU1234->Release();

        // Union u1234 and g5 -> u12345
        ID2D1PathGeometry* u12345 = nullptr; m_d2dFactory->CreatePathGeometry(&u12345);
        if (!u12345) { u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        ID2D1GeometrySink* sinkU12345 = nullptr; u12345->Open(&sinkU12345);
        if (!sinkU12345) { u12345->Release(); u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        sinkU12345->SetFillMode(D2D1_FILL_MODE_WINDING);
        u1234->CombineWithGeometry(g5, D2D1_COMBINE_MODE_UNION, nullptr, sinkU12345);
        sinkU12345->Close(); sinkU12345->Release();

        // Union u12345 and g6 -> u123456
        ID2D1PathGeometry* u123456 = nullptr; m_d2dFactory->CreatePathGeometry(&u123456);
        if (!u123456) { u12345->Release(); u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        ID2D1GeometrySink* sinkU123456 = nullptr; u123456->Open(&sinkU123456);
        if (!sinkU123456) { u123456->Release(); u12345->Release(); u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        sinkU123456->SetFillMode(D2D1_FILL_MODE_WINDING);
        u12345->CombineWithGeometry(g6, D2D1_COMBINE_MODE_UNION, nullptr, sinkU123456);
        sinkU123456->Close(); sinkU123456->Release();

        // Union u123456 and g7 -> final
        ID2D1PathGeometry* pathGeometry = nullptr; m_d2dFactory->CreatePathGeometry(&pathGeometry);
        if (!pathGeometry) { u123456->Release(); u12345->Release(); u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        ID2D1GeometrySink* sinkFinal = nullptr; pathGeometry->Open(&sinkFinal);
        if (!sinkFinal) { pathGeometry->Release(); u123456->Release(); u12345->Release(); u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release(); return; }
        sinkFinal->SetFillMode(D2D1_FILL_MODE_WINDING);
        u123456->CombineWithGeometry(g7, D2D1_COMBINE_MODE_UNION, nullptr, sinkFinal);
        sinkFinal->Close(); sinkFinal->Release();

        g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); g6->Release(); g7->Release();
        u12->Release(); u123->Release(); u1234->Release(); u12345->Release(); u123456->Release();

        float grayWeight = 1.0f - animationProgress;
        float darkWeight = animationProgress;
        float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;

        ID2D1SolidColorBrush* iconBrush = nullptr;
        D2D1_COLOR_F iconColor = D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f);
        m_renderTarget->CreateSolidColorBrush(iconColor, &iconBrush);
        m_renderTarget->FillGeometry(pathGeometry, iconBrush);
        iconBrush->Release();
        pathGeometry->Release();
    }
    
    void DrawParryBarIconAnimated(RECT rect, bool enabled, float animationProgress) {
        ID2D1PathGeometry* pathGeometry = nullptr;
        HRESULT hr = m_d2dFactory->CreatePathGeometry(&pathGeometry);
        if (FAILED(hr)) return;
        
        ID2D1GeometrySink* sink = nullptr;
        hr = pathGeometry->Open(&sink);
        if (FAILED(hr)) {
            pathGeometry->Release();
            return;
        }
        
        float centerX = (rect.left + rect.right) / 2.0f;
        float centerY = (rect.top + rect.bottom) / 2.0f;
        
        float rectLeft = centerX - 7.5f;
        float rectTop = centerY - 2.5f;
        float rectRight = centerX + 7.5f;
        float rectBottom = centerY + 2.5f;
        
        float lineStart = centerX - 6.0f;
        float lineEnd = centerX + 6.0f;
        
        sink->BeginFigure(D2D1::Point2F(rectLeft, rectTop), D2D1_FIGURE_BEGIN_HOLLOW);
        sink->AddLine(D2D1::Point2F(rectRight, rectTop));
        sink->AddLine(D2D1::Point2F(rectRight, rectBottom));
        sink->AddLine(D2D1::Point2F(rectLeft, rectBottom));
        sink->AddLine(D2D1::Point2F(rectLeft, rectTop));
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
        
        sink->BeginFigure(D2D1::Point2F(lineStart, centerY), D2D1_FIGURE_BEGIN_HOLLOW);
        sink->AddLine(D2D1::Point2F(lineEnd, centerY));
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        
        sink->Close();
        sink->Release();
        
        float grayWeight = 1.0f - animationProgress;
        float darkWeight = animationProgress;
        
        float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        
        ID2D1SolidColorBrush* strokeBrush = nullptr;
        D2D1_COLOR_F strokeColor = D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f);
        m_renderTarget->CreateSolidColorBrush(strokeColor, &strokeBrush);
        
        m_renderTarget->DrawGeometry(pathGeometry, strokeBrush, 1.5f);
        strokeBrush->Release();
        pathGeometry->Release();
    }
    
    void DrawCrosshairIconAnimated(RECT rect, bool enabled, float animationProgress) {
        ID2D1PathGeometry* pathGeometry = nullptr;
        HRESULT hr = m_d2dFactory->CreatePathGeometry(&pathGeometry);
        if (FAILED(hr)) return;
        
        ID2D1GeometrySink* sink = nullptr;
        hr = pathGeometry->Open(&sink);
        if (FAILED(hr)) {
            pathGeometry->Release();
            return;
        }
        
        float centerX = (rect.left + rect.right) / 2.0f;
        float centerY = (rect.top + rect.bottom) / 2.0f;
        
        float armLength = 8.0f;
        float armWidth = 2.0f;
        float gapSize = 2.5f;
        
        sink->BeginFigure(D2D1::Point2F(centerX - armLength, centerY - armWidth/2), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine(D2D1::Point2F(centerX - gapSize, centerY - armWidth/2));
        sink->AddLine(D2D1::Point2F(centerX - gapSize, centerY + armWidth/2));
        sink->AddLine(D2D1::Point2F(centerX - armLength, centerY + armWidth/2));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        
        sink->BeginFigure(D2D1::Point2F(centerX + gapSize, centerY - armWidth/2), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine(D2D1::Point2F(centerX + armLength, centerY - armWidth/2));
        sink->AddLine(D2D1::Point2F(centerX + armLength, centerY + armWidth/2));
        sink->AddLine(D2D1::Point2F(centerX + gapSize, centerY + armWidth/2));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        
        sink->BeginFigure(D2D1::Point2F(centerX - armWidth/2, centerY - armLength), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine(D2D1::Point2F(centerX + armWidth/2, centerY - armLength));
        sink->AddLine(D2D1::Point2F(centerX + armWidth/2, centerY - gapSize));
        sink->AddLine(D2D1::Point2F(centerX - armWidth/2, centerY - gapSize));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        
        sink->BeginFigure(D2D1::Point2F(centerX - armWidth/2, centerY + gapSize), D2D1_FIGURE_BEGIN_FILLED);
        sink->AddLine(D2D1::Point2F(centerX + armWidth/2, centerY + gapSize));
        sink->AddLine(D2D1::Point2F(centerX + armWidth/2, centerY + armLength));
        sink->AddLine(D2D1::Point2F(centerX - armWidth/2, centerY + armLength));
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        
        sink->Close();
        sink->Release();
        
        float grayWeight = 1.0f - animationProgress;
        float darkWeight = animationProgress;
        
        float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        
        ID2D1SolidColorBrush* fillBrush = nullptr;
        D2D1_COLOR_F fillColor = D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f);
        m_renderTarget->CreateSolidColorBrush(fillColor, &fillBrush);
        
        m_renderTarget->FillGeometry(pathGeometry, fillBrush);
        fillBrush->Release();
        pathGeometry->Release();
    }
    
    void DrawRollSpitIconAnimated(RECT rect, bool enabled, float animationProgress) {
        const char* svgContent = 
            "<svg xmlns='http://www.w3.org/2000/svg' width='300' height='450' viewBox='0 0 300 450'>"
            "<g transform='translate(150,225) scale(0.45) translate(-150,-225)'>"
            "<g transform='translate(0,450) scale(0.05,-0.05)'>"
            "<path d='M3613 7339 c-595 -71 -1291 -434 -1945 -1014 -283 -251 -344 -364 -257 -478 72 -94 100 -105 415 -159 525 -91 768 -195 1313 -564 523 -354 786 -455 1141 -440 429 19 800 278 1014 710 l96 192 0 292 c0 840 -885 1568 -1777 1461z m517 -191 c993 -241 1468 -1288 876 -1930 -477 -516 -972 -495 -1831 80 -610 409 -982 552 -1548 595 -204 16 -204 3 8 199 914 842 1812 1222 2495 1056z'/>"
            "<g transform='translate(0,-800)'>"
            "<path d='M678 5360 c-194 -18 -275 -52 -337 -139 -69 -97 -42 -144 261 -446 188 -187 313 -333 397 -460 67 -102 128 -194 136 -206 8 -11 17 -29 20 -40 3 -10 28 -84 55 -162 27 -79 64 -232 81 -340 136 -864 189 -1036 441 -1419 582 -887 1566 -1075 2208 -423 141 143 378 527 358 580 -1 3 9 45 22 95 140 518 -188 1419 -708 1947 -161 163 -455 398 -552 440 -27 12 -55 29 -61 37 -41 56 -762 349 -801 325 -10 -6 -18 -3 -18 7 0 26 -529 152 -579 138 -23 -5 -37 -3 -31 5 25 41 -635 86 -892 61z m602 -171 c1489 -121 2661 -1004 2911 -2195 195 -926 -652 -1720 -1514 -1420 -679 236 -1068 819 -1209 1816 -107 756 -386 1239 -966 1676 -147 110 -149 108 158 132 225 17 316 16 620 -9z'/>"
            "</g>"
            "</g>"
            "</g>"
            "</svg>";
        
        DrawSvgWithNanoSVG(svgContent, rect, animationProgress);
    }
    
    void DrawHoldM1IconAnimated(RECT rect, bool enabled, float animationProgress) {
        const char* svgContent = 
            "<svg xmlns='http://www.w3.org/2000/svg' width='1011' height='923'>"
            "<path d='M456.755 499.088c-5.123 5.119-13.505 5.119-18.628.001l-12.888-12.874c-5.122-5.117-5.122-13.492 0-18.61l220-219.768c5.123-5.117 15.216-9.84 22.426-10.496l53.924-4.903c7.212-.655 12.574 4.707 11.92 11.92l-4.896 53.82c-.655 7.211-5.383 17.297-10.506 22.416L498.094 540.389c-5.123 5.117-13.505 5.117-18.628 0l-12.905-12.892c-5.123-5.117-5.123-13.492 0-18.61l184.202-184.032a6.924 6.924 0 0 0 .003-9.804c-2.703-2.71-7.1-2.71-9.814-.002zm-108.8 80.141c-10.395 0-20.164 4.044-27.519 11.387-7.342 7.336-11.384 17.095-11.384 27.47 0 10.382 4.044 20.131 11.384 27.466 7.354 7.338 17.124 11.386 27.52 11.388 10.389 0 20.159-4.052 27.507-11.388 7.348-7.336 11.396-17.089 11.396-27.466 0-10.376-4.05-20.135-11.396-27.47-7.347-7.343-17.118-11.387-27.508-11.387zm54.625 9.47a62.788 62.788 0 0 0-10.771-14.438c-4.544-4.537-9.693-8.24-15.214-11.161-6.597-3.492-1.382-10.972-1.382-10.972 6.752-10.273 12.91-20.595 18-29.587l-34.176-34.149h-33.18c-6.38 0-11.553-5.177-11.553-11.56 0-6.379 5.173-11.556 11.554-11.556h37.956c3.072 0 6.008 1.217 8.175 3.382l124.897 124.768a11.566 11.566 0 0 1 3.386 8.176v38.141c0 6.386-5.174 11.552-11.556 11.552-6.389 0-11.563-5.165-11.563-11.552v-33.35l-34.04-33.997c-9.189 5.423-19.869 12.072-30.451 19.383-.001-.002-6.064 4.313-10.082-3.08z' style='stroke-width:1.46285'/>"
            "<g transform='translate(200, 0) scale(1.5)'>"
            "<g transform='translate(0,320) scale(0.1,-0.1)'>"
            "<path d='M1354 2596 c-3 -9 -2 -71 2 -139 8 -112 11 -122 29 -122 19 0 20 5 16 109 -2 60 -7 121 -10 134 -8 31 -29 40 -37 18z'/>"
            "<path d='M1641 2389 c-76 -86 -92 -117 -65 -126 13 -4 179 182 179 200 0 6 -7 13 -16 15 -10 2 -48 -32 -98 -89z'/>"
            "<path d='M1003 2423 c-7 -17 8 -32 134 -140 48 -42 83 -51 83 -24 0 12 -189 181 -201 181 -6 0 -13 -8 -16 -17z'/>"
            "<path d='M1762 2130 c-57 -4 -105 -10 -109 -13 -3 -4 -3 -14 1 -23 6 -14 20 -15 129 -9 67 4 127 12 134 17 19 15 3 38 -26 37 -14 -1 -72 -5 -129 -9z'/>"
            "<path d='M1393 2088 c-16 -20 -23 -190 -24 -584 -2 -394 -1 -396 57 -422 38 -18 53 -15 154 28 51 22 95 38 98 35 2 -2 37 -77 76 -167 88 -198 94 -204 181 -164 77 35 95 52 95 91 0 34 -24 75 -36 62 -4 -4 -2 -13 4 -19 15 -15 16 -32 2 -23 -5 3 -10 0 -10 -8 0 -8 -7 -20 -15 -27 -17 -14 -21 0 -5 16 7 7 4 10 -12 10 -29 -1 -31 27 -3 32 16 3 17 1 5 -7 -12 -9 -10 -11 9 -11 l24 0 -21 23 c-24 25 -30 46 -9 29 7 -6 19 -7 26 -3 10 6 0 37 -42 135 -31 69 -53 128 -49 130 4 2 48 20 97 40 106 44 135 70 135 127 0 50 5 44 -390 419 -290 275 -318 295 -347 258z m361 -325 c167 -159 311 -301 320 -316 32 -51 16 -74 -82 -119 -94 -43 -132 -72 -132 -99 0 -10 19 -61 42 -114 l42 -97 -17 -46 c-14 -41 -15 -49 -2 -69 8 -12 15 -27 15 -33 0 -14 -49 -32 -70 -25 -10 3 -40 58 -76 142 -64 149 -93 193 -128 193 -12 0 -50 -11 -83 -25 -87 -35 -135 -41 -157 -20 -15 16 -17 53 -15 434 1 229 3 431 6 449 3 19 11 32 20 32 7 0 150 -129 317 -287z'/>"
            "<path d='M973 2081 c-97 -6 -109 -11 -99 -36 5 -13 22 -15 128 -9 145 7 148 7 148 34 0 22 3 22 -177 11z'/>"
            "<path d='M1122 1804 c-79 -89 -95 -124 -59 -124 20 0 180 189 173 205 -10 28 -31 13 -114 -81z'/>"
            "</g>"
            "<path d='m 198.81199,221.25517 c 0.0313,0.30345 0.15921,0.58521 0.23243,0.8781 0.0127,0.0508 0.0121,0.10443 0.0258,0.15495 0.037,0.13573 0.12402,0.23669 0.18078,0.36157 0.0624,0.13732 0.10141,0.29634 0.20662,0.41323 0.045,0.0499 0.10998,0.0792 0.15495,0.12913 0.0336,0.0373 0.0496,0.0874 0.0775,0.12913 0.0682,0.10228 0.14576,0.19741 0.23244,0.2841' style='fill:#000000;stroke:#000000;stroke-width:0.751177'/>"
            "<path d='m 198.91529,221.61673 c -0.006,0.26548 0.0368,0.53103 0.10331,0.78771 0.0268,0.10327 0.0796,0.16668 0.11622,0.27118 0.0946,0.27037 -0.075,-0.0922 0.0387,0.18078 0.025,0.0601 0.0744,0.11 0.1033,0.16787 0.0192,0.0383 0.0292,0.0954 0.0517,0.12914 0.007,0.0101 0.0204,0.0149 0.0258,0.0258 0.007,0.0129 0.0224,0.10854 0.0258,0.11622 0.009,0.0196 0.0291,0.0324 0.0387,0.0517 0.029,0.0581 0.0356,0.11553 0.0775,0.16787 0.0264,0.033 0.0651,0.0566 0.0904,0.0904 0.0159,0.0212 0.0195,0.0453 0.0388,0.0646' style='fill:#000000;stroke:#000000;stroke-width:0.751177'/>"
            "<path d='m 196.97453,220.82477 c -0.45091,-0.22831 -0.86866,-0.25567 -1.35139,-0.25567 -0.0487,0 -0.10254,-0.0217 -0.1461,0 -0.0983,0.0491 -0.15182,0.17712 -0.21914,0.25567 -0.0842,0.0983 -0.16416,0.20068 -0.25567,0.29219 -0.031,0.031 -0.0832,0.0379 -0.10958,0.073 -0.0146,0.0195 0.0146,0.0536 0,0.073 -0.0263,0.0351 -0.0785,0.042 -0.10957,0.073 -0.0502,0.0502 -0.0594,0.13243 -0.10957,0.18263 -0.0356,0.0356 -0.2163,0.10389 -0.21914,0.10957 -0.0731,0.1461 0.0365,0.0183 0,0.10957 -0.081,0.20255 -0.24321,0.34835 -0.32872,0.54786 -0.041,0.0956 -0.087,0.19065 -0.10958,0.29219 -0.0106,0.0476 0.014,0.0995 0,0.1461 -0.0616,0.20518 -0.23336,0.37848 -0.29219,0.58439 -0.0643,0.22494 -0.0799,0.46876 -0.14609,0.69396 0.0291,0.24912 -0.18385,0.44196 -0.25567,0.65743 -0.0734,0.22011 -0.0483,0.43456 -0.0731,0.65743 -0.0233,0.2099 -0.0722,0.44353 -0.0365,0.65744 0.005,0.0268 0.0312,0.0463 0.0365,0.073 0.0433,0.2163 -0.0607,0.658 0,0.84005 0.0224,0.0674 0.0853,0.1159 0.10956,0.18262 0.0558,0.15335 0.0945,0.41905 0.10958,0.58439 0.007,0.0728 -0.0143,0.14752 0,0.21914 0.0106,0.0534 0.0558,0.0944 0.073,0.1461 0.0197,0.0589 0.0187,0.12316 0.0365,0.18262 0.0739,0.24654 0.0598,0.11288 0.1461,0.32872 0.1251,0.31274 0.20291,0.65202 0.36524,0.94962 0.12298,0.22546 0.27879,0.43198 0.40177,0.65744 0.0391,0.0717 0.0627,0.15223 0.10956,0.21914 0.0395,0.0564 0.10308,0.0923 0.1461,0.1461 0.0548,0.0685 0.0899,0.1517 0.1461,0.21914 0.162,0.1944 0.37232,0.3358 0.54786,0.51134 0.20126,0.20126 0.32293,0.43914 0.58439,0.58438 0.53328,0.29628 1.28323,0.29975 1.86273,0.32872 0.48652,0.0243 1.24463,0.12608 1.6801,-0.14609 0.19011,-0.11882 0.33655,-0.38446 0.47482,-0.54786 0.21954,-0.25946 0.49461,-0.49825 0.65743,-0.80354 0.13952,-0.2616 0.19607,-0.57475 0.32872,-0.84005 0.0392,-0.0785 0.10683,-0.14062 0.1461,-0.21914 0.0625,-0.12516 0.16798,-0.60767 0.18261,-0.65744 0.0668,-0.22708 0.15285,-0.45572 0.18263,-0.69396 0.0281,-0.22516 0.0236,-0.7134 0,-0.94962 -0.0201,-0.20067 -0.10677,-0.38775 -0.1461,-0.58439 -0.0465,-0.23233 -0.0201,-0.4338 -0.10957,-0.65743 -0.0653,-0.16332 -0.21308,-0.30444 -0.25567,-0.47481 -0.0118,-0.0473 0.0118,-0.0989 0,-0.1461 -0.0183,-0.0733 -0.25409,-0.49547 -0.2922,-0.54786 -0.1447,-0.19897 -0.21309,-0.15051 -0.32871,-0.36525 -0.0494,-0.0916 -0.0665,-0.19749 -0.10957,-0.29219 -0.11535,-0.25376 -0.36417,-0.43579 -0.47482,-0.69396 -0.0548,-0.12783 0.0274,-0.0274 0,-0.10957 -0.046,-0.13822 -0.16447,-0.2651 -0.21914,-0.40176 -0.009,-0.0226 0.009,-0.0505 0,-0.073 -0.0442,-0.11052 -0.158,-0.19029 -0.21914,-0.2922 -0.0258,-0.043 -0.009,-0.10432 -0.0365,-0.14609 -0.1219,-0.18286 -0.34698,-0.29584 -0.47482,-0.47482 -0.18288,-0.25602 0.0782,-0.0555 -0.25566,-0.32871 -0.53323,-0.43628 0.10279,0.16727 -0.43829,-0.32872 -0.44311,-0.40619 0.0282,-0.0569 -0.51134,-0.36524 -0.31921,-0.1824 -0.32855,-0.23482 -0.69395,-0.18262 z' style='fill:#000000;stroke:#000000;stroke-width:0.751177'/>"
            "<path d='m 200.69731,223.42458 c -0.21696,-0.0784 -0.10068,-0.33246 -0.20661,-0.46487 -0.0388,-0.0485 -0.11106,-0.0594 -0.15496,-0.1033 -0.15682,-0.15683 -0.0771,-0.20976 -0.20661,-0.41323 -0.0722,-0.11345 -0.18367,-0.19802 -0.25827,-0.30992 -0.0641,-0.0961 -0.0909,-0.21381 -0.15495,-0.30991 -0.0405,-0.0607 -0.10331,-0.10331 -0.15496,-0.15496 -0.0517,-0.0688 -0.10933,-0.13361 -0.15496,-0.20662 -0.19676,-0.31481 -0.31252,-0.64622 -0.46487,-0.9814 -0.18679,-0.41093 -0.40967,-0.81694 -0.56819,-1.23967 -0.29526,-0.78736 -0.46523,-1.75655 -1.08471,-2.37603' style='fill:#000000;stroke:#000000;stroke-width:0.751177'/>"
            "</g>"
            "</svg>";
        
        DrawSvgWithNanoSVG(svgContent, rect, animationProgress);
    }
    
    void DrawExitIconAnimated(RECT rect, bool enabled, float animationProgress, bool hovered, float scale) {
        const char* svgContent = 
            "<svg version='1.0' xmlns='http://www.w3.org/2000/svg' width='111.000000pt' height='126.000000pt' viewBox='0 0 111.000000 126.000000' preserveAspectRatio='xMidYMid meet'>"
            "<g transform='translate(0.000000,126.000000) scale(0.050000,-0.050000)' fill='#000000' stroke='none'>"
            "<path d='M652 2015 c-146 -84 -152 -111 -152 -737 0 -768 -10 -755 580 -759 l370 -2 0 103 0 104 -337 -3 c-454 -4 -413 -59 -413 553 0 626 -42 566 401 566 l359 0 0 110 0 110 -365 0 c-341 -1 -370 -4 -443 -45z'/>"
            "<path d='M1174 1523 c-232 -237 -232 -248 -7 -476 144 -146 175 -169 225 -164 101 12 92 110 -21 214 -50 46 -91 88 -91 93 0 6 131 10 290 10 l290 0 0 93 0 92 -290 -9 c-330 -10 -323 -14 -190 112 109 104 118 129 63 179 -61 55 -89 40 -269 -144z'/>"
            "</g>"
            "</svg>";
        
        char* mutableSvg = (char*)malloc(strlen(svgContent) + 1);
        strcpy(mutableSvg, svgContent);
        
        NSVGimage* image = nsvgParse(mutableSvg, "px", 96.0f);
        free(mutableSvg);
        
        if (!image) return;
        
        float centerX = (rect.left + rect.right) * 0.5f;
        float centerY = (rect.top + rect.bottom) * 0.5f;
        
        D2D1::Matrix3x2F originalTransform;
        m_renderTarget->GetTransform(&originalTransform);
        D2D1::Matrix3x2F scaleTransform = D2D1::Matrix3x2F::Scale(scale, scale, D2D1::Point2F(centerX, centerY));
        m_renderTarget->SetTransform(scaleTransform * originalTransform);
        
        float rw = (float)(rect.right - rect.left);
        float rh = (float)(rect.bottom - rect.top);
        float scaleX = rw / image->width;
        float scaleY = rh / image->height;
        float iconScale = min(scaleX, scaleY) * 1.2f;
        
        float offsetX = rect.left + (rw - image->width * iconScale) * 0.5f;
        float offsetY = rect.top + (rh - image->height * iconScale) * 0.5f;
        
        ID2D1SolidColorBrush* brush = nullptr;
        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &brush);
        
        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
            if (!(shape->flags & NSVG_FLAGS_VISIBLE)) continue;
            
            for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
                ID2D1PathGeometry* geometry = nullptr;
                m_d2dFactory->CreatePathGeometry(&geometry);
                
                ID2D1GeometrySink* sink = nullptr;
                geometry->Open(&sink);
                sink->SetFillMode(shape->fillRule == NSVG_FILLRULE_EVENODD ? D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);
                
                bool hasStarted = false;
                for (int i = 0; i < path->npts - 1; i += 3) {
                    float* p = &path->pts[i * 2];
                    
                    D2D1_POINT_2F p1 = D2D1::Point2F(offsetX + p[0] * iconScale, offsetY + p[1] * iconScale);
                    D2D1_POINT_2F p2 = D2D1::Point2F(offsetX + p[2] * iconScale, offsetY + p[3] * iconScale);
                    D2D1_POINT_2F p3 = D2D1::Point2F(offsetX + p[4] * iconScale, offsetY + p[5] * iconScale);
                    D2D1_POINT_2F p4 = D2D1::Point2F(offsetX + p[6] * iconScale, offsetY + p[7] * iconScale);
                    
                    if (!hasStarted) {
                        sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
                        hasStarted = true;
                    }
                    
                    sink->AddBezier(D2D1::BezierSegment(p2, p3, p4));
                }
                
                if (hasStarted) {
                    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                }
                
                sink->Close();
                sink->Release();
                
                m_renderTarget->FillGeometry(geometry, brush);
                geometry->Release();
            }
        }
        
        brush->Release();
        nsvgDelete(image);
        
        m_renderTarget->SetTransform(originalTransform);
    }
    
    void DrawQuickTurnIconAnimated(RECT rect, bool enabled, float animationProgress) {
        const char* svgContent = 
            "<svg version='1.0' xmlns='http://www.w3.org/2000/svg' width='300' height='283' viewBox='0 0 300 283'>"
            "<g transform='translate(150,141.5) scale(0.48) translate(-150,-141.5)'>"
            "<g transform='translate(0.000000,283.000000) scale(0.100000,-0.100000)' fill='#000000' stroke='none'>"
            "<path d='M1037 2579 c-11 -6 -25 -26 -33 -44 -26 -62 -10 -85 196 -289 104 -104 190 -190 190 -192 0 -2 -21 -4 -47 -4 -27 0 -93 -7 -148 -15 -482 -68 -845 -279 -970 -562 -28 -64 -30 -76 -30 -193 l0 -125 43 -88 c141 -288 507 -484 1025 -547 157 -20 472 -8 637 23 428 82 738 270 862 524 l43 88 0 125 0 125 -48 98 c-78 159 -218 282 -437 385 -125 59 -167 64 -209 26 -71 -65 -42 -139 71 -184 174 -70 331 -194 393 -310 27 -49 30 -65 30 -140 0 -78 -3 -90 -34 -148 -107 -197 -412 -351 -803 -407 -131 -19 -403 -19 -533 0 -403 57 -705 212 -811 416 -25 47 -29 65 -29 139 0 104 21 153 102 241 126 136 332 234 616 293 71 15 131 26 133 23 2 -2 -48 -55 -112 -118 -132 -130 -152 -161 -130 -213 18 -43 51 -66 96 -66 32 0 54 19 302 268 l268 267 0 45 0 45 -263 262 -262 263 -45 0 c-25 0 -53 -5 -63 -11z m335 -276 c137 -137 251 -258 254 -269 3 -12 -1 -35 -10 -50 -16 -31 -444 -460 -486 -488 -32 -21 -46 -20 -70 4 -39 39 -28 63 95 190 63 65 115 125 115 133 0 7 -9 22 -20 32 -24 22 -87 17 -234 -21 -462 -116 -729 -381 -652 -646 64 -220 356 -407 766 -490 102 -20 143 -23 370 -23 227 0 267 3 370 23 374 76 645 233 743 431 28 57 32 76 32 146 0 99 -20 155 -86 237 -70 88 -168 161 -309 231 -118 58 -125 64 -128 94 -2 25 2 35 21 48 31 20 55 14 164 -35 217 -100 371 -244 437 -410 24 -60 33 -205 16 -267 -83 -311 -489 -554 -1022 -612 -125 -14 -351 -14 -476 0 -475 52 -852 249 -984 515 -39 80 -43 93 -46 183 -6 149 36 255 150 376 178 191 503 325 885 366 94 10 116 16 134 35 18 19 19 25 8 46 -6 12 -92 104 -190 204 -186 188 -198 206 -162 246 38 42 55 30 315 -229z'/>"
            "</g>"
            "</g>"
            "</svg>";
        
        DrawSvgWithNanoSVG(svgContent, rect, animationProgress);
    }
    
    void DrawGoldenTongueIconAnimated(RECT rect, bool enabled, float animationProgress) {
        const char* svgContent = 
            "<svg version='1.0' xmlns='http://www.w3.org/2000/svg' width='600.000000pt' height='400.000000pt' viewBox='0 0 600.000000 400.000000' preserveAspectRatio='xMidYMid meet'>"
            "<g transform='translate(300,200) scale(1.6) translate(-300,-200)'>"
            "<g transform='translate(0.000000,400.000000) scale(0.100000,-0.100000)' fill='#000000' stroke='#000000' stroke-width='24' stroke-linecap='round' stroke-linejoin='round'>"
            "<path d='M3855 2737 c-269 -143 -339 -177 -363 -177 -15 0 -76 16 -135 35 -229 73 -256 72 -403 -19 -123 -75 -149 -98 -196 -177 -86 -147 -47 -239 102 -239 60 0 96 17 157 73 l45 41 34 -19 c90 -54 364 -337 403 -419 26 -52 18 -81 -24 -92 -40 -10 -72 3 -138 59 -99 84 -135 109 -147 102 -22 -14 -6 -37 90 -127 105 -97 110 -108 72 -150 -28 -32 -74 -22 -142 30 -136 104 -140 106 -152 94 -16 -16 7 -48 87 -118 36 -33 65 -62 65 -66 0 -15 -45 -48 -66 -48 -12 0 -58 25 -102 55 -76 52 -80 53 -96 36 -16 -17 -14 -21 35 -64 44 -39 50 -49 39 -62 -20 -24 -52 -18 -105 19 l-48 34 -38 -20 c-21 -11 -46 -18 -56 -16 -14 3 4 24 71 89 76 75 87 90 81 110 -8 27 -32 49 -53 49 -7 0 -37 -24 -65 -54 -68 -72 -145 -136 -162 -136 -19 0 -45 28 -39 43 2 7 47 50 99 95 97 84 110 106 79 136 -29 30 -55 17 -155 -74 -53 -49 -104 -90 -112 -90 -8 0 -20 7 -27 15 -17 20 0 40 96 120 95 79 101 88 80 121 -30 44 -62 33 -159 -57 -87 -81 -109 -90 -124 -53 -6 16 18 43 100 111 32 27 35 58 8 73 -29 15 -35 13 -85 -25 -26 -19 -50 -35 -55 -35 -5 0 -51 28 -103 63 -52 34 -164 104 -250 156 l-155 94 48 136 c79 226 96 266 110 263 8 -2 104 -44 214 -94 136 -62 216 -93 249 -96 27 -3 102 3 166 13 111 17 118 20 158 57 23 21 42 41 42 44 0 5 -12 4 -203 -22 -87 -12 -127 -14 -155 -6 -20 5 -133 53 -250 106 -117 53 -226 96 -241 96 -49 0 -48 4 -161 -313 -63 -174 -70 -202 -60 -226 8 -20 73 -64 273 -184 l262 -157 0 -44 c1 -49 19 -77 65 -96 19 -8 31 -22 35 -41 9 -41 40 -76 80 -89 25 -8 39 -21 49 -45 18 -42 57 -68 103 -69 27 -1 44 -10 68 -33 35 -35 68 -41 122 -23 25 9 38 7 78 -15 67 -35 132 -34 176 2 24 20 41 26 68 24 49 -3 84 12 117 49 16 17 37 30 50 30 13 0 42 7 64 16 32 13 48 29 71 69 23 41 36 54 60 59 83 16 120 82 96 166 -14 49 -14 51 7 62 114 59 610 366 623 384 15 24 12 32 -92 247 -108 223 -135 267 -164 267 -9 0 -61 -24 -116 -53z m192 -231 l92 -185 -300 -181 c-165 -100 -303 -180 -307 -178 -5 2 -49 48 -98 103 -50 55 -123 127 -162 160 -40 32 -72 62 -72 66 0 4 14 12 31 18 51 18 30 36 -42 36 -35 1 -75 6 -89 12 -37 15 -78 -1 -130 -51 -25 -24 -55 -49 -67 -55 -30 -15 -75 -14 -99 3 -19 15 -19 16 6 62 49 94 77 122 182 184 57 33 117 62 133 65 20 4 78 -9 180 -40 208 -65 208 -65 450 64 105 55 192 101 195 101 3 0 46 -83 97 -184z'/>"
            "</g>"
            "</g>"
            "</svg>";
        
        DrawSvgWithNanoSVG(svgContent, rect, animationProgress);
    }
    
    void DrawMapCastIconAnimated(RECT rect, bool enabled, float animationProgress) {
        float rw = (float)(rect.right - rect.left);
        float rh = (float)(rect.bottom - rect.top);
        float vb = 1115.0f;
        float s = (min(rw, rh) / vb) * 0.5f;
        float ox = rect.left + (rw - vb * s) * 0.5f;
        float oy = rect.top + (rh - vb * s) * 0.5f;
        auto TP = [&](float x, float y) { return D2D1::Point2F(ox + x * s - 3.0f, oy + y * s); };
        auto createPathFromSvg = [&](const char* d, ID2D1PathGeometry** outGeom) -> bool {
            *outGeom = nullptr;
            ID2D1PathGeometry* g = nullptr;
            HRESULT hrLocal = m_d2dFactory->CreatePathGeometry(&g);
            if (FAILED(hrLocal) || !g) return false;
            ID2D1GeometrySink* sinkLocal = nullptr;
            hrLocal = g->Open(&sinkLocal);
            if (FAILED(hrLocal) || !sinkLocal) { if (g) g->Release(); return false; }
            sinkLocal->SetFillMode(D2D1_FILL_MODE_WINDING);
            *outGeom = g;
            auto parseTo = [&](ID2D1GeometrySink* targetSink, const char* pathD, auto TPfn) {
                const char* p = pathD;
                auto skip = [&]() { while (*p && (*p==' '||*p=='\t'||*p=='\n'||*p==',')) ++p; };
                auto isnum = [&](char c){ return (c=='-'||c=='+'||c=='.'||(c>='0'&&c<='9')); };
                auto readf = [&]() -> float { skip(); char* e=nullptr; float v=(float)strtod(p,&e); p=e; return v; };
                float cx=0, cy=0, sx=0, sy=0; char cmd=0;
                while (*p) {
                    skip(); if (!*p) break; char c=*p;
                    if ((c>='A'&&c<='Z')||(c>='a'&&c<='z')) { cmd=c; ++p; }
                    else if (!cmd) { ++p; continue; }
                    switch(cmd) {
                        case 'M': {
                            float x=readf(), y=readf(); cx=sx=x; cy=sy=y;
                            targetSink->BeginFigure(TPfn(cx,cy), D2D1_FIGURE_BEGIN_FILLED);
                            while (isnum(*p)) { x=readf(); y=readf(); cx=x; cy=y; targetSink->AddLine(TPfn(cx,cy)); }
                            break;
                        }
                        case 'm': {
                            float dx=readf(), dy=readf(); cx+=dx; cy+=dy; sx=cx; sy=cy;
                            targetSink->BeginFigure(TPfn(cx,cy), D2D1_FIGURE_BEGIN_FILLED);
                            while (isnum(*p)) { dx=readf(); dy=readf(); cx+=dx; cy+=dy; targetSink->AddLine(TPfn(cx,cy)); }
                            break;
                        }
                        case 'L': { while (isnum(*p)) { float x=readf(), y=readf(); cx=x; cy=y; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'l': { while (isnum(*p)) { float dx=readf(), dy=readf(); cx+=dx; cy+=dy; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'H': { while (isnum(*p)) { float x=readf(); cx=x; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'h': { while (isnum(*p)) { float dx=readf(); cx+=dx; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'V': { while (isnum(*p)) { float y=readf(); cy=y; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'v': { while (isnum(*p)) { float dy=readf(); cy+=dy; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'C': { while (isnum(*p)) { float x1=readf(), y1=readf(), x2=readf(), y2=readf(), x=readf(), y=readf(); D2D1_BEZIER_SEGMENT b={TPfn(x1,y1),TPfn(x2,y2),TPfn(x,y)}; targetSink->AddBezier(b); cx=x; cy=y; } break; }
                        case 'c': { while (isnum(*p)) { float dx1=readf(), dy1=readf(), dx2=readf(), dy2=readf(), dx=readf(), dy=readf(); D2D1_BEZIER_SEGMENT b={TPfn(cx+dx1,cy+dy1),TPfn(cx+dx2,cy+dy2),TPfn(cx+dx,cy+dy)}; targetSink->AddBezier(b); cx+=dx; cy+=dy; } break; }
                        case 'Z': case 'z': { targetSink->EndFigure(D2D1_FIGURE_END_CLOSED); cx=sx; cy=sy; cmd=0; break; }
                        default: { ++p; break; }
                    }
                }
            };
            parseTo(sinkLocal, d, TP);
            sinkLocal->Close();
            sinkLocal->Release();
            return true;
        };

        ID2D1PathGeometry* g1 = nullptr;
        ID2D1PathGeometry* g2 = nullptr;
        ID2D1PathGeometry* g3 = nullptr;
        ID2D1PathGeometry* g4 = nullptr;

        const char* d1 = "M212 1115 c-62 -27 -64 -37 -61 -519 2 -480 2 -486 63 -511 52 -22 1000 -22 1052 0 60 25 61 31 63 511 2 368 0 437 -13 468 -23 54 -74 71 -179 57 -5 -1 -7 -35 -6 -76 l4 -76 -395 0 -395 0 4 72 c2 52 -1 74 -11 80 -20 13 -89 10 -126 -6z m98 -436 l0 -414 -30 3 c-16 2 -44 -4 -62 -12 -17 -9 -32 -15 -32 -14 -6 16 11 828 17 834 7 7 97 23 105 18 1 0 2 -187 2 -415z m967 397 c6 -6 22 -818 17 -834 0 -1 -15 5 -32 14 -18 8 -46 14 -62 12 l-30 -3 0 413 c0 226 0 413 0 415 0 6 100 -10 107 -17z m-144 -493 l2 -348 68 -5 c73 -5 87 -15 87 -62 0 -60 18 -58 -550 -58 -568 0 -550 -2 -550 58 0 47 14 57 87 62 l68 5 2 345 c1 190 3 347 3 350 0 3 176 4 391 3 l390 -3 2 -347z";
        const char* d2 = "M589 828 c-27 -21 -69 -91 -69 -113 0 -11 7 -31 16 -44 22 -30 39 -13 23 23 -11 24 -9 31 18 69 25 33 37 42 60 41 15 -1 53 -3 83 -5 30 -1 93 1 140 5 69 7 97 5 142 -8 70 -20 79 -40 47 -101 -21 -40 -22 -46 -9 -99 17 -72 6 -88 -55 -79 -39 5 -44 3 -66 -26 -30 -41 -80 -43 -120 -5 -19 18 -34 24 -46 20 -15 -6 -12 -12 22 -41 65 -55 125 -55 161 0 15 23 22 25 54 20 28 -5 42 -1 63 15 30 24 34 50 16 111 -10 33 -9 42 10 72 65 106 -25 167 -224 152 -130 -9 -121 -9 -190 -1 -44 6 -64 4 -76 -6z";
        const char* d3 = "M478 588 c-63 -29 -68 -33 -83 -79 -9 -27 -14 -57 -10 -68 3 -11 29 -34 58 -52 63 -38 67 -43 67 -94 0 -62 21 -85 79 -85 47 0 50 2 88 53 64 85 64 82 41 182 -11 50 -25 96 -31 103 -12 15 -89 56 -122 65 -12 3 -48 -7 -87 -25z m205 -130 c10 -40 17 -82 17 -92 0 -11 -18 -43 -40 -73 -35 -46 -45 -53 -74 -53 -37 0 -46 12 -46 65 0 51 -8 63 -71 106 -33 22 -59 43 -59 46 0 4 6 22 13 41 10 29 23 40 72 63 l60 27 55 -28 56 -28 17 -74z";
        const char* d4 = "M852 347 c-11 -12 -25 -33 -30 -47 -12 -31 2 -91 24 -107 56 -41 174 25 174 98 0 44 -28 66 -91 73 -49 5 -58 3 -77 -17z m130 -31 c22 -32 -17 -89 -70 -103 -53 -13 -83 43 -51 96 18 29 21 30 66 24 27 -3 51 -11 55 -17z";

        if (!createPathFromSvg(d1, &g1)) return;
        if (!createPathFromSvg(d2, &g2)) { g1->Release(); return; }
        if (!createPathFromSvg(d3, &g3)) { g1->Release(); g2->Release(); return; }
        if (!createPathFromSvg(d4, &g4)) { g1->Release(); g2->Release(); g3->Release(); return; }

        ID2D1PathGeometry* u12 = nullptr; m_d2dFactory->CreatePathGeometry(&u12);
        if (!u12) { g1->Release(); g2->Release(); g3->Release(); g4->Release(); return; }
        ID2D1GeometrySink* sinkU12 = nullptr; u12->Open(&sinkU12);
        if (!sinkU12) { u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); return; }
        sinkU12->SetFillMode(D2D1_FILL_MODE_WINDING);
        g1->CombineWithGeometry(g2, D2D1_COMBINE_MODE_UNION, nullptr, sinkU12);
        sinkU12->Close(); sinkU12->Release();

        ID2D1PathGeometry* u123 = nullptr; m_d2dFactory->CreatePathGeometry(&u123);
        if (!u123) { u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); return; }
        ID2D1GeometrySink* sinkU123 = nullptr; u123->Open(&sinkU123);
        if (!sinkU123) { u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); return; }
        sinkU123->SetFillMode(D2D1_FILL_MODE_WINDING);
        u12->CombineWithGeometry(g3, D2D1_COMBINE_MODE_UNION, nullptr, sinkU123);
        sinkU123->Close(); sinkU123->Release();

        ID2D1PathGeometry* pathGeometry = nullptr; m_d2dFactory->CreatePathGeometry(&pathGeometry);
        if (!pathGeometry) { u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); return; }
        ID2D1GeometrySink* sinkFinal = nullptr; pathGeometry->Open(&sinkFinal);
        if (!sinkFinal) { pathGeometry->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); return; }
        sinkFinal->SetFillMode(D2D1_FILL_MODE_WINDING);
        u123->CombineWithGeometry(g4, D2D1_COMBINE_MODE_UNION, nullptr, sinkFinal);
        sinkFinal->Close(); sinkFinal->Release();

        g1->Release(); g2->Release(); g3->Release(); g4->Release();
        u12->Release(); u123->Release();

        float grayWeight = 1.0f - animationProgress;
        float darkWeight = animationProgress;
        float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        ID2D1SolidColorBrush* iconBrush = nullptr;
        D2D1_COLOR_F iconColor = D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f);
        m_renderTarget->CreateSolidColorBrush(iconColor, &iconBrush);
        m_renderTarget->FillGeometry(pathGeometry, iconBrush);
        iconBrush->Release();
        pathGeometry->Release();
    }
    
    void DrawRollCastIconAnimated(RECT rect, bool enabled, float animationProgress) {
        float rw = (float)(rect.right - rect.left);
        float rh = (float)(rect.bottom - rect.top);
        float vbW = 970.0f;
        float vbH = 800.0f;
        float s = (min(rw, rh) / max(vbW, vbH)) * 0.95f;
        float ox = rect.left + (rw - vbW * s) * 0.5f;
        float oy = rect.top + (rh - vbH * s) * 0.5f;
        auto TP = [&](float x, float y) { return D2D1::Point2F(ox + x * s, oy + (vbH - y) * s); };
        auto createPathFromSvg = [&](const char* d, ID2D1PathGeometry** outGeom) -> bool {
            *outGeom = nullptr;
            ID2D1PathGeometry* g = nullptr;
            HRESULT hrLocal = m_d2dFactory->CreatePathGeometry(&g);
            if (FAILED(hrLocal) || !g) return false;
            ID2D1GeometrySink* sinkLocal = nullptr;
            hrLocal = g->Open(&sinkLocal);
            if (FAILED(hrLocal) || !sinkLocal) { if (g) g->Release(); return false; }
            sinkLocal->SetFillMode(D2D1_FILL_MODE_WINDING);
            *outGeom = g;
            auto parseTo = [&](ID2D1GeometrySink* targetSink, const char* pathD, auto TPfn) {
                const char* p = pathD;
                auto skip = [&]() { while (*p && (*p==' '||*p=='\t'||*p=='\n'||*p==',')) ++p; };
                auto isnum = [&](char c){ return (c=='-'||c=='+'||c=='.'||(c>='0'&&c<='9')); };
                auto readf = [&]() -> float { skip(); char* e=nullptr; float v=(float)strtod(p,&e); p=e; return v; };
                float cx=0, cy=0, sx=0, sy=0; char cmd=0;
                while (*p) {
                    skip(); if (!*p) break; char c=*p;
                    if ((c>='A'&&c<='Z')||(c>='a'&&c<='z')) { cmd=c; ++p; }
                    else if (!cmd) { ++p; continue; }
                    switch(cmd) {
                        case 'M': {
                            float x=readf(), y=readf(); cx=sx=x; cy=sy=y;
                            targetSink->BeginFigure(TPfn(cx,cy), D2D1_FIGURE_BEGIN_FILLED);
                            while (isnum(*p)) { x=readf(); y=readf(); cx=x; cy=y; targetSink->AddLine(TPfn(cx,cy)); }
                            break;
                        }
                        case 'm': {
                            float dx=readf(), dy=readf(); cx+=dx; cy+=dy; sx=cx; sy=cy;
                            targetSink->BeginFigure(TPfn(cx,cy), D2D1_FIGURE_BEGIN_FILLED);
                            while (isnum(*p)) { dx=readf(); dy=readf(); cx+=dx; cy+=dy; targetSink->AddLine(TPfn(cx,cy)); }
                            break;
                        }
                        case 'L': { while (isnum(*p)) { float x=readf(), y=readf(); cx=x; cy=y; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'l': { while (isnum(*p)) { float dx=readf(), dy=readf(); cx+=dx; cy+=dy; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'H': { while (isnum(*p)) { float x=readf(); cx=x; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'h': { while (isnum(*p)) { float dx=readf(); cx+=dx; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'V': { while (isnum(*p)) { float y=readf(); cy=y; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'v': { while (isnum(*p)) { float dy=readf(); cy+=dy; targetSink->AddLine(TPfn(cx,cy)); } break; }
                        case 'C': { while (isnum(*p)) { float x1=readf(), y1=readf(), x2=readf(), y2=readf(), x=readf(), y=readf(); D2D1_BEZIER_SEGMENT b={TPfn(x1,y1),TPfn(x2,y2),TPfn(x,y)}; targetSink->AddBezier(b); cx=x; cy=y; } break; }
                        case 'c': { while (isnum(*p)) { float dx1=readf(), dy1=readf(), dx2=readf(), dy2=readf(), dx=readf(), dy=readf(); D2D1_BEZIER_SEGMENT b={TPfn(cx+dx1,cy+dy1),TPfn(cx+dx2,cy+dy2),TPfn(cx+dx,cy+dy)}; targetSink->AddBezier(b); cx+=dx; cy+=dy; } break; }
                        case 'Z': case 'z': { targetSink->EndFigure(D2D1_FIGURE_END_CLOSED); cx=sx; cy=sy; cmd=0; break; }
                        default: { ++p; break; }
                    }
                }
            };
            parseTo(sinkLocal, d, TP);
            sinkLocal->Close();
            sinkLocal->Release();
            return true;
        };

        ID2D1PathGeometry* g1 = nullptr;
        ID2D1PathGeometry* g2 = nullptr;
        ID2D1PathGeometry* g3 = nullptr;
        ID2D1PathGeometry* g4 = nullptr;
        ID2D1PathGeometry* g5 = nullptr;

        const char* d1 = "M130 720 c-9 -2 -20 -24 -24 -47 c-8 -36 -2 -46 35 -81 c23 -22 58 -69 76 -105 c19 -35 43 -71 54 -80 c29 -22 76 -22 76 1 c0 35 23 19 36 -26 c19 -62 17 -110 -3 -130 c-19 -20 -33 -12 -25 12 c7 15 -52 37 -66 24 c-19 -19 -8 -62 21 -86 c52 -45 103 -31 126 36 c5 12 15 19 21 14 c14 -9 7 57 -11 89 c-7 11 -12 25 -13 32 c-1 8 -5 14 -9 19 c-4 4 -2 -20 2 -50 c11 -74 1 -113 -38 -137 c-29 -20 -32 -20 -60 8 c-33 31 -42 75 -13 65 c10 -4 20 -17 23 -31 c8 -27 36 -31 57 -4 c26 33 7 198 -24 198 c-7 0 -15 11 -19 24 c-4 13 -13 24 -20 24 c-8 0 -8 -9 1 -23 c17 -34 15 -44 -7 -25 c-23 20 -23 41 -1 59 c22 19 68 -2 68 -31 c0 -10 13 -24 31 -32 c32 -14 50 -49 44 -86 c-3 -13 3 -20 10 -14 c8 5 13 3 13 -5 c0 -8 13 -26 31 -43 c32 -33 96 -41 140 -19 c22 12 22 16 4 50 c-17 33 -17 38 -2 54 c15 16 20 15 38 -3 c42 -43 5 -138 -53 -138 c-13 0 -24 -5 -24 -11 c0 -13 94 17 122 38 c10 9 24 32 32 51 c17 49 -4 74 -65 83 c-65 9 -86 -16 -66 -70 c17 -45 17 -44 -35 -50 c-53 -8 -89 18 -108 74 c-9 25 -20 50 -25 58 c-16 17 -13 30 8 30 c10 0 18 9 18 18 c0 10 9 18 18 18 c10 0 18 11 18 23 c0 13 7 25 15 27 c25 9 0 44 -59 78 c-45 25 -74 34 -132 36 c-64 2 -81 8 -129 45 c-55 42 -70 47 -99 35 L130 720z m122 -73 c28 -23 44 -26 87 -20 c44 5 66 0 120 -25 c70 -34 108 -68 77 -68 c-10 0 -23 6 -32 14 c-10 10 -14 10 -14 1 c0 -8 9 -20 19 -28 c21 -17 24 -36 6 -36 c-7 0 -21 17 -31 37 c-11 21 -23 35 -25 32 c-4 -4 3 -23 12 -44 c22 -41 24 -61 9 -61 c-5 0 -19 21 -29 46 l-20 45 8 -47 c5 -36 4 -46 -10 -41 c-10 4 -20 16 -22 29 c-10 53 -48 69 -88 33 c-27 -24 -28 -62 -2 -90 c16 -17 16 -22 3 -22 c-32 2 -58 32 -96 107 c-21 40 -48 81 -61 89 c-29 17 -30 42 -5 78 c21 30 17 32 93 -31z m508 -293 c8 -23 -15 -82 -31 -82 c-4 0 -8 26 -8 59 c0 60 21 73 37 23z";
        const char* d2 = "M580 450 c-3 -4 -3 -15 -2 -27 c2 -11 3 -22 4 -26 c1 -4 10 -6 21 -6 c25 0 42 34 27 53 c-10 12 -44 15 -49 5z m40 -21 c2 -6 -4 -17 -13 -22 c-11 -5 -17 -5 -17 0 c0 4 4 8 9 8 c5 0 7 7 5 17 c-3 11 -1 15 5 13 c5 -2 11 -9 12 -16z";
        const char* d3 = "M520 360 c-6 -12 -6 -20 0 -26 c6 -6 9 -4 9 8 c0 28 24 30 48 4 c58 -66 -2 -155 -103 -155 c-31 0 -62 18 -92 52 c-13 15 -14 15 -10 -12 c8 -59 24 -100 47 -119 c29 -24 49 -21 52 8 c5 41 40 34 47 -10 c4 -30 -51 -47 -92 -28 c-30 14 -46 36 -56 76 c-4 15 -7 31 -8 35 c-1 4 -2 10 -3 14 c0 5 -4 8 -9 8 c-5 0 -5 -7 0 -21 c5 -12 6 -24 4 -28 c-7 -12 27 -71 49 -86 c12 -7 37 -15 57 -18 c44 -5 68 13 68 47 c0 53 -52 68 -68 17 l-9 -28 -23 20 c-20 18 -49 85 -40 94 c2 2 14 -5 28 -16 c32 -23 100 -24 139 -1 c37 22 63 75 55 112 c-13 58 -68 89 -90 51z";
        const char* d4 = "M580 195 c-12 -13 -12 -17 -4 -28 c20 -24 49 -11 49 24 c0 17 -30 20 -45 4z m37 -6 c0 -4 -4 -12 -12 -16 c-13 -11 -26 2 -17 15 c6 10 28 11 28 1z";
        const char* d5 = "";

        if (!createPathFromSvg(d1, &g1)) return;
        if (!createPathFromSvg(d2, &g2)) { g1->Release(); return; }
        if (!createPathFromSvg(d3, &g3)) { g1->Release(); g2->Release(); return; }
        if (!createPathFromSvg(d4, &g4)) { g1->Release(); g2->Release(); g3->Release(); return; }
        if (!createPathFromSvg(d5, &g5)) { g1->Release(); g2->Release(); g3->Release(); g4->Release(); return; }

        ID2D1PathGeometry* u12 = nullptr; m_d2dFactory->CreatePathGeometry(&u12);
        if (!u12) { g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); return; }
        ID2D1GeometrySink* sinkU12 = nullptr; u12->Open(&sinkU12);
        if (!sinkU12) { u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); return; }
        sinkU12->SetFillMode(D2D1_FILL_MODE_WINDING);
        g1->CombineWithGeometry(g2, D2D1_COMBINE_MODE_UNION, nullptr, sinkU12);
        sinkU12->Close(); sinkU12->Release();

        ID2D1PathGeometry* u123 = nullptr; m_d2dFactory->CreatePathGeometry(&u123);
        if (!u123) { u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); return; }
        ID2D1GeometrySink* sinkU123 = nullptr; u123->Open(&sinkU123);
        if (!sinkU123) { u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); return; }
        sinkU123->SetFillMode(D2D1_FILL_MODE_WINDING);
        u12->CombineWithGeometry(g3, D2D1_COMBINE_MODE_UNION, nullptr, sinkU123);
        sinkU123->Close(); sinkU123->Release();

        ID2D1PathGeometry* u1234 = nullptr; m_d2dFactory->CreatePathGeometry(&u1234);
        if (!u1234) { u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); return; }
        ID2D1GeometrySink* sinkU1234 = nullptr; u1234->Open(&sinkU1234);
        if (!sinkU1234) { u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); return; }
        sinkU1234->SetFillMode(D2D1_FILL_MODE_WINDING);
        u123->CombineWithGeometry(g4, D2D1_COMBINE_MODE_UNION, nullptr, sinkU1234);
        sinkU1234->Close(); sinkU1234->Release();

        ID2D1PathGeometry* pathGeometry = nullptr; m_d2dFactory->CreatePathGeometry(&pathGeometry);
        if (!pathGeometry) { u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); return; }
        ID2D1GeometrySink* sinkFinal = nullptr; pathGeometry->Open(&sinkFinal);
        if (!sinkFinal) { pathGeometry->Release(); u1234->Release(); u123->Release(); u12->Release(); g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); return; }
        sinkFinal->SetFillMode(D2D1_FILL_MODE_WINDING);
        u1234->CombineWithGeometry(g5, D2D1_COMBINE_MODE_UNION, nullptr, sinkFinal);
        sinkFinal->Close(); sinkFinal->Release();

        g1->Release(); g2->Release(); g3->Release(); g4->Release(); g5->Release(); u12->Release(); u123->Release(); u1234->Release();

        float grayWeight = 1.0f - animationProgress;
        float darkWeight = animationProgress;
        float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
        ID2D1SolidColorBrush* iconBrush = nullptr;
        D2D1_COLOR_F iconColor = D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f);
        m_renderTarget->CreateSolidColorBrush(iconColor, &iconBrush);
        m_renderTarget->FillGeometry(pathGeometry, iconBrush);
        iconBrush->Release();
        pathGeometry->Release();
    }
    
    void RenderModCards() {
		m_renderTarget->SetTransform(GetBaseRenderTransform());
        if (m_showProfile) {
            m_renderTarget->PushAxisAlignedClip(D2D1::RectF(0.0f, 60.0f, kContentWidth, kContentHeight), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            m_renderTarget->SetTransform(GetBaseRenderTransform());
            
            float centerX = kContentWidth / 2.0f;
            float centerY = (60.0f + kContentHeight) / 2.0f;
            D2D1_RECT_F textRect = D2D1::RectF(0.0f, centerY - 20.0f, kContentWidth, centerY + 20.0f);
            m_renderTarget->DrawText(L"Coming Soon..", 13, m_titleFormat, textRect, m_textBrush);
            
            m_renderTarget->PopAxisAlignedClip();
        } else if (m_showSettings) {
            // Clip content to prevent drawing above title bar (y=60) BEFORE transform
            m_renderTarget->PushAxisAlignedClip(D2D1::RectF(0.0f, 60.0f, kContentWidth, kContentHeight), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            
			// Apply scrolling transform for all settings content
			m_renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0, -m_settingsScrollOffset) * GetBaseRenderTransform());
            
            // Dynamic positioning for settings sections based on search results
            int currentY = 84; // Starting Y position
            
            // Blur settings section
            if (SettingsMatchesSearch(L"Background Blur")) {
                int yPos = currentY;
                RECT blurArea = {60 + 40, yPos, 60 + 40 + 600, yPos + 140};
                m_renderTarget->DrawText(L"Background Blur", 15, m_titleFormat, D2D1::RectF(blurArea.left + 16, blurArea.top + 12, blurArea.right, blurArea.top + 40), m_textBrush);
                
                int trackLeft = blurArea.left + 40;
                int trackRight = blurArea.right - 40;
                int trackY = blurArea.top + 80;
                m_sliderTrackRect = {trackLeft, trackY, trackRight, trackY + 4};
                ID2D1SolidColorBrush* blurTrackBrush = nullptr;
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &blurTrackBrush);
                m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_sliderTrackRect.left, m_sliderTrackRect.top, m_sliderTrackRect.right, m_sliderTrackRect.bottom), 2, 2), blurTrackBrush);
                blurTrackBrush->Release();
                ID2D1SolidColorBrush* blurTrackBorder = nullptr;
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &blurTrackBorder);
                m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_sliderTrackRect.left, m_sliderTrackRect.top, m_sliderTrackRect.right, m_sliderTrackRect.bottom), 2, 2), blurTrackBorder, 1.0f);
                blurTrackBorder->Release();
                int knobX = trackLeft + (int)((trackRight - trackLeft) * m_blurSlider + 0.5f);
                RECT knob = {knobX - 8, trackY - 8, knobX + 8, trackY + 8};
                m_sliderKnobRect = knob;
                m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F((float)knobX, (float)trackY), 8.0f, 8.0f), m_whiteBrush);
                wchar_t label[64];
                swprintf(label, 64, L"Blur Intensity: %.2f", m_blurSlider);
                m_renderTarget->DrawText(label, (UINT32)wcslen(label), m_titleFormat, D2D1::RectF(blurArea.left + 16, trackY + 20, blurArea.right, trackY + 44), m_textBrush);

                // Separator line
                ID2D1SolidColorBrush* separatorBrush = nullptr;
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
                m_renderTarget->DrawLine(D2D1::Point2F(blurArea.left, blurArea.bottom + 10), D2D1::Point2F(blurArea.right, blurArea.bottom + 10), separatorBrush, 1.0f);
                separatorBrush->Release();

                currentY += 140; // Move to next section position
            }
            
            // Hotkeys settings section
            if (SettingsMatchesSearch(L"Hotkeys")) {
                int yPos = currentY + 20;
                RECT hotkeysArea = {60 + 40, yPos, 60 + 40 + 600, yPos + 260};

            m_renderTarget->DrawText(L"Hotkeys", 7, m_titleFormat, D2D1::RectF(hotkeysArea.left + 16, hotkeysArea.top + 12, hotkeysArea.right, hotkeysArea.top + 40), m_textBrush);

			int btnW = 60;
			int btnH = 24;

			// Roll Cast section
			RECT rollCastLabelRect = {hotkeysArea.left + 16, hotkeysArea.top + 40, hotkeysArea.left + 140, hotkeysArea.top + 64};
			m_renderTarget->DrawText(L"Roll Cast", 9, m_titleFormat, D2D1::RectF((FLOAT)rollCastLabelRect.left, (FLOAT)rollCastLabelRect.top, (FLOAT)rollCastLabelRect.right, (FLOAT)rollCastLabelRect.bottom), m_textBrush);
			int rollCastBtnLeft = rollCastLabelRect.right + 12;
			int rollCastRowCenterY = (rollCastLabelRect.top + rollCastLabelRect.bottom) / 2;
			int rollCastBtnTop = rollCastRowCenterY - btnH / 2;
			m_rollCastSetButtonRect = {rollCastBtnLeft, rollCastBtnTop, rollCastBtnLeft + btnW, rollCastBtnTop + btnH};
			ID2D1SolidColorBrush* rollCastBtnBg = nullptr;
			m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &rollCastBtnBg);
			m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollCastSetButtonRect.left, (FLOAT)m_rollCastSetButtonRect.top, (FLOAT)m_rollCastSetButtonRect.right, (FLOAT)m_rollCastSetButtonRect.bottom), 6, 6), rollCastBtnBg);
			rollCastBtnBg->Release();
			ID2D1SolidColorBrush* rollCastBtnBorder = nullptr;
			m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &rollCastBtnBorder);
			m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollCastSetButtonRect.left, (FLOAT)m_rollCastSetButtonRect.top, (FLOAT)m_rollCastSetButtonRect.right, (FLOAT)m_rollCastSetButtonRect.bottom), 6, 6), rollCastBtnBorder, 1.0f);
			rollCastBtnBorder->Release();
			if (!m_rollCastKey.empty()) {
				ID2D1SolidColorBrush* btnTextBrush = nullptr;
				if (m_keyCaptureActive && m_rollCastCapturing) {
					m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
				} else {
					btnTextBrush = m_textBrush;
				}
				m_renderTarget->DrawText(m_rollCastKey.c_str(), (UINT32)m_rollCastKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_rollCastSetButtonRect.left, (FLOAT)m_rollCastSetButtonRect.top, (FLOAT)m_rollCastSetButtonRect.right, (FLOAT)m_rollCastSetButtonRect.bottom), btnTextBrush);
				if (m_keyCaptureActive && m_rollCastCapturing && btnTextBrush != m_textBrush) {
					btnTextBrush->Release();
				}
			}
			// Zoom section
			RECT zoomLabelRect = {hotkeysArea.left + 16, hotkeysArea.top + 68, hotkeysArea.left + 140, hotkeysArea.top + 92};
			m_renderTarget->DrawText(L"Zoom", 4, m_titleFormat, D2D1::RectF((FLOAT)zoomLabelRect.left, (FLOAT)zoomLabelRect.top, (FLOAT)zoomLabelRect.right, (FLOAT)zoomLabelRect.bottom), m_textBrush);
			int zoomBtnLeft = zoomLabelRect.right + 12;
			int zoomRowCenterY = (zoomLabelRect.top + zoomLabelRect.bottom) / 2;
			int zoomBtnTop = zoomRowCenterY - btnH / 2;
			m_zoomSetButtonRect = {zoomBtnLeft, zoomBtnTop, zoomBtnLeft + btnW, zoomBtnTop + btnH};
			ID2D1SolidColorBrush* zoomBtnBg = nullptr;
			m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &zoomBtnBg);
			m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_zoomSetButtonRect.left, (FLOAT)m_zoomSetButtonRect.top, (FLOAT)m_zoomSetButtonRect.right, (FLOAT)m_zoomSetButtonRect.bottom), 6, 6), zoomBtnBg);
			zoomBtnBg->Release();
			ID2D1SolidColorBrush* zoomBtnBorder = nullptr;
			m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &zoomBtnBorder);
			m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_zoomSetButtonRect.left, (FLOAT)m_zoomSetButtonRect.top, (FLOAT)m_zoomSetButtonRect.right, (FLOAT)m_zoomSetButtonRect.bottom), 6, 6), zoomBtnBorder, 1.0f);
			zoomBtnBorder->Release();
			if (!m_zoomKey.empty()) {
				ID2D1SolidColorBrush* btnTextBrush = nullptr;
				if (m_keyCaptureActive && m_zoomCapturing) {
					m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
				} else {
					btnTextBrush = m_textBrush;
				}
				m_renderTarget->DrawText(m_zoomKey.c_str(), (UINT32)m_zoomKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_zoomSetButtonRect.left, (FLOAT)m_zoomSetButtonRect.top, (FLOAT)m_zoomSetButtonRect.right, (FLOAT)m_zoomSetButtonRect.bottom), btnTextBrush);
				if (m_keyCaptureActive && m_zoomCapturing && btnTextBrush != m_textBrush) {
					btnTextBrush->Release();
				}
			}
		RECT dodgeLabelRect = {hotkeysArea.left + 16, hotkeysArea.top + 96, hotkeysArea.left + 140, hotkeysArea.top + 120};
		m_renderTarget->DrawText(L"Dodge", 5, m_titleFormat, D2D1::RectF((FLOAT)dodgeLabelRect.left, (FLOAT)dodgeLabelRect.top, (FLOAT)dodgeLabelRect.right, (FLOAT)dodgeLabelRect.bottom), m_textBrush);
		int dodgeBtnLeft = dodgeLabelRect.right + 12;
		int dodgeRowCenterY = (dodgeLabelRect.top + dodgeLabelRect.bottom) / 2;
		int dodgeBtnTop = dodgeRowCenterY - btnH / 2;
		m_dodgeSetButtonRect = {dodgeBtnLeft, dodgeBtnTop, dodgeBtnLeft + btnW, dodgeBtnTop + btnH};
		ID2D1SolidColorBrush* dodgeBtnBg = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &dodgeBtnBg);
		m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_dodgeSetButtonRect.left, (FLOAT)m_dodgeSetButtonRect.top, (FLOAT)m_dodgeSetButtonRect.right, (FLOAT)m_dodgeSetButtonRect.bottom), 6, 6), dodgeBtnBg);
		dodgeBtnBg->Release();
		ID2D1SolidColorBrush* dodgeBtnBorder = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &dodgeBtnBorder);
		m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_dodgeSetButtonRect.left, (FLOAT)m_dodgeSetButtonRect.top, (FLOAT)m_dodgeSetButtonRect.right, (FLOAT)m_dodgeSetButtonRect.bottom), 6, 6), dodgeBtnBorder, 1.0f);
		dodgeBtnBorder->Release();
		if (!m_dodgeKey.empty()) {
			ID2D1SolidColorBrush* btnTextBrush = nullptr;
			if (m_keyCaptureActive && m_dodgeCapturing) {
				m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			} else {
				btnTextBrush = m_textBrush;
			}
			m_renderTarget->DrawText(m_dodgeKey.c_str(), (UINT32)m_dodgeKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_dodgeSetButtonRect.left, (FLOAT)m_dodgeSetButtonRect.top, (FLOAT)m_dodgeSetButtonRect.right, (FLOAT)m_dodgeSetButtonRect.bottom), btnTextBrush);
			if (m_keyCaptureActive && m_dodgeCapturing && btnTextBrush != m_textBrush) {
				btnTextBrush->Release();
			}
		}
		
		RECT parryLabelRect = {hotkeysArea.left + 16, hotkeysArea.top + 124, hotkeysArea.left + 140, hotkeysArea.top + 148};
		m_renderTarget->DrawText(L"Block / Parry", 13, m_titleFormat, D2D1::RectF((FLOAT)parryLabelRect.left, (FLOAT)parryLabelRect.top, (FLOAT)parryLabelRect.right, (FLOAT)parryLabelRect.bottom), m_textBrush);
		int parryBtnLeft = parryLabelRect.right + 12;
		int parryRowCenterY = (parryLabelRect.top + parryLabelRect.bottom) / 2;
		int parryBtnTop = parryRowCenterY - btnH / 2;
		m_parrySetButtonRect = {parryBtnLeft, parryBtnTop, parryBtnLeft + btnW, parryBtnTop + btnH};
		ID2D1SolidColorBrush* parryBtnBg = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &parryBtnBg);
		m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_parrySetButtonRect.left, (FLOAT)m_parrySetButtonRect.top, (FLOAT)m_parrySetButtonRect.right, (FLOAT)m_parrySetButtonRect.bottom), 6, 6), parryBtnBg);
		parryBtnBg->Release();
		ID2D1SolidColorBrush* parryBtnBorder = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &parryBtnBorder);
		m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_parrySetButtonRect.left, (FLOAT)m_parrySetButtonRect.top, (FLOAT)m_parrySetButtonRect.right, (FLOAT)m_parrySetButtonRect.bottom), 6, 6), parryBtnBorder, 1.0f);
		parryBtnBorder->Release();
		if (!m_parryKey.empty()) {
			ID2D1SolidColorBrush* btnTextBrush = nullptr;
			if (m_keyCaptureActive && m_parryCapturing) {
				m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			} else {
				btnTextBrush = m_textBrush;
			}
			m_renderTarget->DrawText(m_parryKey.c_str(), (UINT32)m_parryKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_parrySetButtonRect.left, (FLOAT)m_parrySetButtonRect.top, (FLOAT)m_parrySetButtonRect.right, (FLOAT)m_parrySetButtonRect.bottom), btnTextBrush);
			if (m_keyCaptureActive && m_parryCapturing && btnTextBrush != m_textBrush) {
				btnTextBrush->Release();
			}
		}
		
		RECT openMapLabelRect = {hotkeysArea.left + 16, hotkeysArea.top + 152, hotkeysArea.left + 140, hotkeysArea.top + 176};
		m_renderTarget->DrawText(L"Open Map", 8, m_titleFormat, D2D1::RectF((FLOAT)openMapLabelRect.left, (FLOAT)openMapLabelRect.top, (FLOAT)openMapLabelRect.right, (FLOAT)openMapLabelRect.bottom), m_textBrush);
		int openMapBtnLeft = openMapLabelRect.right + 12;
		int openMapRowCenterY = (openMapLabelRect.top + openMapLabelRect.bottom) / 2;
		int openMapBtnTop = openMapRowCenterY - btnH / 2;
		m_openMapSetButtonRect = {openMapBtnLeft, openMapBtnTop, openMapBtnLeft + btnW, openMapBtnTop + btnH};
		ID2D1SolidColorBrush* openMapBtnBg = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &openMapBtnBg);
		m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_openMapSetButtonRect.left, (FLOAT)m_openMapSetButtonRect.top, (FLOAT)m_openMapSetButtonRect.right, (FLOAT)m_openMapSetButtonRect.bottom), 6, 6), openMapBtnBg);
		openMapBtnBg->Release();
		ID2D1SolidColorBrush* openMapBtnBorder = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &openMapBtnBorder);
		m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_openMapSetButtonRect.left, (FLOAT)m_openMapSetButtonRect.top, (FLOAT)m_openMapSetButtonRect.right, (FLOAT)m_openMapSetButtonRect.bottom), 6, 6), openMapBtnBorder, 1.0f);
		openMapBtnBorder->Release();
		if (!m_openMapKey.empty()) {
			ID2D1SolidColorBrush* btnTextBrush = nullptr;
			if (m_keyCaptureActive && m_openMapCapturing) {
				m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			} else {
				btnTextBrush = m_textBrush;
			}
			m_renderTarget->DrawText(m_openMapKey.c_str(), (UINT32)m_openMapKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_openMapSetButtonRect.left, (FLOAT)m_openMapSetButtonRect.top, (FLOAT)m_openMapSetButtonRect.right, (FLOAT)m_openMapSetButtonRect.bottom), btnTextBrush);
			if (m_keyCaptureActive && m_openMapCapturing && btnTextBrush != m_textBrush) {
				btnTextBrush->Release();
			}
		}
		
		RECT criticalAttackLabelRect = {hotkeysArea.left + 16, hotkeysArea.top + 180, hotkeysArea.left + 140, hotkeysArea.top + 204};
		m_renderTarget->DrawText(L"Critical Attack", 15, m_titleFormat, D2D1::RectF((FLOAT)criticalAttackLabelRect.left, (FLOAT)criticalAttackLabelRect.top, (FLOAT)criticalAttackLabelRect.right, (FLOAT)criticalAttackLabelRect.bottom), m_textBrush);
		int criticalAttackBtnLeft = criticalAttackLabelRect.right + 12;
		int criticalAttackRowCenterY = (criticalAttackLabelRect.top + criticalAttackLabelRect.bottom) / 2;
		int criticalAttackBtnTop = criticalAttackRowCenterY - btnH / 2;
		m_criticalAttackSetButtonRect = {criticalAttackBtnLeft, criticalAttackBtnTop, criticalAttackBtnLeft + btnW, criticalAttackBtnTop + btnH};
		ID2D1SolidColorBrush* criticalAttackBtnBg = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &criticalAttackBtnBg);
		m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_criticalAttackSetButtonRect.left, (FLOAT)m_criticalAttackSetButtonRect.top, (FLOAT)m_criticalAttackSetButtonRect.right, (FLOAT)m_criticalAttackSetButtonRect.bottom), 6, 6), criticalAttackBtnBg);
		criticalAttackBtnBg->Release();
		ID2D1SolidColorBrush* criticalAttackBtnBorder = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &criticalAttackBtnBorder);
		m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_criticalAttackSetButtonRect.left, (FLOAT)m_criticalAttackSetButtonRect.top, (FLOAT)m_criticalAttackSetButtonRect.right, (FLOAT)m_criticalAttackSetButtonRect.bottom), 6, 6), criticalAttackBtnBorder, 1.0f);
		criticalAttackBtnBorder->Release();
		if (!m_criticalAttackKey.empty()) {
			ID2D1SolidColorBrush* btnTextBrush = nullptr;
			if (m_keyCaptureActive && m_criticalAttackCapturing) {
				m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			} else {
				btnTextBrush = m_textBrush;
			}
			m_renderTarget->DrawText(m_criticalAttackKey.c_str(), (UINT32)m_criticalAttackKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_criticalAttackSetButtonRect.left, (FLOAT)m_criticalAttackSetButtonRect.top, (FLOAT)m_criticalAttackSetButtonRect.right, (FLOAT)m_criticalAttackSetButtonRect.bottom), btnTextBrush);
			if (m_keyCaptureActive && m_criticalAttackCapturing && btnTextBrush != m_textBrush) {
				btnTextBrush->Release();
			}
		}
		
		RECT goldenTongueLabelRect = {hotkeysArea.left + 16, hotkeysArea.top + 208, hotkeysArea.left + 140, hotkeysArea.top + 232};
		m_renderTarget->DrawText(L"Golden Tongue", 13, m_titleFormat, D2D1::RectF((FLOAT)goldenTongueLabelRect.left, (FLOAT)goldenTongueLabelRect.top, (FLOAT)goldenTongueLabelRect.right, (FLOAT)goldenTongueLabelRect.bottom), m_textBrush);
		int goldenTongueBtnLeft = goldenTongueLabelRect.right + 12;
		int goldenTongueRowCenterY = (goldenTongueLabelRect.top + goldenTongueLabelRect.bottom) / 2;
		int goldenTongueBtnTop = goldenTongueRowCenterY - btnH / 2;
		m_goldenTongueSetButtonRect = {goldenTongueBtnLeft, goldenTongueBtnTop, goldenTongueBtnLeft + btnW, goldenTongueBtnTop + btnH};
		ID2D1SolidColorBrush* goldenTongueBtnBg = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &goldenTongueBtnBg);
		m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_goldenTongueSetButtonRect.left, (FLOAT)m_goldenTongueSetButtonRect.top, (FLOAT)m_goldenTongueSetButtonRect.right, (FLOAT)m_goldenTongueSetButtonRect.bottom), 6, 6), goldenTongueBtnBg);
		goldenTongueBtnBg->Release();
		ID2D1SolidColorBrush* goldenTongueBtnBorder = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &goldenTongueBtnBorder);
		m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_goldenTongueSetButtonRect.left, (FLOAT)m_goldenTongueSetButtonRect.top, (FLOAT)m_goldenTongueSetButtonRect.right, (FLOAT)m_goldenTongueSetButtonRect.bottom), 6, 6), goldenTongueBtnBorder, 1.0f);
		goldenTongueBtnBorder->Release();
		if (!m_goldenTongueKey.empty()) {
			ID2D1SolidColorBrush* btnTextBrush = nullptr;
			if (m_keyCaptureActive && m_goldenTongueCapturing) {
				m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			} else {
				btnTextBrush = m_textBrush;
			}
			m_renderTarget->DrawText(m_goldenTongueKey.c_str(), (UINT32)m_goldenTongueKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_goldenTongueSetButtonRect.left, (FLOAT)m_goldenTongueSetButtonRect.top, (FLOAT)m_goldenTongueSetButtonRect.right, (FLOAT)m_goldenTongueSetButtonRect.bottom), btnTextBrush);
			if (m_keyCaptureActive && m_goldenTongueCapturing && btnTextBrush != m_textBrush) {
				btnTextBrush->Release();
			}
		}

                // Separator line
                ID2D1SolidColorBrush* separatorBrush = nullptr;
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
                m_renderTarget->DrawLine(D2D1::Point2F(hotkeysArea.left, hotkeysArea.bottom - 5), D2D1::Point2F(hotkeysArea.right, hotkeysArea.bottom - 5), separatorBrush, 1.0f);
                separatorBrush->Release();

                currentY = yPos + 260;
		}

		// Roll M1 settings section
		if (SettingsMatchesSearch(L"Roll M1")) {
		    int yPos = currentY + 5;
		    RECT rollM1Area = {60 + 40, yPos, 60 + 40 + 600, yPos + 140};

		    m_renderTarget->DrawText(L"Roll M1", 7, m_titleFormat, D2D1::RectF(rollM1Area.left + 16, rollM1Area.top + 12, rollM1Area.right, rollM1Area.top + 40), m_textBrush);

		    int toggleWidth = 40;
		    int toggleHeight = 20;
		    int rowSpacing = 28;
		    int btnW = 60;
		    int btnH = 24;

		    // Roll M1 Hotkey
		    RECT rollM1LabelRect = {rollM1Area.left + 16, rollM1Area.top + 40, rollM1Area.left + 140, rollM1Area.top + 64};
		    m_renderTarget->DrawText(L"Roll M1", 7, m_titleFormat, D2D1::RectF((FLOAT)rollM1LabelRect.left, (FLOAT)rollM1LabelRect.top, (FLOAT)rollM1LabelRect.right, (FLOAT)rollM1LabelRect.bottom), m_textBrush);
		    int btnLeft = rollM1LabelRect.right + 12;
		    int rowCenterY = (rollM1LabelRect.top + rollM1LabelRect.bottom) / 2;
		    int btnTop = rowCenterY - btnH / 2;
		    m_rollM1SetButtonRect = {btnLeft, btnTop, btnLeft + btnW, btnTop + btnH};

		    ID2D1SolidColorBrush* rollM1BtnBg = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &rollM1BtnBg);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollM1SetButtonRect.left, (FLOAT)m_rollM1SetButtonRect.top, (FLOAT)m_rollM1SetButtonRect.right, (FLOAT)m_rollM1SetButtonRect.bottom), 6, 6), rollM1BtnBg);
		    rollM1BtnBg->Release();
		    ID2D1SolidColorBrush* rollM1BtnBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &rollM1BtnBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollM1SetButtonRect.left, (FLOAT)m_rollM1SetButtonRect.top, (FLOAT)m_rollM1SetButtonRect.right, (FLOAT)m_rollM1SetButtonRect.bottom), 6, 6), rollM1BtnBorder, 1.0f);
		    rollM1BtnBorder->Release();
		    if (!m_rollM1Key.empty()) {
		        ID2D1SolidColorBrush* btnTextBrush = nullptr;
		        if (m_keyCaptureActive && m_rollM1Capturing) {
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
		        } else {
		            btnTextBrush = m_textBrush;
		        }
		        m_renderTarget->DrawText(m_rollM1Key.c_str(), (UINT32)m_rollM1Key.size(), m_titleFormat, D2D1::RectF((FLOAT)m_rollM1SetButtonRect.left, (FLOAT)m_rollM1SetButtonRect.top, (FLOAT)m_rollM1SetButtonRect.right, (FLOAT)m_rollM1SetButtonRect.bottom), btnTextBrush);
		        if (m_keyCaptureActive && m_rollM1Capturing && btnTextBrush != m_textBrush) {
		            btnTextBrush->Release();
		        }
		    }

		    // Roll Crit Hotkey
		    RECT rollCritLabelRect = {rollM1Area.left + 16, rollM1Area.top + 40 + rowSpacing, rollM1Area.left + 140, rollM1Area.top + 64 + rowSpacing};
		    m_renderTarget->DrawText(L"Roll Critical", 13, m_titleFormat, D2D1::RectF((FLOAT)rollCritLabelRect.left, (FLOAT)rollCritLabelRect.top, (FLOAT)rollCritLabelRect.right, (FLOAT)rollCritLabelRect.bottom), m_textBrush);
		    btnLeft = rollCritLabelRect.right + 12;
		    rowCenterY = (rollCritLabelRect.top + rollCritLabelRect.bottom) / 2;
		    btnTop = rowCenterY - btnH / 2;
		    m_rollCritSetButtonRect = {btnLeft, btnTop, btnLeft + btnW, btnTop + btnH};
		    ID2D1SolidColorBrush* rollCritBtnBg = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &rollCritBtnBg);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollCritSetButtonRect.left, (FLOAT)m_rollCritSetButtonRect.top, (FLOAT)m_rollCritSetButtonRect.right, (FLOAT)m_rollCritSetButtonRect.bottom), 6, 6), rollCritBtnBg);
		    rollCritBtnBg->Release();
		    ID2D1SolidColorBrush* rollCritBtnBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &rollCritBtnBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollCritSetButtonRect.left, (FLOAT)m_rollCritSetButtonRect.top, (FLOAT)m_rollCritSetButtonRect.right, (FLOAT)m_rollCritSetButtonRect.bottom), 6, 6), rollCritBtnBorder, 1.0f);
		    rollCritBtnBorder->Release();
		    if (!m_rollCritKey.empty()) {
		        ID2D1SolidColorBrush* btnTextBrush = nullptr;
		        if (m_keyCaptureActive && m_rollCritCapturing) {
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
		        } else {
		            btnTextBrush = m_textBrush;
		        }
		        m_renderTarget->DrawText(m_rollCritKey.c_str(), (UINT32)m_rollCritKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_rollCritSetButtonRect.left, (FLOAT)m_rollCritSetButtonRect.top, (FLOAT)m_rollCritSetButtonRect.right, (FLOAT)m_rollCritSetButtonRect.bottom), btnTextBrush);
		        if (m_keyCaptureActive && m_rollCritCapturing && btnTextBrush != m_textBrush) {
		            btnTextBrush->Release();
		        }
		    }

		    // Roll Critical Toggle
		    RECT rollCriticalLabelRect = {rollM1Area.left + 16, rollM1Area.top + 40 + rowSpacing * 2, rollM1Area.left + 140, rollM1Area.top + 64 + rowSpacing * 2};
		    m_renderTarget->DrawText(L"Roll Critical", 13, m_titleFormat, D2D1::RectF((FLOAT)rollCriticalLabelRect.left, (FLOAT)rollCriticalLabelRect.top, (FLOAT)rollCriticalLabelRect.right, (FLOAT)rollCriticalLabelRect.bottom), m_textBrush);
		    int toggleLeft = rollCriticalLabelRect.right + 12;
		    rowCenterY = (rollCriticalLabelRect.top + rollCriticalLabelRect.bottom) / 2;
		    int toggleTop = rowCenterY - toggleHeight / 2;
		    m_rollCriticalToggleRect = {toggleLeft, toggleTop, toggleLeft + toggleWidth, toggleTop + toggleHeight};

		    ID2D1SolidColorBrush* toggleTrackBg = nullptr;
		    if (m_rollCritical) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &toggleTrackBg);
		    } else {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &toggleTrackBg);
		    }
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollCriticalToggleRect.left, (FLOAT)m_rollCriticalToggleRect.top, (FLOAT)m_rollCriticalToggleRect.right, (FLOAT)m_rollCriticalToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBg);
		    toggleTrackBg->Release();
		    ID2D1SolidColorBrush* toggleTrackBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &toggleTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollCriticalToggleRect.left, (FLOAT)m_rollCriticalToggleRect.top, (FLOAT)m_rollCriticalToggleRect.right, (FLOAT)m_rollCriticalToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBorder, 1.0f);
		    toggleTrackBorder->Release();

		    int knobSize = 16;
		    float knobX = m_rollCritical ? (m_rollCriticalToggleRect.right - knobSize / 2.0f - 2.0f) : (m_rollCriticalToggleRect.left + knobSize / 2.0f + 2.0f);
		    float knobY = (m_rollCriticalToggleRect.top + m_rollCriticalToggleRect.bottom) / 2.0f;
		    ID2D1SolidColorBrush* knobBrush = nullptr;
		    if (m_rollCritical) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &knobBrush);
		    } else {
		        knobBrush = m_whiteBrush;
		        knobBrush->AddRef();
		    }
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, knobY), knobSize / 2.0f, knobSize / 2.0f), knobBrush);
		    knobBrush->Release();

		    // Separator line
		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(rollM1Area.left, rollM1Area.bottom + 10), D2D1::Point2F(rollM1Area.right, rollM1Area.bottom + 10), separatorBrush, 1.0f);
		    separatorBrush->Release();

		    currentY = yPos + 140;
		}

		if (SettingsMatchesSearch(L"Roll Spit")) {
		    int yPos = currentY + 20;
		    RECT rollSpitArea = {60 + 40, yPos, 60 + 40 + 600, yPos + 120};

		    m_renderTarget->DrawText(L"Roll Spit", 9, m_titleFormat, D2D1::RectF(rollSpitArea.left + 16, rollSpitArea.top + 12, rollSpitArea.right, rollSpitArea.top + 40), m_textBrush);
		    
		    RECT spitLabelRect = {rollSpitArea.left + 16, rollSpitArea.top + 40, rollSpitArea.left + 140, rollSpitArea.top + 64};
		    m_renderTarget->DrawText(L"Spit Key", 8, m_titleFormat, D2D1::RectF((FLOAT)spitLabelRect.left, (FLOAT)spitLabelRect.top, (FLOAT)spitLabelRect.right, (FLOAT)spitLabelRect.bottom), m_textBrush);
		    int spitBtnW = 60;
		    int spitBtnH = 24;
		    int spitBtnLeft = spitLabelRect.right + 12;
		    int spitRowCenterY = (spitLabelRect.top + spitLabelRect.bottom) / 2;
		    int spitBtnTop = spitRowCenterY - spitBtnH / 2;
		    m_rollSpitSpitSetButtonRect = {spitBtnLeft, spitBtnTop, spitBtnLeft + spitBtnW, spitBtnTop + spitBtnH};
		    ID2D1SolidColorBrush* rollSpitSpitBtnBg = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &rollSpitSpitBtnBg);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollSpitSpitSetButtonRect.left, (FLOAT)m_rollSpitSpitSetButtonRect.top, (FLOAT)m_rollSpitSpitSetButtonRect.right, (FLOAT)m_rollSpitSpitSetButtonRect.bottom), 6, 6), rollSpitSpitBtnBg);
		    rollSpitSpitBtnBg->Release();
		    ID2D1SolidColorBrush* rollSpitSpitBtnBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &rollSpitSpitBtnBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollSpitSpitSetButtonRect.left, (FLOAT)m_rollSpitSpitSetButtonRect.top, (FLOAT)m_rollSpitSpitSetButtonRect.right, (FLOAT)m_rollSpitSpitSetButtonRect.bottom), 6, 6), rollSpitSpitBtnBorder, 1.0f);
		    rollSpitSpitBtnBorder->Release();
		    if (!m_rollSpitSpitKey.empty()) {
			    ID2D1SolidColorBrush* btnTextBrush = nullptr;
			    if (m_keyCaptureActive && m_rollSpitSpitCapturing) {
				    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			    } else {
				    btnTextBrush = m_textBrush;
			    }
			    m_renderTarget->DrawText(m_rollSpitSpitKey.c_str(), (UINT32)m_rollSpitSpitKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_rollSpitSpitSetButtonRect.left, (FLOAT)m_rollSpitSpitSetButtonRect.top, (FLOAT)m_rollSpitSpitSetButtonRect.right, (FLOAT)m_rollSpitSpitSetButtonRect.bottom), btnTextBrush);
			    if (m_keyCaptureActive && m_rollSpitSpitCapturing && btnTextBrush != m_textBrush) {
				    btnTextBrush->Release();
			    }
		    }
		    
		    RECT triggerLabelRect = {rollSpitArea.left + 16, rollSpitArea.top + 68, rollSpitArea.left + 140, rollSpitArea.top + 92};
		    m_renderTarget->DrawText(L"Roll Spit", 9, m_titleFormat, D2D1::RectF((FLOAT)triggerLabelRect.left, (FLOAT)triggerLabelRect.top, (FLOAT)triggerLabelRect.right, (FLOAT)triggerLabelRect.bottom), m_textBrush);
		    int triggerBtnLeft = triggerLabelRect.right + 12;
		    int triggerRowCenterY = (triggerLabelRect.top + triggerLabelRect.bottom) / 2;
		    int triggerBtnTop = triggerRowCenterY - spitBtnH / 2;
		    m_rollSpitTriggerSetButtonRect = {triggerBtnLeft, triggerBtnTop, triggerBtnLeft + spitBtnW, triggerBtnTop + spitBtnH};
		    ID2D1SolidColorBrush* rollSpitTriggerBtnBg = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &rollSpitTriggerBtnBg);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollSpitTriggerSetButtonRect.left, (FLOAT)m_rollSpitTriggerSetButtonRect.top, (FLOAT)m_rollSpitTriggerSetButtonRect.right, (FLOAT)m_rollSpitTriggerSetButtonRect.bottom), 6, 6), rollSpitTriggerBtnBg);
		    rollSpitTriggerBtnBg->Release();
		    ID2D1SolidColorBrush* rollSpitTriggerBtnBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &rollSpitTriggerBtnBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollSpitTriggerSetButtonRect.left, (FLOAT)m_rollSpitTriggerSetButtonRect.top, (FLOAT)m_rollSpitTriggerSetButtonRect.right, (FLOAT)m_rollSpitTriggerSetButtonRect.bottom), 6, 6), rollSpitTriggerBtnBorder, 1.0f);
		    rollSpitTriggerBtnBorder->Release();
		    if (!m_rollSpitTriggerKey.empty()) {
			    ID2D1SolidColorBrush* btnTextBrush = nullptr;
			    if (m_keyCaptureActive && m_rollSpitTriggerCapturing) {
				    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			    } else {
				    btnTextBrush = m_textBrush;
			    }
			    m_renderTarget->DrawText(m_rollSpitTriggerKey.c_str(), (UINT32)m_rollSpitTriggerKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_rollSpitTriggerSetButtonRect.left, (FLOAT)m_rollSpitTriggerSetButtonRect.top, (FLOAT)m_rollSpitTriggerSetButtonRect.right, (FLOAT)m_rollSpitTriggerSetButtonRect.bottom), btnTextBrush);
			    if (m_keyCaptureActive && m_rollSpitTriggerCapturing && btnTextBrush != m_textBrush) {
				    btnTextBrush->Release();
			    }
		    }

		    // Separator line
		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(rollSpitArea.left, rollSpitArea.bottom + 10), D2D1::Point2F(rollSpitArea.right, rollSpitArea.bottom + 10), separatorBrush, 1.0f);
		    separatorBrush->Release();

		    currentY += 115;
		}

		// Hotbar Slots section
		if (SettingsMatchesSearch(L"Hotbar Slots")) {
		    int yPos = currentY + 40;
		    RECT hotbarArea = {60 + 40, yPos, 60 + 40 + 600, yPos + 340};

		    m_renderTarget->DrawText(L"Hotbar Slots", 12, m_titleFormat, D2D1::RectF(hotbarArea.left + 16, hotbarArea.top + 12, hotbarArea.right, hotbarArea.top + 40), m_textBrush);
		    
		    for (int i = 0; i < 10; i++) {
		        int slotY = hotbarArea.top + 40 + 8 + (i * 28);
		        RECT slotLabelRect = {hotbarArea.left + 16, slotY, hotbarArea.left + 140, slotY + 24};
		        std::wstring slotLabel = L"Slot " + std::to_wstring(i + 1);
		        if (i == 9) slotLabel = L"Slot 0";
		        m_renderTarget->DrawText(slotLabel.c_str(), (UINT32)slotLabel.size(), m_titleFormat, D2D1::RectF((FLOAT)slotLabelRect.left, (FLOAT)slotLabelRect.top, (FLOAT)slotLabelRect.right, (FLOAT)slotLabelRect.bottom), m_textBrush);
		        
		        int slotBtnLeft = slotLabelRect.right + 12;
		        int slotRowCenterY = (slotLabelRect.top + slotLabelRect.bottom) / 2;
		        int slotBtnTop = slotRowCenterY - 12; // btnH/2 = 12
		        m_hotbarSlotSetButtonRect[i] = {slotBtnLeft, slotBtnTop, slotBtnLeft + 60, slotBtnTop + 24}; // btnW = 60, btnH = 24
		        ID2D1SolidColorBrush* hotbarSlotBtnBg = nullptr;
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &hotbarSlotBtnBg);
		        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_hotbarSlotSetButtonRect[i].left, (FLOAT)m_hotbarSlotSetButtonRect[i].top, (FLOAT)m_hotbarSlotSetButtonRect[i].right, (FLOAT)m_hotbarSlotSetButtonRect[i].bottom), 6, 6), hotbarSlotBtnBg);
		        hotbarSlotBtnBg->Release();
		        ID2D1SolidColorBrush* hotbarSlotBtnBorder = nullptr;
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &hotbarSlotBtnBorder);
		        m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_hotbarSlotSetButtonRect[i].left, (FLOAT)m_hotbarSlotSetButtonRect[i].top, (FLOAT)m_hotbarSlotSetButtonRect[i].right, (FLOAT)m_hotbarSlotSetButtonRect[i].bottom), 6, 6), hotbarSlotBtnBorder, 1.0f);
		        hotbarSlotBtnBorder->Release();
		        if (!m_hotbarSlotKeys[i].empty()) {
			        ID2D1SolidColorBrush* btnTextBrush = nullptr;
			        if (m_keyCaptureActive && m_hotbarSlotCapturing[i]) {
				        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			        } else {
				        btnTextBrush = m_textBrush;
			        }
			        m_renderTarget->DrawText(m_hotbarSlotKeys[i].c_str(), (UINT32)m_hotbarSlotKeys[i].size(), m_titleFormat, D2D1::RectF((FLOAT)m_hotbarSlotSetButtonRect[i].left, (FLOAT)m_hotbarSlotSetButtonRect[i].top, (FLOAT)m_hotbarSlotSetButtonRect[i].right, (FLOAT)m_hotbarSlotSetButtonRect[i].bottom), btnTextBrush);
			        if (m_keyCaptureActive && m_hotbarSlotCapturing[i] && btnTextBrush != m_textBrush) {
				        btnTextBrush->Release();
			        }
		        }
		        
		        // Roll Cast icon (left of toggle slider)
		        int iconSize = 20;
		        int iconLeft = m_hotbarSlotSetButtonRect[i].right + 128;
		        int iconTop = slotRowCenterY - iconSize / 2;
		        m_hotbarSlotIconRect[i] = {iconLeft, iconTop, iconLeft + iconSize, iconTop + iconSize};
		        DrawRollCastIconAnimated(m_hotbarSlotIconRect[i], false, 0.0f);
		        
		        // Toggle slider (right of icon)
		        int toggleWidth = 40;
		        int toggleHeight = 20;
		        int toggleLeft = m_hotbarSlotIconRect[i].right + 12;
		        int toggleTop = slotRowCenterY - toggleHeight / 2;
		        m_hotbarSlotToggleRect[i] = {toggleLeft, toggleTop, toggleLeft + toggleWidth, toggleTop + toggleHeight};
		        
		        // Toggle track background
		        ID2D1SolidColorBrush* toggleTrackBg = nullptr;
		        if (m_hotbarSlotEnabled[i]) {
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &toggleTrackBg);
		        } else {
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &toggleTrackBg);
		        }
		        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_hotbarSlotToggleRect[i].left, (FLOAT)m_hotbarSlotToggleRect[i].top, (FLOAT)m_hotbarSlotToggleRect[i].right, (FLOAT)m_hotbarSlotToggleRect[i].bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBg);
		        toggleTrackBg->Release();
		        ID2D1SolidColorBrush* toggleTrackBorder = nullptr;
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &toggleTrackBorder);
		        m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_hotbarSlotToggleRect[i].left, (FLOAT)m_hotbarSlotToggleRect[i].top, (FLOAT)m_hotbarSlotToggleRect[i].right, (FLOAT)m_hotbarSlotToggleRect[i].bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBorder, 1.0f);
		        toggleTrackBorder->Release();

		        // Toggle knob
		        int knobSize = 16;
		        float knobX = m_hotbarSlotEnabled[i] ? (m_hotbarSlotToggleRect[i].right - knobSize / 2.0f - 2.0f) : (m_hotbarSlotToggleRect[i].left + knobSize / 2.0f + 2.0f);
		        float knobY = (m_hotbarSlotToggleRect[i].top + m_hotbarSlotToggleRect[i].bottom) / 2.0f;
		        ID2D1SolidColorBrush* hotbarKnobBrush = nullptr;
		        if (m_hotbarSlotEnabled[i]) {
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &hotbarKnobBrush);
		        } else {
		            hotbarKnobBrush = m_whiteBrush;
		            hotbarKnobBrush->AddRef();
		        }
		        m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, knobY), knobSize / 2.0f, knobSize / 2.0f), hotbarKnobBrush);
		        hotbarKnobBrush->Release();
		        
		        // Map Cast icon (left of Map Cast toggle slider)
		        int mapCastIconSize = 24;
		        int mapCastIconLeft = m_hotbarSlotToggleRect[i].right + 12;
		        int mapCastIconTop = slotRowCenterY - mapCastIconSize / 2;
		        m_hotbarSlotMapCastIconRect[i] = {mapCastIconLeft, mapCastIconTop, mapCastIconLeft + mapCastIconSize, mapCastIconTop + mapCastIconSize};
		        DrawMapCastIconAnimated(m_hotbarSlotMapCastIconRect[i], false, 0.0f);
		        
		        // Map Cast toggle slider (right of Map Cast icon)
		        int mapCastToggleLeft = m_hotbarSlotMapCastIconRect[i].right + 12;
		        int mapCastToggleTop = slotRowCenterY - toggleHeight / 2;
		        m_hotbarSlotMapCastToggleRect[i] = {mapCastToggleLeft, mapCastToggleTop, mapCastToggleLeft + toggleWidth, mapCastToggleTop + toggleHeight};
		        
		        // Map Cast toggle track background
		        ID2D1SolidColorBrush* mapCastToggleTrackBg = nullptr;
		        if (m_hotbarSlotMapCastEnabled[i]) {
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &mapCastToggleTrackBg);
		        } else {
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &mapCastToggleTrackBg);
		        }
		        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_hotbarSlotMapCastToggleRect[i].left, (FLOAT)m_hotbarSlotMapCastToggleRect[i].top, (FLOAT)m_hotbarSlotMapCastToggleRect[i].right, (FLOAT)m_hotbarSlotMapCastToggleRect[i].bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), mapCastToggleTrackBg);
		        mapCastToggleTrackBg->Release();
		        ID2D1SolidColorBrush* mapCastToggleTrackBorder = nullptr;
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &mapCastToggleTrackBorder);
		        m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_hotbarSlotMapCastToggleRect[i].left, (FLOAT)m_hotbarSlotMapCastToggleRect[i].top, (FLOAT)m_hotbarSlotMapCastToggleRect[i].right, (FLOAT)m_hotbarSlotMapCastToggleRect[i].bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), mapCastToggleTrackBorder, 1.0f);
		        mapCastToggleTrackBorder->Release();

		        // Map Cast toggle knob
		        float mapCastKnobX = m_hotbarSlotMapCastEnabled[i] ? (m_hotbarSlotMapCastToggleRect[i].right - knobSize / 2.0f - 2.0f) : (m_hotbarSlotMapCastToggleRect[i].left + knobSize / 2.0f + 2.0f);
		        float mapCastKnobY = (m_hotbarSlotMapCastToggleRect[i].top + m_hotbarSlotMapCastToggleRect[i].bottom) / 2.0f;
		        ID2D1SolidColorBrush* mapCastKnobBrush = nullptr;
		        if (m_hotbarSlotMapCastEnabled[i]) {
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &mapCastKnobBrush);
		        } else {
		            mapCastKnobBrush = m_whiteBrush;
		            mapCastKnobBrush->AddRef();
		        }
		        m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(mapCastKnobX, mapCastKnobY), knobSize / 2.0f, knobSize / 2.0f), mapCastKnobBrush);
		        mapCastKnobBrush->Release();
		    }

		    // Separator line
		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(hotbarArea.left, hotbarArea.bottom + 10), D2D1::Point2F(hotbarArea.right, hotbarArea.bottom + 10), separatorBrush, 1.0f);
		    separatorBrush->Release();

		    currentY += 380; // Move to next section position
		}

		if (SettingsMatchesSearch(L"Hold M1")) {
		    int yPos = currentY + 30;
		    RECT holdM1Area = {60 + 40, yPos, 60 + 40 + 600, yPos + 68};

		    m_renderTarget->DrawText(L"Hold M1", 7, m_titleFormat, D2D1::RectF(holdM1Area.left + 16, holdM1Area.top + 12, holdM1Area.right, holdM1Area.top + 40), m_textBrush);
		    
		    RECT holdM1LabelRect = {holdM1Area.left + 16, holdM1Area.top + 40, holdM1Area.left + 200, holdM1Area.top + 64};
		    m_renderTarget->DrawText(L"Intelligent Hold M1", 19, m_titleFormat, D2D1::RectF((FLOAT)holdM1LabelRect.left, (FLOAT)holdM1LabelRect.top, (FLOAT)holdM1LabelRect.right, (FLOAT)holdM1LabelRect.bottom), m_textBrush);
		    
		    int toggleWidth = 40;
		    int toggleHeight = 20;
		    int toggleLeft = holdM1LabelRect.right + 12;
		    int rowCenterY = (holdM1LabelRect.top + holdM1LabelRect.bottom) / 2;
		    int toggleTop = rowCenterY - toggleHeight / 2;
		    m_holdM1ToggleRect = {toggleLeft, toggleTop, toggleLeft + toggleWidth, toggleTop + toggleHeight};
		    
		    ID2D1SolidColorBrush* toggleTrackBg = nullptr;
		    if (m_holdM1Enabled) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &toggleTrackBg);
		    } else {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &toggleTrackBg);
		    }
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_holdM1ToggleRect.left, (FLOAT)m_holdM1ToggleRect.top, (FLOAT)m_holdM1ToggleRect.right, (FLOAT)m_holdM1ToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBg);
		    toggleTrackBg->Release();
		    ID2D1SolidColorBrush* toggleTrackBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &toggleTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_holdM1ToggleRect.left, (FLOAT)m_holdM1ToggleRect.top, (FLOAT)m_holdM1ToggleRect.right, (FLOAT)m_holdM1ToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBorder, 1.0f);
		    toggleTrackBorder->Release();
		    
		    int knobSize = 16;
		    float knobX = m_holdM1Enabled ? (m_holdM1ToggleRect.right - knobSize / 2.0f - 2.0f) : (m_holdM1ToggleRect.left + knobSize / 2.0f + 2.0f);
		    float knobY = (m_holdM1ToggleRect.top + m_holdM1ToggleRect.bottom) / 2.0f;
		    ID2D1SolidColorBrush* knobBrush = nullptr;
		    if (m_holdM1Enabled) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &knobBrush);
		    } else {
		        knobBrush = m_whiteBrush;
		        knobBrush->AddRef();
		    }
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, knobY), knobSize / 2.0f, knobSize / 2.0f), knobBrush);
		    knobBrush->Release();

		    // Separator line
		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(holdM1Area.left, holdM1Area.bottom + 25), D2D1::Point2F(holdM1Area.right, holdM1Area.bottom + 25), separatorBrush, 1.0f);
		    separatorBrush->Release();

		    currentY += 108;
		}

		if (SettingsMatchesSearch(L"Better Parry")) {
		    int yPos = currentY + 40;
		    RECT betterParryArea = {60 + 40, yPos, 60 + 40 + 600, yPos + 240};

		    m_renderTarget->DrawText(L"Better Parry", 12, m_titleFormat, D2D1::RectF(betterParryArea.left + 16, betterParryArea.top + 12, betterParryArea.right, betterParryArea.top + 40), m_textBrush);

		    int toggleWidth = 40;
		    int toggleHeight = 20;
		    int rowSpacing = 28;

		    // Roll Parry Toggle
		    RECT rollParryLabelRect = {betterParryArea.left + 16, betterParryArea.top + 40, betterParryArea.left + 200, betterParryArea.top + 64};
		    m_renderTarget->DrawText(L"Roll Parry", 10, m_titleFormat, D2D1::RectF((FLOAT)rollParryLabelRect.left, (FLOAT)rollParryLabelRect.top, (FLOAT)rollParryLabelRect.right, (FLOAT)rollParryLabelRect.bottom), m_textBrush);
		    int toggleLeft = rollParryLabelRect.right + 12;
		    int rowCenterY = (rollParryLabelRect.top + rollParryLabelRect.bottom) / 2;
		    int toggleTop = rowCenterY - toggleHeight / 2;
		    m_rollParryToggleRect = {toggleLeft, toggleTop, toggleLeft + toggleWidth, toggleTop + toggleHeight};

		    ID2D1SolidColorBrush* toggleTrackBg = nullptr;
		    if (m_rollParry) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &toggleTrackBg);
		    } else {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &toggleTrackBg);
		    }
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollParryToggleRect.left, (FLOAT)m_rollParryToggleRect.top, (FLOAT)m_rollParryToggleRect.right, (FLOAT)m_rollParryToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBg);
		    toggleTrackBg->Release();
		    ID2D1SolidColorBrush* toggleTrackBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &toggleTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollParryToggleRect.left, (FLOAT)m_rollParryToggleRect.top, (FLOAT)m_rollParryToggleRect.right, (FLOAT)m_rollParryToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBorder, 1.0f);
		    toggleTrackBorder->Release();

		    int knobSize = 16;
		    float knobX = m_rollParry ? (m_rollParryToggleRect.right - knobSize / 2.0f - 2.0f) : (m_rollParryToggleRect.left + knobSize / 2.0f + 2.0f);
		    float knobY = (m_rollParryToggleRect.top + m_rollParryToggleRect.bottom) / 2.0f;
		    ID2D1SolidColorBrush* knobBrush = nullptr;
		    if (m_rollParry) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &knobBrush);
		    } else {
		        knobBrush = m_whiteBrush;
		        knobBrush->AddRef();
		    }
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, knobY), knobSize / 2.0f, knobSize / 2.0f), knobBrush);
		    knobBrush->Release();

		    RECT antiShakyLabelRect = {betterParryArea.left + 16, betterParryArea.top + 40 + rowSpacing, betterParryArea.left + 200, betterParryArea.top + 64 + rowSpacing};
		    m_renderTarget->DrawText(L"Anti-Shaky Block (Partial)", 26, m_titleFormat, D2D1::RectF((FLOAT)antiShakyLabelRect.left, (FLOAT)antiShakyLabelRect.top, (FLOAT)antiShakyLabelRect.right, (FLOAT)antiShakyLabelRect.bottom), m_textBrush);
		    toggleLeft = antiShakyLabelRect.right + 12;
		    rowCenterY = (antiShakyLabelRect.top + antiShakyLabelRect.bottom) / 2;
		    toggleTop = rowCenterY - toggleHeight / 2;
		    m_betterParryAntiShakyBlockToggleRect = {toggleLeft, toggleTop, toggleLeft + toggleWidth, toggleTop + toggleHeight};

		    toggleTrackBg = nullptr;
		    if (m_betterParryAntiShakyBlock) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &toggleTrackBg);
		    } else {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &toggleTrackBg);
		    }
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_betterParryAntiShakyBlockToggleRect.left, (FLOAT)m_betterParryAntiShakyBlockToggleRect.top, (FLOAT)m_betterParryAntiShakyBlockToggleRect.right, (FLOAT)m_betterParryAntiShakyBlockToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBg);
		    toggleTrackBg->Release();
		    toggleTrackBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &toggleTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_betterParryAntiShakyBlockToggleRect.left, (FLOAT)m_betterParryAntiShakyBlockToggleRect.top, (FLOAT)m_betterParryAntiShakyBlockToggleRect.right, (FLOAT)m_betterParryAntiShakyBlockToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBorder, 1.0f);
		    toggleTrackBorder->Release();

		    knobX = m_betterParryAntiShakyBlock ? (m_betterParryAntiShakyBlockToggleRect.right - knobSize / 2.0f - 2.0f) : (m_betterParryAntiShakyBlockToggleRect.left + knobSize / 2.0f + 2.0f);
		    knobY = (m_betterParryAntiShakyBlockToggleRect.top + m_betterParryAntiShakyBlockToggleRect.bottom) / 2.0f;
		    knobBrush = nullptr;
		    if (m_betterParryAntiShakyBlock) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &knobBrush);
		    } else {
		        knobBrush = m_whiteBrush;
		        knobBrush->AddRef();
		    }
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, knobY), knobSize / 2.0f, knobSize / 2.0f), knobBrush);
		    knobBrush->Release();
		    
		    RECT lightspeedLabelRect = {betterParryArea.left + 16, betterParryArea.top + 40 + rowSpacing * 2, betterParryArea.left + 200, betterParryArea.top + 64 + rowSpacing * 2};
		    m_renderTarget->DrawText(L"Lightspeed Reflex Macro", 23, m_titleFormat, D2D1::RectF((FLOAT)lightspeedLabelRect.left, (FLOAT)lightspeedLabelRect.top, (FLOAT)lightspeedLabelRect.right, (FLOAT)lightspeedLabelRect.bottom), m_textBrush);
		    toggleLeft = lightspeedLabelRect.right + 12;
		    rowCenterY = (lightspeedLabelRect.top + lightspeedLabelRect.bottom) / 2;
		    toggleTop = rowCenterY - toggleHeight / 2;
		    m_betterParryLightspeedReflexToggleRect = {toggleLeft, toggleTop, toggleLeft + toggleWidth, toggleTop + toggleHeight};
		    
		    if (m_betterParryLightspeedReflex) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &toggleTrackBg);
		    } else {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &toggleTrackBg);
		    }
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_betterParryLightspeedReflexToggleRect.left, (FLOAT)m_betterParryLightspeedReflexToggleRect.top, (FLOAT)m_betterParryLightspeedReflexToggleRect.right, (FLOAT)m_betterParryLightspeedReflexToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBg);
		    toggleTrackBg->Release();
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &toggleTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_betterParryLightspeedReflexToggleRect.left, (FLOAT)m_betterParryLightspeedReflexToggleRect.top, (FLOAT)m_betterParryLightspeedReflexToggleRect.right, (FLOAT)m_betterParryLightspeedReflexToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBorder, 1.0f);
		    toggleTrackBorder->Release();
		    
		    knobX = m_betterParryLightspeedReflex ? (m_betterParryLightspeedReflexToggleRect.right - knobSize / 2.0f - 2.0f) : (m_betterParryLightspeedReflexToggleRect.left + knobSize / 2.0f + 2.0f);
		    knobY = (m_betterParryLightspeedReflexToggleRect.top + m_betterParryLightspeedReflexToggleRect.bottom) / 2.0f;
		    if (m_betterParryLightspeedReflex) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &knobBrush);
		    } else {
		        knobBrush = m_whiteBrush;
		        knobBrush->AddRef();
		    }
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, knobY), knobSize / 2.0f, knobSize / 2.0f), knobBrush);
		    knobBrush->Release();
		    
		    RECT shouldParryLabelRect = {betterParryArea.left + 16, betterParryArea.top + 40 + rowSpacing * 3, betterParryArea.left + 200, betterParryArea.top + 64 + rowSpacing * 3};
		    m_renderTarget->DrawText(L"Should Parry", 12, m_titleFormat, D2D1::RectF((FLOAT)shouldParryLabelRect.left, (FLOAT)shouldParryLabelRect.top, (FLOAT)shouldParryLabelRect.right, (FLOAT)shouldParryLabelRect.bottom), m_textBrush);
		    toggleLeft = shouldParryLabelRect.right + 12;
		    rowCenterY = (shouldParryLabelRect.top + shouldParryLabelRect.bottom) / 2;
		    toggleTop = rowCenterY - toggleHeight / 2;
		    m_betterParryShouldParryToggleRect = {toggleLeft, toggleTop, toggleLeft + toggleWidth, toggleTop + toggleHeight};
		    
		    if (m_betterParryShouldParry) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &toggleTrackBg);
		    } else {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &toggleTrackBg);
		    }
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_betterParryShouldParryToggleRect.left, (FLOAT)m_betterParryShouldParryToggleRect.top, (FLOAT)m_betterParryShouldParryToggleRect.right, (FLOAT)m_betterParryShouldParryToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBg);
		    toggleTrackBg->Release();
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &toggleTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_betterParryShouldParryToggleRect.left, (FLOAT)m_betterParryShouldParryToggleRect.top, (FLOAT)m_betterParryShouldParryToggleRect.right, (FLOAT)m_betterParryShouldParryToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBorder, 1.0f);
		    toggleTrackBorder->Release();
		    
		    knobX = m_betterParryShouldParry ? (m_betterParryShouldParryToggleRect.right - knobSize / 2.0f - 2.0f) : (m_betterParryShouldParryToggleRect.left + knobSize / 2.0f + 2.0f);
		    knobY = (m_betterParryShouldParryToggleRect.top + m_betterParryShouldParryToggleRect.bottom) / 2.0f;
		    if (m_betterParryShouldParry) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &knobBrush);
		    } else {
		        knobBrush = m_whiteBrush;
		        knobBrush->AddRef();
		    }
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, knobY), knobSize / 2.0f, knobSize / 2.0f), knobBrush);
		    knobBrush->Release();

		    // Roll Parry Hotkey Button
		    RECT rollParryKeyLabelRect = {betterParryArea.left + 16, betterParryArea.top + 40 + rowSpacing * 4, betterParryArea.left + 200, betterParryArea.top + 64 + rowSpacing * 4};
		    m_renderTarget->DrawText(L"Roll Parry", 10, m_titleFormat, D2D1::RectF((FLOAT)rollParryKeyLabelRect.left, (FLOAT)rollParryKeyLabelRect.top, (FLOAT)rollParryKeyLabelRect.right, (FLOAT)rollParryKeyLabelRect.bottom), m_textBrush);
		    int btnW = 60;
		    int btnH = 24;
		    int btnLeft = rollParryKeyLabelRect.right + 12;
		    rowCenterY = (rollParryKeyLabelRect.top + rollParryKeyLabelRect.bottom) / 2;
		    int btnTop = rowCenterY - btnH / 2;
		    m_rollParrySetButtonRect = {btnLeft, btnTop, btnLeft + btnW, btnTop + btnH};
		    ID2D1SolidColorBrush* rollParryBtnBg = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &rollParryBtnBg);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollParrySetButtonRect.left, (FLOAT)m_rollParrySetButtonRect.top, (FLOAT)m_rollParrySetButtonRect.right, (FLOAT)m_rollParrySetButtonRect.bottom), 6, 6), rollParryBtnBg);
		    rollParryBtnBg->Release();
		    ID2D1SolidColorBrush* rollParryBtnBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &rollParryBtnBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_rollParrySetButtonRect.left, (FLOAT)m_rollParrySetButtonRect.top, (FLOAT)m_rollParrySetButtonRect.right, (FLOAT)m_rollParrySetButtonRect.bottom), 6, 6), rollParryBtnBorder, 1.0f);
		    rollParryBtnBorder->Release();
		    if (!m_rollParryKey.empty()) {
		        ID2D1SolidColorBrush* btnTextBrush = nullptr;
		        if (m_keyCaptureActive && m_rollParryCapturing) {
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
		        } else {
		            btnTextBrush = m_textBrush;
		        }
		        m_renderTarget->DrawText(m_rollParryKey.c_str(), (UINT32)m_rollParryKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_rollParrySetButtonRect.left, (FLOAT)m_rollParrySetButtonRect.top, (FLOAT)m_rollParrySetButtonRect.right, (FLOAT)m_rollParrySetButtonRect.bottom), btnTextBrush);
		        if (m_keyCaptureActive && m_rollParryCapturing && btnTextBrush != m_textBrush) {
		            btnTextBrush->Release();
		        }
		    }

		    // Separator line
		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(betterParryArea.left, betterParryArea.bottom - 40), D2D1::Point2F(betterParryArea.right, betterParryArea.bottom - 40), separatorBrush, 1.0f);
		    separatorBrush->Release();

		    currentY += 245;
		}

		// Crosshair settings section
		if (SettingsMatchesSearch(L"Crosshair")) {
		    int yPos = currentY + 15;
		    RECT crosshairArea = {60 + 40, yPos, 60 + 40 + 600, yPos + 170};

		m_renderTarget->DrawText(L"Crosshair", 9, m_titleFormat, D2D1::RectF(crosshairArea.left + 16, crosshairArea.top + 12, crosshairArea.right, crosshairArea.top + 40), m_textBrush);
		
		RECT crosshairLabelRect = {crosshairArea.left + 16, crosshairArea.top + 40, crosshairArea.left + 140, crosshairArea.top + 64};
		m_renderTarget->DrawText(L"Crosshair", 9, m_titleFormat, D2D1::RectF((FLOAT)crosshairLabelRect.left, (FLOAT)crosshairLabelRect.top, (FLOAT)crosshairLabelRect.right, (FLOAT)crosshairLabelRect.bottom), m_textBrush);
		
		int crosshairBtnW = 60;
		int crosshairBtnH = 24;
		int crosshairBtnLeft = crosshairLabelRect.right + 12;
		int crosshairRowCenterY = (crosshairLabelRect.top + crosshairLabelRect.bottom) / 2;
		int crosshairBtnTop = crosshairRowCenterY - crosshairBtnH / 2;
		m_crosshairFileButtonRect = {crosshairBtnLeft, crosshairBtnTop, crosshairBtnLeft + crosshairBtnW, crosshairBtnTop + crosshairBtnH};
		ID2D1SolidColorBrush* crosshairFileBtnBg = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &crosshairFileBtnBg);
		m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_crosshairFileButtonRect.left, (FLOAT)m_crosshairFileButtonRect.top, (FLOAT)m_crosshairFileButtonRect.right, (FLOAT)m_crosshairFileButtonRect.bottom), 6, 6), crosshairFileBtnBg);
		crosshairFileBtnBg->Release();
		ID2D1SolidColorBrush* crosshairFileBtnBorder = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &crosshairFileBtnBorder);
		m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_crosshairFileButtonRect.left, (FLOAT)m_crosshairFileButtonRect.top, (FLOAT)m_crosshairFileButtonRect.right, (FLOAT)m_crosshairFileButtonRect.bottom), 6, 6), crosshairFileBtnBorder, 1.0f);
		crosshairFileBtnBorder->Release();
		m_renderTarget->DrawText(L"...", 3, m_titleFormat, D2D1::RectF((FLOAT)m_crosshairFileButtonRect.left, (FLOAT)m_crosshairFileButtonRect.top, (FLOAT)m_crosshairFileButtonRect.right, (FLOAT)m_crosshairFileButtonRect.bottom), m_searchBrush);
		
		RECT fileTextRect = {m_crosshairFileButtonRect.right + 12, crosshairLabelRect.top, crosshairArea.right - 20, crosshairLabelRect.bottom};
		
		std::wstring displayPath = m_crosshairFilePath;
		if (displayPath.length() > 50) {
			size_t startLen = 20;
			size_t endLen = 20;
			displayPath = displayPath.substr(0, startLen) + L"..." + displayPath.substr(displayPath.length() - endLen);
		}
		
		m_renderTarget->DrawText(displayPath.c_str(), (UINT32)displayPath.size(), m_titleFormat, D2D1::RectF((FLOAT)fileTextRect.left, (FLOAT)fileTextRect.top, (FLOAT)fileTextRect.right, (FLOAT)fileTextRect.bottom), m_textBrush);
		
		// Camera Lock section
		RECT cameraLockLabelRect = {crosshairArea.left + 16, crosshairArea.top + 68, crosshairArea.left + 140, crosshairArea.top + 92};
		m_renderTarget->DrawText(L"Camera Lock", 11, m_titleFormat, D2D1::RectF((FLOAT)cameraLockLabelRect.left, (FLOAT)cameraLockLabelRect.top, (FLOAT)cameraLockLabelRect.right, (FLOAT)cameraLockLabelRect.bottom), m_textBrush);
		
		int cameraLockBtnW = 60;
		int cameraLockBtnH = 24;
		int cameraLockBtnLeft = cameraLockLabelRect.right + 12;
		int cameraLockRowCenterY = (cameraLockLabelRect.top + cameraLockLabelRect.bottom) / 2;
		int cameraLockBtnTop = cameraLockRowCenterY - cameraLockBtnH / 2;
		m_cameraLockSetButtonRect = {cameraLockBtnLeft, cameraLockBtnTop, cameraLockBtnLeft + cameraLockBtnW, cameraLockBtnTop + cameraLockBtnH};
		ID2D1SolidColorBrush* cameraLockBtnBg = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &cameraLockBtnBg);
		m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_cameraLockSetButtonRect.left, (FLOAT)m_cameraLockSetButtonRect.top, (FLOAT)m_cameraLockSetButtonRect.right, (FLOAT)m_cameraLockSetButtonRect.bottom), 6, 6), cameraLockBtnBg);
		cameraLockBtnBg->Release();
		ID2D1SolidColorBrush* cameraLockBtnBorder = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &cameraLockBtnBorder);
		m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_cameraLockSetButtonRect.left, (FLOAT)m_cameraLockSetButtonRect.top, (FLOAT)m_cameraLockSetButtonRect.right, (FLOAT)m_cameraLockSetButtonRect.bottom), 6, 6), cameraLockBtnBorder, 1.0f);
		cameraLockBtnBorder->Release();
		if (!m_cameraLockKey.empty()) {
			ID2D1SolidColorBrush* btnTextBrush = nullptr;
			if (m_keyCaptureActive && m_cameraLockCapturing) {
				m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			} else {
				btnTextBrush = m_textBrush;
			}
			m_renderTarget->DrawText(m_cameraLockKey.c_str(), (UINT32)m_cameraLockKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_cameraLockSetButtonRect.left, (FLOAT)m_cameraLockSetButtonRect.top, (FLOAT)m_cameraLockSetButtonRect.right, (FLOAT)m_cameraLockSetButtonRect.bottom), btnTextBrush);
			if (m_keyCaptureActive && m_cameraLockCapturing && btnTextBrush != m_textBrush) {
				btnTextBrush->Release();
			}
		}
		
		// Scale section
		RECT scaleLabelRect = {crosshairArea.left + 16, crosshairArea.top + 96, crosshairArea.left + 140, crosshairArea.top + 120};
		
		int crosshairTrackLeft = scaleLabelRect.left;
		int crosshairTrackRight = crosshairArea.right - 40;
		int crosshairTrackY = scaleLabelRect.top + 12;
		m_crosshairScaleTrackRect = {crosshairTrackLeft, crosshairTrackY, crosshairTrackRight, crosshairTrackY + 4};
		ID2D1SolidColorBrush* crosshairTrackBrush = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &crosshairTrackBrush);
		m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_crosshairScaleTrackRect.left, m_crosshairScaleTrackRect.top, m_crosshairScaleTrackRect.right, m_crosshairScaleTrackRect.bottom), 2, 2), crosshairTrackBrush);
		crosshairTrackBrush->Release();
		ID2D1SolidColorBrush* crosshairTrackBorder = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &crosshairTrackBorder);
		m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_crosshairScaleTrackRect.left, m_crosshairScaleTrackRect.top, m_crosshairScaleTrackRect.right, m_crosshairScaleTrackRect.bottom), 2, 2), crosshairTrackBorder, 1.0f);
		crosshairTrackBorder->Release();
		
		int crosshairKnobX = crosshairTrackLeft + (int)((crosshairTrackRight - crosshairTrackLeft) * m_crosshairScale + 0.5f);
		RECT crosshairKnob = {crosshairKnobX - 8, crosshairTrackY - 8, crosshairKnobX + 8, crosshairTrackY + 8};
		m_crosshairScaleKnobRect = crosshairKnob;
		m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F((float)crosshairKnobX, (float)crosshairTrackY), 8.0f, 8.0f), m_whiteBrush);
		
		    wchar_t scaleLabel[64];
		    swprintf(scaleLabel, 64, L"Scale: %.2f", m_crosshairScale);
		    m_renderTarget->DrawText(scaleLabel, (UINT32)wcslen(scaleLabel), m_titleFormat, D2D1::RectF(crosshairArea.left + 16, crosshairTrackY + 20, crosshairArea.right, crosshairTrackY + 44), m_textBrush);

		    // Separator line
		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(crosshairArea.left, crosshairArea.bottom + 10), D2D1::Point2F(crosshairArea.right, crosshairArea.bottom + 10), separatorBrush, 1.0f);
		    separatorBrush->Release();

		    currentY += 210; // Move to next section position
		}

		// Motion Blur settings section
		if (SettingsMatchesSearch(L"Motion Blur")) {
		    int yPos = currentY + 20;
		    RECT motionBlurArea = {60 + 40, yPos, 60 + 40 + 600, yPos + 140};
		    m_renderTarget->DrawText(L"Motion Blur", 11, m_titleFormat, D2D1::RectF(motionBlurArea.left + 16, motionBlurArea.top + 12, motionBlurArea.right, motionBlurArea.top + 40), m_textBrush);
		    
		    int motionBlurTrackLeft = motionBlurArea.left + 40;
		    int motionBlurTrackRight = motionBlurArea.right - 40;
		    int motionBlurTrackY = motionBlurArea.top + 80;
		    m_motionBlurTrackRect = {motionBlurTrackLeft, motionBlurTrackY, motionBlurTrackRight, motionBlurTrackY + 4};
		    ID2D1SolidColorBrush* motionBlurTrackBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &motionBlurTrackBrush);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_motionBlurTrackRect.left, m_motionBlurTrackRect.top, m_motionBlurTrackRect.right, m_motionBlurTrackRect.bottom), 2, 2), motionBlurTrackBrush);
		    motionBlurTrackBrush->Release();
		    ID2D1SolidColorBrush* motionBlurTrackBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &motionBlurTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_motionBlurTrackRect.left, m_motionBlurTrackRect.top, m_motionBlurTrackRect.right, m_motionBlurTrackRect.bottom), 2, 2), motionBlurTrackBorder, 1.0f);
		    motionBlurTrackBorder->Release();
		    int motionBlurKnobX = motionBlurTrackLeft + (int)((motionBlurTrackRight - motionBlurTrackLeft) * m_motionBlurSlider + 0.5f);
		    RECT motionBlurKnob = {motionBlurKnobX - 8, motionBlurTrackY - 8, motionBlurKnobX + 8, motionBlurTrackY + 8};
		    m_motionBlurKnobRect = motionBlurKnob;
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F((float)motionBlurKnobX, (float)motionBlurTrackY), 8.0f, 8.0f), m_whiteBrush);
		    wchar_t motionBlurLabel[64];
		    swprintf(motionBlurLabel, 64, L"Blur Intensity: %.2f", m_motionBlurSlider);
		    m_renderTarget->DrawText(motionBlurLabel, (UINT32)wcslen(motionBlurLabel), m_titleFormat, D2D1::RectF(motionBlurArea.left + 16, motionBlurTrackY + 20, motionBlurArea.right, motionBlurTrackY + 44), m_textBrush);

		    // Separator line
		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(motionBlurArea.left, motionBlurArea.bottom + 10), D2D1::Point2F(motionBlurArea.right, motionBlurArea.bottom + 10), separatorBrush, 1.0f);
		    separatorBrush->Release();

		    currentY += 140;
		}

		if (SettingsMatchesSearch(L"Quick Turn")) {
		    int yPos = currentY + 40;
		    RECT quickTurnArea = {60 + 40, yPos, 60 + 40 + 600, yPos + 240};
		    m_renderTarget->DrawText(L"Quick Turn", 10, m_titleFormat, D2D1::RectF(quickTurnArea.left + 16, quickTurnArea.top + 12, quickTurnArea.right, quickTurnArea.top + 40), m_textBrush);

		    RECT quickTurnLabelRect = {quickTurnArea.left + 16, quickTurnArea.top + 52, quickTurnArea.left + 140, quickTurnArea.top + 76};
		    m_renderTarget->DrawText(L"Quick Turn", 10, m_titleFormat, D2D1::RectF((FLOAT)quickTurnLabelRect.left, (FLOAT)quickTurnLabelRect.top, (FLOAT)quickTurnLabelRect.right, (FLOAT)quickTurnLabelRect.bottom), m_textBrush);
		    int btnW = 60;
		    int btnH = 24;
		    int btnLeft = quickTurnLabelRect.right + 12;
		    int rowCenterY = (quickTurnLabelRect.top + quickTurnLabelRect.bottom) / 2;
		    int btnTop = rowCenterY - btnH / 2;
		    m_quickTurnSetButtonRect = {btnLeft, btnTop, btnLeft + btnW, btnTop + btnH};
		    ID2D1SolidColorBrush* quickTurnBtnBg = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &quickTurnBtnBg);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_quickTurnSetButtonRect.left, (FLOAT)m_quickTurnSetButtonRect.top, (FLOAT)m_quickTurnSetButtonRect.right, (FLOAT)m_quickTurnSetButtonRect.bottom), 6, 6), quickTurnBtnBg);
		    quickTurnBtnBg->Release();
		    ID2D1SolidColorBrush* quickTurnBtnBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &quickTurnBtnBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_quickTurnSetButtonRect.left, (FLOAT)m_quickTurnSetButtonRect.top, (FLOAT)m_quickTurnSetButtonRect.right, (FLOAT)m_quickTurnSetButtonRect.bottom), 6, 6), quickTurnBtnBorder, 1.0f);
		    quickTurnBtnBorder->Release();
		    if (!m_quickTurnKey.empty()) {
			    ID2D1SolidColorBrush* btnTextBrush = nullptr;
			    if (m_keyCaptureActive && m_quickTurnCapturing) {
				    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
			    } else {
				    btnTextBrush = m_textBrush;
			    }
			    m_renderTarget->DrawText(m_quickTurnKey.c_str(), (UINT32)m_quickTurnKey.size(), m_titleFormat, D2D1::RectF((FLOAT)m_quickTurnSetButtonRect.left, (FLOAT)m_quickTurnSetButtonRect.top, (FLOAT)m_quickTurnSetButtonRect.right, (FLOAT)m_quickTurnSetButtonRect.bottom), btnTextBrush);
			    if (m_keyCaptureActive && m_quickTurnCapturing && btnTextBrush != m_textBrush) {
				    btnTextBrush->Release();
			    }
		    }

		    // Toggle mode slider (Hold/Toggle)
		    int toggleLabelLeft = m_quickTurnSetButtonRect.right + 24;
		    m_renderTarget->DrawText(L"Toggle", 6, m_titleFormat, D2D1::RectF((FLOAT)toggleLabelLeft, (FLOAT)quickTurnLabelRect.top, (FLOAT)toggleLabelLeft + 60, (FLOAT)quickTurnLabelRect.bottom), m_textBrush);
		    int toggleWidth = 40;
		    int toggleHeight = 20;
		    int toggleLeft = toggleLabelLeft + 68;
		    int toggleTop = rowCenterY - toggleHeight / 2;
		    m_quickTurnToggleRect = {toggleLeft, toggleTop, toggleLeft + toggleWidth, toggleTop + toggleHeight};

		    // Toggle track background
		    ID2D1SolidColorBrush* toggleTrackBg = nullptr;
		    if (m_quickTurnToggleMode) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &toggleTrackBg);
		    } else {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &toggleTrackBg);
		    }
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_quickTurnToggleRect.left, (FLOAT)m_quickTurnToggleRect.top, (FLOAT)m_quickTurnToggleRect.right, (FLOAT)m_quickTurnToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBg);
		    toggleTrackBg->Release();
		    ID2D1SolidColorBrush* toggleTrackBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &toggleTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF((FLOAT)m_quickTurnToggleRect.left, (FLOAT)m_quickTurnToggleRect.top, (FLOAT)m_quickTurnToggleRect.right, (FLOAT)m_quickTurnToggleRect.bottom), toggleHeight / 2.0f, toggleHeight / 2.0f), toggleTrackBorder, 1.0f);
		    toggleTrackBorder->Release();

		    // Toggle knob
		    int knobSize = 16;
		    float knobX = m_quickTurnToggleMode ? (m_quickTurnToggleRect.right - knobSize / 2.0f - 2.0f) : (m_quickTurnToggleRect.left + knobSize / 2.0f + 2.0f);
		    float knobY = (m_quickTurnToggleRect.top + m_quickTurnToggleRect.bottom) / 2.0f;
		    ID2D1SolidColorBrush* knobBrush = nullptr;
		    if (m_quickTurnToggleMode) {
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &knobBrush);
		    } else {
		        knobBrush = m_whiteBrush;
		        knobBrush->AddRef();
		    }
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, knobY), knobSize / 2.0f, knobSize / 2.0f), knobBrush);
		    knobBrush->Release();

		    int sensitivityTrackLeft = quickTurnArea.left + 40;
		    int sensitivityTrackRight = quickTurnArea.right - 40;
		    int sensitivityTrackY = quickTurnArea.top + 102;
		    m_quickTurnSensitivityTrackRect = {sensitivityTrackLeft, sensitivityTrackY, sensitivityTrackRight, sensitivityTrackY + 4};
		    ID2D1SolidColorBrush* sensitivityTrackBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &sensitivityTrackBrush);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_quickTurnSensitivityTrackRect.left, m_quickTurnSensitivityTrackRect.top, m_quickTurnSensitivityTrackRect.right, m_quickTurnSensitivityTrackRect.bottom), 2, 2), sensitivityTrackBrush);
		    sensitivityTrackBrush->Release();
		    ID2D1SolidColorBrush* sensitivityTrackBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &sensitivityTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_quickTurnSensitivityTrackRect.left, m_quickTurnSensitivityTrackRect.top, m_quickTurnSensitivityTrackRect.right, m_quickTurnSensitivityTrackRect.bottom), 2, 2), sensitivityTrackBorder, 1.0f);
		    sensitivityTrackBorder->Release();
		    int sensitivityKnobX = sensitivityTrackLeft + (int)((sensitivityTrackRight - sensitivityTrackLeft) * m_quickTurnSensitivitySlider + 0.5f);
		    RECT sensitivityKnob = {sensitivityKnobX - 8, sensitivityTrackY - 8, sensitivityKnobX + 8, sensitivityTrackY + 8};
		    m_quickTurnSensitivityKnobRect = sensitivityKnob;
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F((float)sensitivityKnobX, (float)sensitivityTrackY), 8.0f, 8.0f), m_whiteBrush);
		    wchar_t sensitivityLabel[64];
		    swprintf(sensitivityLabel, 64, L"Mouse Sensitivity: %.2f", m_quickTurnSensitivitySlider);
		    m_renderTarget->DrawText(sensitivityLabel, (UINT32)wcslen(sensitivityLabel), m_titleFormat, D2D1::RectF(quickTurnArea.left + 16, sensitivityTrackY + 20, quickTurnArea.right, sensitivityTrackY + 44), m_textBrush);
		    
		    int degreesTrackLeft = quickTurnArea.left + 40;
		    int degreesTrackRight = quickTurnArea.right - 40;
		    int degreesTrackY = quickTurnArea.top + 162;
		    m_quickTurnDegreesTrackRect = {degreesTrackLeft, degreesTrackY, degreesTrackRight, degreesTrackY + 4};
		    ID2D1SolidColorBrush* degreesTrackBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &degreesTrackBrush);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_quickTurnDegreesTrackRect.left, m_quickTurnDegreesTrackRect.top, m_quickTurnDegreesTrackRect.right, m_quickTurnDegreesTrackRect.bottom), 2, 2), degreesTrackBrush);
		    degreesTrackBrush->Release();
		    ID2D1SolidColorBrush* degreesTrackBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &degreesTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_quickTurnDegreesTrackRect.left, m_quickTurnDegreesTrackRect.top, m_quickTurnDegreesTrackRect.right, m_quickTurnDegreesTrackRect.bottom), 2, 2), degreesTrackBorder, 1.0f);
		    degreesTrackBorder->Release();
		    int degreesKnobX = degreesTrackLeft + (int)((degreesTrackRight - degreesTrackLeft) * m_quickTurnDegreesSlider + 0.5f);
		    RECT degreesKnob = {degreesKnobX - 8, degreesTrackY - 8, degreesKnobX + 8, degreesTrackY + 8};
		    m_quickTurnDegreesKnobRect = degreesKnob;
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F((float)degreesKnobX, (float)degreesTrackY), 8.0f, 8.0f), m_whiteBrush);
		    float degreesValue = m_quickTurnDegreesSlider * 360.0f;
		    wchar_t degreesLabel[64];
		    swprintf(degreesLabel, 64, L"Target Degrees: %.0f\u00B0", degreesValue);
		    m_renderTarget->DrawText(degreesLabel, (UINT32)wcslen(degreesLabel), m_titleFormat, D2D1::RectF(quickTurnArea.left + 16, degreesTrackY + 20, quickTurnArea.right, degreesTrackY + 44), m_textBrush);

		    // Separator line
		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(quickTurnArea.left, quickTurnArea.bottom - 5), D2D1::Point2F(quickTurnArea.right, quickTurnArea.bottom - 5), separatorBrush, 1.0f);
		    separatorBrush->Release();

		    currentY += 240;
		}

		if (SettingsMatchesSearch(L"Gamma")) {
		    int yPos = currentY + 40;
		    RECT gammaArea = {60 + 40, yPos, 60 + 40 + 600, yPos + 140};
		    m_renderTarget->DrawText(L"Gamma", 5, m_titleFormat, D2D1::RectF(gammaArea.left + 16, gammaArea.top + 12, gammaArea.right, gammaArea.top + 40), m_textBrush);
		    
		    int trackLeft = gammaArea.left + 40;
		    int trackRight = gammaArea.right - 40;
		    int trackY = gammaArea.top + 80;
		    m_gammaSliderTrackRect = {trackLeft, trackY, trackRight, trackY + 4};
		    ID2D1SolidColorBrush* gammaTrackBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &gammaTrackBrush);
		    m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_gammaSliderTrackRect.left, m_gammaSliderTrackRect.top, m_gammaSliderTrackRect.right, m_gammaSliderTrackRect.bottom), 2, 2), gammaTrackBrush);
		    gammaTrackBrush->Release();
		    ID2D1SolidColorBrush* gammaTrackBorder = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &gammaTrackBorder);
		    m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(m_gammaSliderTrackRect.left, m_gammaSliderTrackRect.top, m_gammaSliderTrackRect.right, m_gammaSliderTrackRect.bottom), 2, 2), gammaTrackBorder, 1.0f);
		    gammaTrackBorder->Release();
		    int knobX = trackLeft + (int)((trackRight - trackLeft) * m_gammaSlider + 0.5f);
		    RECT knob = {knobX - 8, trackY - 8, knobX + 8, trackY + 8};
		    m_gammaSliderKnobRect = knob;
		    m_renderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F((float)knobX, (float)trackY), 8.0f, 8.0f), m_whiteBrush);
		    float gammaValue = 0.1f + m_gammaSlider * 3.9f;
		    wchar_t label[64];
		    swprintf(label, 64, L"Gamma: %.2f", gammaValue);
		    m_renderTarget->DrawText(label, (UINT32)wcslen(label), m_titleFormat, D2D1::RectF(gammaArea.left + 16, trackY + 20, gammaArea.right, trackY + 44), m_textBrush);

		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(gammaArea.left, gammaArea.bottom + 5), D2D1::Point2F(gammaArea.right, gammaArea.bottom), separatorBrush, 1.0f);
		    separatorBrush->Release();

		    currentY += 140;
		}
		
		// Auto Wisp section
		if (SettingsMatchesSearch(L"Auto Wisp")) {
		    int yPos = currentY + 50;
		    int sectionHeight = 60 + (int)m_wispSequences.size() * 50; // Base height + 50px per sequence
		    RECT autoWispArea = {60 + 40, yPos, 60 + 40 + 600, yPos + sectionHeight};
		    m_renderTarget->DrawText(L"Auto Wisp", 9, m_titleFormat, D2D1::RectF(autoWispArea.left + 16, autoWispArea.top + 12, autoWispArea.right, autoWispArea.top + 40), m_textBrush);
		    
		    int rowY = autoWispArea.top + 50;
		    int charBoxSize = 24; // Small box for one character
		    int charBoxSpacing = 4; // Small space between boxes
		    int btnW = 60;
		    int btnH = 24;
		    int iconBtnSize = 24;
		    int spacing = 12;
		    
		    for (size_t i = 0; i < m_wispSequences.size(); i++) {
		        auto& seq = m_wispSequences[i];
		        
		        // Ensure at least one char box exists
		        if (seq.charBoxes.empty()) {
		            seq.charBoxes.push_back(WispCharBox());
		        }
		        
		        // Render character boxes
		        int charBoxX = autoWispArea.left + 16;
		        int rowCenterY = rowY + 14; // Center of row
		        int charBoxY = rowCenterY - charBoxSize / 2;
		        
		        for (size_t j = 0; j < seq.charBoxes.size(); j++) {
		            auto& charBox = seq.charBoxes[j];
		            charBox.rect = {charBoxX, charBoxY, charBoxX + charBoxSize, charBoxY + charBoxSize};
		            
		            ID2D1SolidColorBrush* charBoxBg = nullptr;
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &charBoxBg);
		            m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(charBox.rect.left, charBox.rect.top, charBox.rect.right, charBox.rect.bottom), 4, 4), charBoxBg);
		            charBoxBg->Release();
		            
		            ID2D1SolidColorBrush* charBoxBorder = nullptr;
		            float borderOpacity = (seq.isEditing && charBox.focused) ? 0.3f : 0.05f;
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, borderOpacity), &charBoxBorder);
		            m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(charBox.rect.left, charBox.rect.top, charBox.rect.right, charBox.rect.bottom), 4, 4), charBoxBorder, 1.0f);
		            charBoxBorder->Release();
		            
		            // Draw character if exists
		            if (charBox.ch != 0) {
		                wchar_t chStr[2] = {charBox.ch, 0};
		                m_renderTarget->DrawText(chStr, 1, m_titleFormat, D2D1::RectF(charBox.rect.left, charBox.rect.top, charBox.rect.right, charBox.rect.bottom), m_textBrush);
		            } else if (seq.isEditing && charBox.focused && m_wispCursorVisible) {
		                // Draw cursor
		                float cursorX = charBox.rect.left + charBoxSize / 2.0f;
		                ID2D1SolidColorBrush* cursorBrush = nullptr;
		                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &cursorBrush);
		                m_renderTarget->DrawLine(D2D1::Point2F(cursorX, charBox.rect.top + 4), D2D1::Point2F(cursorX, charBox.rect.bottom - 4), cursorBrush, 2.0f);
		                cursorBrush->Release();
		            }
		            
		            charBoxX += charBoxSize + charBoxSpacing;
		        }
		        
		        // Set Key button
		        int setKeyBtnLeft = charBoxX + spacing;
		        int setKeyBtnTop = rowCenterY - btnH / 2;
		        seq.setKeyButtonRect = {setKeyBtnLeft, setKeyBtnTop, setKeyBtnLeft + btnW, setKeyBtnTop + btnH};
		        ID2D1SolidColorBrush* setKeyBtnBg = nullptr;
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &setKeyBtnBg);
		        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(seq.setKeyButtonRect.left, seq.setKeyButtonRect.top, seq.setKeyButtonRect.right, seq.setKeyButtonRect.bottom), 6, 6), setKeyBtnBg);
		        setKeyBtnBg->Release();
		        ID2D1SolidColorBrush* setKeyBtnBorder = nullptr;
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &setKeyBtnBorder);
		        m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(seq.setKeyButtonRect.left, seq.setKeyButtonRect.top, seq.setKeyButtonRect.right, seq.setKeyButtonRect.bottom), 6, 6), setKeyBtnBorder, 1.0f);
		        setKeyBtnBorder->Release();
		        if (!seq.key.empty()) {
		            ID2D1SolidColorBrush* btnTextBrush = nullptr;
		            if (seq.capturingKey) {
		                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x888888), &btnTextBrush);
		            } else {
		                btnTextBrush = m_textBrush;
		            }
		            m_renderTarget->DrawText(seq.key.c_str(), (UINT32)seq.key.size(), m_titleFormat, D2D1::RectF(seq.setKeyButtonRect.left, seq.setKeyButtonRect.top, seq.setKeyButtonRect.right, seq.setKeyButtonRect.bottom), btnTextBrush);
		            if (seq.capturingKey && btnTextBrush != m_textBrush) {
		                btnTextBrush->Release();
		            }
		        } else {
		            m_renderTarget->DrawText(L"Set Key", 7, m_titleFormat, D2D1::RectF(seq.setKeyButtonRect.left, seq.setKeyButtonRect.top, seq.setKeyButtonRect.right, seq.setKeyButtonRect.bottom), m_textBrush);
		        }
		        
		        // Plus button (only show on last row)
		        int nextBtnX = seq.setKeyButtonRect.right + spacing;
		        if (i == m_wispSequences.size() - 1) {
		            int plusBtnTop = rowCenterY - iconBtnSize / 2;
		            seq.plusButtonRect = {nextBtnX, plusBtnTop, nextBtnX + iconBtnSize, plusBtnTop + iconBtnSize};
		            ID2D1SolidColorBrush* plusBtnBg = nullptr;
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &plusBtnBg);
		            m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(seq.plusButtonRect.left, seq.plusButtonRect.top, seq.plusButtonRect.right, seq.plusButtonRect.bottom), 6, 6), plusBtnBg);
		            plusBtnBg->Release();
		            ID2D1SolidColorBrush* plusBtnBorder = nullptr;
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &plusBtnBorder);
		            m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(seq.plusButtonRect.left, seq.plusButtonRect.top, seq.plusButtonRect.right, seq.plusButtonRect.bottom), 6, 6), plusBtnBorder, 1.0f);
		            plusBtnBorder->Release();
		            float plusCX = seq.plusButtonRect.left + iconBtnSize / 2.0f;
		            float plusCY = seq.plusButtonRect.top + iconBtnSize / 2.0f;
		            float plusHalf = 8.0f;
		            m_renderTarget->DrawLine(D2D1::Point2F(plusCX - plusHalf, plusCY), D2D1::Point2F(plusCX + plusHalf, plusCY), m_textBrush, 2.0f);
		            m_renderTarget->DrawLine(D2D1::Point2F(plusCX, plusCY - plusHalf), D2D1::Point2F(plusCX, plusCY + plusHalf), m_textBrush, 2.0f);
		            nextBtnX += iconBtnSize + spacing;
		        }
		        
		        // Delete button (can't delete first sequence)
		        if (i > 0) {
		            int deleteBtnTop = rowCenterY - iconBtnSize / 2;
		            seq.deleteButtonRect = {nextBtnX, deleteBtnTop, nextBtnX + iconBtnSize, deleteBtnTop + iconBtnSize};
		            ID2D1SolidColorBrush* deleteBtnBg = nullptr;
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &deleteBtnBg);
		            m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(seq.deleteButtonRect.left, seq.deleteButtonRect.top, seq.deleteButtonRect.right, seq.deleteButtonRect.bottom), 6, 6), deleteBtnBg);
		            deleteBtnBg->Release();
		            ID2D1SolidColorBrush* deleteBtnBorder = nullptr;
		            m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &deleteBtnBorder);
		            m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(seq.deleteButtonRect.left, seq.deleteButtonRect.top, seq.deleteButtonRect.right, seq.deleteButtonRect.bottom), 6, 6), deleteBtnBorder, 1.0f);
		            deleteBtnBorder->Release();
		            DrawXIcon(seq.deleteButtonRect, 10.5f, m_textBrush);
		            nextBtnX += iconBtnSize + spacing;
		        }
		        
		        // Refresh button
		        int refreshBtnTop = rowCenterY - iconBtnSize / 2;
		        seq.refreshButtonRect = {nextBtnX, refreshBtnTop, nextBtnX + iconBtnSize, refreshBtnTop + iconBtnSize};
		        ID2D1SolidColorBrush* refreshBtnBg = nullptr;
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.6f), &refreshBtnBg);
		        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(seq.refreshButtonRect.left, seq.refreshButtonRect.top, seq.refreshButtonRect.right, seq.refreshButtonRect.bottom), 6, 6), refreshBtnBg);
		        refreshBtnBg->Release();
		        ID2D1SolidColorBrush* refreshBtnBorder = nullptr;
		        m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &refreshBtnBorder);
		        m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(seq.refreshButtonRect.left, seq.refreshButtonRect.top, seq.refreshButtonRect.right, seq.refreshButtonRect.bottom), 6, 6), refreshBtnBorder, 1.0f);
		        refreshBtnBorder->Release();
		        
		        // Draw refresh icon (circular arrow)
		        float refreshX = seq.refreshButtonRect.left + iconBtnSize / 2.0f;
		        float refreshY = seq.refreshButtonRect.top + iconBtnSize / 2.0f;
		        float refreshRadius = 7.0f;
		        ID2D1PathGeometry* refreshPath = nullptr;
		        if (SUCCEEDED(m_d2dFactory->CreatePathGeometry(&refreshPath))) {
		            ID2D1GeometrySink* sink = nullptr;
		            if (SUCCEEDED(refreshPath->Open(&sink))) {
		                float startAngle = 3.6f;
		                float sweepAngle = 5.5f;
		                int numSegments = 50;
		                
		                float startX = refreshX + refreshRadius * cosf(startAngle);
		                float startY = refreshY + refreshRadius * sinf(startAngle);
		                sink->BeginFigure(D2D1::Point2F(startX, startY), D2D1_FIGURE_BEGIN_HOLLOW);
		                
		                for (int k = 1; k <= numSegments; k++) {
		                    float t = (float)k / (float)numSegments;
		                    float angle = startAngle + t * sweepAngle;
		                    float x = refreshX + refreshRadius * cosf(angle);
		                    float y = refreshY + refreshRadius * sinf(angle);
		                    sink->AddLine(D2D1::Point2F(x, y));
		                }
		                
		                sink->EndFigure(D2D1_FIGURE_END_OPEN);
		                
		                // Arrow head
		                float arrowAngle = startAngle;
		                float arrowSize = 3.0f;
		                float arrowSpreadLeft = 0.9f;
		                float arrowSpreadRight = 0.7f;
		                
		                float dirX = -sinf(arrowAngle);
		                float dirY = cosf(arrowAngle);
		                
		                float arrowTip1X = startX + arrowSize * (dirX * cosf(arrowSpreadLeft) - dirY * sinf(arrowSpreadLeft));
		                float arrowTip1Y = startY + arrowSize * (dirX * sinf(arrowSpreadLeft) + dirY * cosf(arrowSpreadLeft));
		                float arrowTip2X = startX + arrowSize * (dirX * cosf(-arrowSpreadRight) - dirY * sinf(-arrowSpreadRight));
		                float arrowTip2Y = startY + arrowSize * (dirX * sinf(-arrowSpreadRight) + dirY * cosf(-arrowSpreadRight));
		                
		                sink->BeginFigure(D2D1::Point2F(arrowTip1X, arrowTip1Y), D2D1_FIGURE_BEGIN_HOLLOW);
		                sink->AddLine(D2D1::Point2F(startX, startY));
		                sink->AddLine(D2D1::Point2F(arrowTip2X, arrowTip2Y));
		                sink->EndFigure(D2D1_FIGURE_END_OPEN);
		                
		                sink->Close();
		                sink->Release();
		            }
		            ID2D1SolidColorBrush* refreshIconBrush = m_textBrush;
		            m_renderTarget->DrawGeometry(refreshPath, refreshIconBrush, 1.5f);
		            refreshPath->Release();
		        }
		        
		        rowY += 50;
		    }
		    
		    // Separator line
		    ID2D1SolidColorBrush* separatorBrush = nullptr;
		    m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f), &separatorBrush);
		    m_renderTarget->DrawLine(D2D1::Point2F(autoWispArea.left, autoWispArea.bottom + 10), D2D1::Point2F(autoWispArea.right, autoWispArea.bottom + 10), separatorBrush, 1.0f);
		    separatorBrush->Release();
		    
		    currentY += sectionHeight + 20;
		}
		
		// Reset transform and pop clipping after settings content
			m_renderTarget->SetTransform(GetBaseRenderTransform());
		m_renderTarget->PopAxisAlignedClip();
		
		// Calculate max scroll offset based on content height
		float blurHeight = 140.0f; // blur section height
		float hotbarLabelHeight = 24.0f; // hotbar label height
		float hotbarSlotsHeight = 10 * 28.0f; // 10 slots * 28px each
		float hotkeysHeight = 210.0f; // hotkeys section height
		float rollM1Height = 140.0f; // roll M1 section height
		float holdM1Height = 80.0f; // hold M1 section height
		float betterParryHeight = 240.0f; // better parry section height
		float crosshairHeight = 168.0f; // crosshair section height
		float motionBlurHeight = 140.0f; // motion blur section height
		float quickTurnHeight = 240.0f; // quick turn section height
		float gammaHeight = 140.0f; // gamma section height
		float autoWispHeight = 60.0f + m_wispSequences.size() * 50.0f; // auto wisp section height (base + per sequence)
		float totalContentHeight = blurHeight + hotbarLabelHeight + hotbarSlotsHeight + hotkeysHeight + rollM1Height + holdM1Height + betterParryHeight + crosshairHeight + motionBlurHeight + quickTurnHeight + gammaHeight + autoWispHeight + 150.0f; // +150px spacing
		float visibleHeight = 400.0f; // visible area height
		m_settingsMaxScrollOffset = max(0.0f, totalContentHeight - visibleHeight + 150.0f); // +150px padding
        } else {
            m_renderTarget->PushAxisAlignedClip(D2D1::RectF(0.0f, 60.0f, kContentWidth, kContentHeight), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            
            m_renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(0, -m_modsScrollOffset) * GetBaseRenderTransform());
            
        int visibleCardIndex = 0;
        for (int i = 0; i < m_modCards.size(); i++) {
            auto& card = m_modCards[i];
            
            if (!ModMatchesSearch(card)) continue;
            // Recalculate position based on visible card index
            int col = visibleCardIndex % 4;
            int row = visibleCardIndex / 4;
            int cardWidth = 160;
            int cardHeight = 140;
            int spacing = 12;
            int contentAreaWidth = 700;
            int gridWidth = 4 * cardWidth + 3 * spacing;
            int gridStartX = 60 + (contentAreaWidth - gridWidth) / 2;
            int startY = 76;
            
            // Update card position for visible cards only
            card.rect.left = gridStartX + col * (cardWidth + spacing);
            card.rect.top = startY + row * (cardHeight + spacing);
            card.rect.right = card.rect.left + cardWidth;
            card.rect.bottom = card.rect.top + cardHeight;
            
            visibleCardIndex++;
            
            float cardOpacity = (i == 10) ? 0.4f : 1.0f;
            ID2D1SolidColorBrush* cardBg = nullptr;
            D2D1_COLOR_F bgColor = D2D1::ColorF(0x0a0a0b, 0.6f * cardOpacity);
            m_renderTarget->CreateSolidColorBrush(bgColor, &cardBg);
            m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(card.rect.left, card.rect.top, card.rect.right, card.rect.bottom), 10, 10), cardBg);
            cardBg->Release();
            
            ID2D1SolidColorBrush* cardBorder = nullptr;
            D2D1_COLOR_F borderColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.05f * cardOpacity);
            m_renderTarget->CreateSolidColorBrush(borderColor, &cardBorder);
            m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(card.rect.left, card.rect.top, card.rect.right, card.rect.bottom), 10, 10), cardBorder, 1.0f);
            cardBorder->Release();
            
            // Do not mutate opacity here; only draw from current value
            if (card.borderOpacity > 0.0f) {
                // Create brush with animated opacity
                ID2D1SolidColorBrush* animatedBorderBrush = nullptr;
                D2D1_COLOR_F borderColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, card.borderOpacity); // White with animated alpha
                m_renderTarget->CreateSolidColorBrush(borderColor, &animatedBorderBrush);
                m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(card.rect.left, card.rect.top, card.rect.right, card.rect.bottom), 8, 8), animatedBorderBrush, 1.0f);
                animatedBorderBrush->Release();
            }
            
            // Smooth lift on hover
            int lift = static_cast<int>(card.hoverLift + 0.5f);
            RECT contentRect = {card.rect.left + 20, card.rect.top + 20 - lift, card.rect.right - 20, card.rect.bottom - 20 - lift};
            
            // Icon (36x36) centered horizontally
            RECT iconRect = {contentRect.left + (contentRect.right - contentRect.left - 36) / 2, contentRect.top + 5, 
                            contentRect.left + (contentRect.right - contentRect.left - 36) / 2 + 36, contentRect.top + 5 + 36};
            
            // Icon background: modern subtle bg, white when enabled
            if (card.iconOpacity <= 0.0f) {
                ID2D1SolidColorBrush* subtleBg = nullptr;
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0x0a0a0b, 0.3f), &subtleBg);
                m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(iconRect.left, iconRect.top, iconRect.right, iconRect.bottom), 8, 8), subtleBg);
                subtleBg->Release();
            } else if (card.iconOpacity >= 1.0f) {
                m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(iconRect.left, iconRect.top, iconRect.right, iconRect.bottom), 8, 8), m_whiteBrush);
            } else {
                float grayW = 1.0f - card.iconOpacity;
                float whiteW = card.iconOpacity;
                float r = (0x0a * grayW + 0xFF * whiteW) / 255.0f;
                float g = (0x0a * grayW + 0xFF * whiteW) / 255.0f;
                float b = (0x0b * grayW + 0xFF * whiteW) / 255.0f;
                ID2D1SolidColorBrush* blend = nullptr;
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 1.0f), &blend);
                m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(iconRect.left, iconRect.top, iconRect.right, iconRect.bottom), 8, 8), blend);
                if (blend) blend->Release();
            }
            
			// Draw specific icons for mod cards
			if (i == 0) { // Roll M1 is the first card (index 0)
				DrawRollM1IconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 1) { // Parry Bar is the second card (index 1)
				DrawParryBarIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 2) { // Map Cast card (index 2)
				DrawMapCastIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 3) { // Keystrokes card
				DrawKeyboardIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 4) { // Auto Wisp card (index 4)
				DrawAutoWispIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 5) { // CPS card (index 5)
				DrawCpsIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 6) { // Roll Cast card (index 6)
				DrawRollCastIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 7) { // Hold M1 card (index 7)
				DrawHoldM1IconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 8) { // Crosshair card (index 8)
				DrawCrosshairIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 9) { // Roll Spit card (index 9)
				DrawRollSpitIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 10) { // Motion Blur card (index 10)
				DrawMotionBlurIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 11) {
				DrawZoomIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 12) {
				DrawQuickTurnIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 13) {
				DrawLightspeedReflexesIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 14) {
				DrawGoldenTongueIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			} else if (i == 15) {
				DrawGammaIconAnimated(iconRect, card.enabled, card.enabled ? 1.0f : 0.0f);
			}
            
            // Title (SemiBold, 14px) - 12px margin bottom
            RECT titleRect = {contentRect.left + 8, iconRect.bottom + 12, contentRect.right - 8, iconRect.bottom + 12 + 20};
            m_renderTarget->DrawText(card.title.c_str(), card.title.length(), m_titleFormat, D2D1::RectF(titleRect.left, titleRect.top, titleRect.right, titleRect.bottom), m_textBrush);
            
            // Description (Normal, 11px) - 8px margin bottom
            RECT descRect = { contentRect.left + 8, titleRect.bottom + 8, contentRect.right - 8, titleRect.bottom + 8 + 20};
            m_renderTarget->DrawText(card.description.c_str(), card.description.length(), m_descFormat, D2D1::RectF(descRect.left, descRect.top, descRect.right, descRect.bottom), m_descBrush);
            
            if (i == 10) {
                ID2D1SolidColorBrush* tagBg = nullptr;
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &tagBg);
                RECT tagRect = {card.rect.right - 78, card.rect.top + 8, card.rect.right - 8, card.rect.top + 24};
                m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(tagRect.left, tagRect.top, tagRect.right, tagRect.bottom), 4, 4), tagBg);
                tagBg->Release();
                
                ID2D1SolidColorBrush* tagText = nullptr;
                m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 1.0f), &tagText);
                m_renderTarget->DrawText(L"EXPERIMENTAL", 12, m_tagFormat, D2D1::RectF(tagRect.left, tagRect.top, tagRect.right, tagRect.bottom), tagText);
                tagText->Release();
            }
        }
        
            m_renderTarget->SetTransform(GetBaseRenderTransform());
            m_renderTarget->PopAxisAlignedClip();
            
            int totalRows = (int)ceil((float)visibleCardIndex / 4.0f);
            int cardHeight = 140;
            int spacing = 12;
            int startY = 76;
            float totalContentHeight = startY + totalRows * (cardHeight + spacing) - spacing;
            float visibleHeight = 400.0f;
            m_modsMaxScrollOffset = max(0.0f, totalContentHeight - visibleHeight - 130.0f);
        }
    }
    
    
    void SetupKeyboardHook() {
        m_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(nullptr), 0);
    }
    
    
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION) {
            KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
            if (LightSpeedReflex::ShouldIgnoreHookEvent(kb)) {
                return CallNextHookEx(NULL, nCode, wParam, lParam);
            }
            if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && kb->vkCode == VK_RSHIFT) {
                if (s_instance) {
                    // Only process right shift if Roblox is in focus
                    if (IsRobloxFound()) {
                        HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                        if (robloxWindow) {
                            HWND foregroundWindow = GetForegroundWindow();
                            bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                            if (robloxInFocus) {
                                // If UI is hidden, handle via LL hook (swallow and toggle show)
                                // If UI is visible, let RawInput path handle it (do NOT swallow)
                                if (!s_instance->m_showUI) {
                                    PostMessage(s_instance->m_hwnd, WM_USER + 1, 0, 0);
                                    return 1; // swallow when showing
                                }
                            }
                        }
                    }
                    // UI is visible or Roblox not in focus: don't swallow; RawInput will detect and hide
                }
                return CallNextHookEx(NULL, nCode, wParam, lParam);
            }
            
            if (s_instance && g_rollM1Running && s_instance->m_rollM1TriggerScanCode != 0) {
                if (kb->scanCode == s_instance->m_rollM1TriggerScanCode) {
                    if (!IsRobloxFound()) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                    if (!robloxWindow) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND foregroundWindow = GetForegroundWindow();
                    bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                    if (!robloxInFocus) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                        if (!RollM1::GetTriggerKeyWasPressed()) {
                            RollM1::TriggerRoll();
                            RollM1::SetTriggerKeyWasPressed(true);
                        }
                        return 1;
                    }
                    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                        RollM1::SetTriggerKeyWasPressed(false);
                        return 1;
                    }
                }
            }

            if (s_instance && g_goldenTongueRunning && s_instance->m_goldenTongueTriggerScanCode != 0) {
                if (kb->scanCode == s_instance->m_goldenTongueTriggerScanCode) {
                    if (!IsRobloxFound()) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                    if (!robloxWindow) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND foregroundWindow = GetForegroundWindow();
                    bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                    if (!robloxInFocus) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                        GoldenTongue::TriggerGoldenTongue();
                        return 1;
                    }
                    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                        return 1;
                    }
                }
            }

            if (s_instance && g_rollCriticalRunning && s_instance->m_rollCritScanCode != 0) {
                if (kb->scanCode == s_instance->m_rollCritScanCode) {
                    // Check if Roll M1 mod is enabled
                    bool rollM1ModEnabled = false;
                    if (s_instance->m_modCards.size() > 0) {
                        rollM1ModEnabled = s_instance->m_modCards[0].enabled;
                    }
                    if (!rollM1ModEnabled) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }

                    if (!IsRobloxFound()) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                    if (!robloxWindow) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND foregroundWindow = GetForegroundWindow();
                    bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                    if (!robloxInFocus) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                        if (!RollCritical::GetTriggerKeyWasPressed()) {
                            RollCritical::TriggerRollCritical();
                            RollCritical::SetTriggerKeyWasPressed(true);
                        }
                        return 1;
                    }
                    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                        RollCritical::SetTriggerKeyWasPressed(false);
                        return 1;
                    }
                }
            }

            if (s_instance && g_rollParryRunning && s_instance->m_rollParryScanCode != 0) {
                if (kb->scanCode == s_instance->m_rollParryScanCode) {
                    // Check if Better Parry mod is enabled
                    bool betterParryModEnabled = false;
                    if (s_instance->m_modCards.size() > 13) {
                        betterParryModEnabled = s_instance->m_modCards[13].enabled;
                    }
                    if (!betterParryModEnabled) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }

                    if (!IsRobloxFound()) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                    if (!robloxWindow) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND foregroundWindow = GetForegroundWindow();
                    bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                    if (!robloxInFocus) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                        if (!RollParry::GetTriggerKeyWasPressed()) {
                            RollParry::TriggerRollParry();
                            RollParry::SetTriggerKeyWasPressed(true);
                        }
                        return 1;
                    }
                    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                        RollParry::SetTriggerKeyWasPressed(false);
                        return 1;
                    }
                }
            }

            if (s_instance && s_instance->m_parryScanCode != 0) {
                if (kb->scanCode == s_instance->m_parryScanCode) {
                    bool betterParryEnabled = false;
                    if (s_instance->m_modCards.size() > 13) {
                        betterParryEnabled = s_instance->m_modCards[13].enabled;
                    }

                    if (betterParryEnabled && IsRobloxFound()) {
                        HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                        if (robloxWindow) {
                            HWND foregroundWindow = GetForegroundWindow();
                            bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                            if (robloxInFocus) {
                                bool lightspeedMacroEnabled = s_instance->m_betterParryLightspeedReflex;

                                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                                    // Check if M1 was pressed within 440ms (using actual M1 event timestamp)
                                    DWORD currentTime = GetTickCount();
                                    DWORD elapsedTime = currentTime - g_lightspeedM1PressTime;
                                    bool withinTimeWindow = (elapsedTime <= 440) && !g_lightspeedSequenceRunning;

                                    if (lightspeedMacroEnabled && withinTimeWindow) {
                                        g_lightspeedSequenceRunning = true;

                                        // Spawn thread for exact timing
                                        struct ThreadData {
                                            USHORT parryScanCode;
                                            bool shouldParry;
                                        };
                                        ThreadData* data = new ThreadData{s_instance->m_parryScanCode, s_instance->m_betterParryShouldParry};

                                        CreateThread(nullptr, 0, [](LPVOID param) -> DWORD {
                                            ThreadData* data = (ThreadData*)param;

                                            Sleep(3);

                                            // Right click
                                            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                                            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);

                                            // Wait 75ms after M2
                                            Sleep(75);

                                            // Only press F if "Should Parry" is enabled
                                            if (data->shouldParry) {
                                                INPUT inputs[2] = {};
                                                inputs[0].type = INPUT_KEYBOARD;
                                                inputs[0].ki.wScan = data->parryScanCode;
                                                inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE;
                                                inputs[1].type = INPUT_KEYBOARD;
                                                inputs[1].ki.wScan = data->parryScanCode;
                                                inputs[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
                                                SendInput(2, inputs, sizeof(INPUT));
                                            }

                                            delete data;
                                            g_lightspeedSequenceRunning = false;
                                            return 0;
                                        }, data, 0, nullptr);

                                        return 1; // Block the original F key press
                                    } else {
                                        // Normal parry logic
                                        LightSpeedReflex::OnPhysicalParryDown();
                                        LightSpeedReflex::UpdateForwarding(s_instance->m_parryScanCode, betterParryEnabled, robloxInFocus, s_instance->m_betterParryAntiShakyBlock);
                                        return 1;
                                    }
                                }
                                if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                                    // Normal parry logic
                                    LightSpeedReflex::OnPhysicalParryUp(s_instance->m_parryScanCode);
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
            
            if (s_instance && g_quickTurnRunning && s_instance->m_quickTurnTriggerScanCode != 0) {
                if (kb->scanCode == s_instance->m_quickTurnTriggerScanCode) {
                    if (!IsRobloxFound()) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                    if (!robloxWindow) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND foregroundWindow = GetForegroundWindow();
                    bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                    bool shiftLockActive = Crosshair::IsShiftLock();
                    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                        if (robloxInFocus && shiftLockActive) {
                            QuickTurn::TriggerTurn();
                            return 1;
                        }
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                        bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                        bool shiftLockActive = Crosshair::IsShiftLock();
                        if (s_instance->m_quickTurnToggleMode) {
                            // In toggle mode: reset state so next press can turn again
                            QuickTurn::ForceResetState();
                        } else {
                            // In hold mode: turn back on release
                            if (robloxInFocus && shiftLockActive) {
                                QuickTurn::ReleaseTurn();
                            } else {
                                QuickTurn::ForceResetState();
                            }
                        }
                        return 1;
                    }
                }
            }
            
            if (s_instance && g_rollSpitRunning && s_instance->m_rollSpitTriggerScanCode != 0) {
                if (kb->scanCode == s_instance->m_rollSpitTriggerScanCode) {
                    if (RollSpit::IsMacroSendingSpit()) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    if (!IsRobloxFound()) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                    if (!robloxWindow) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    HWND foregroundWindow = GetForegroundWindow();
                    bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                    if (!robloxInFocus) {
                        return CallNextHookEx(NULL, nCode, wParam, lParam);
                    }
                    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                        if (!RollSpit::GetTriggerKeyWasPressed()) {
                            RollSpit::TriggerRollSpit();
                            RollSpit::SetTriggerKeyWasPressed(true);
                        }
                        return 1;
                    }
                    if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                        RollSpit::SetTriggerKeyWasPressed(false);
                        return 1;
                    }
                }
            }

            
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    void ToggleWindow() {
        if (m_isAnimating) return;
        bool wantShow = !m_showUI;
        if (wantShow) {
            if (!IsRobloxFound()) return; // Only gate showing
            
            // Check if Roblox is in fullscreen mode
            if (WinRTCapture::IsCaptureInitialized()) {
                int clientWidth = WinRTCapture::GetCaptureWidth();
                int clientHeight = WinRTCapture::GetCaptureHeight();
                    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                    bool isFullscreen = (clientWidth == screenWidth && clientHeight == screenHeight);
                    
                    if (!isFullscreen) {
                        // Set blur fullscreen state to false and don't open window
                        SetRobloxFullscreen(false);
                        return; // Don't open window if not fullscreen
                    }
                    
                    // Set blur fullscreen state to true since we're proceeding
                    SetRobloxFullscreen(true);
            } else {
                SetRobloxFullscreen(false);
                return;
            }
            
            m_showUI = true;
            ShowWindowWithAnimation();
        } else {
            m_showUI = false;
            HideWindowWithAnimation();
        }
    }
    
    void ShowWindowWithAnimation() {
        // Check if Roblox is in focus before showing window
        HWND robloxWindow = WinRTCapture::FindRobloxWindow();
        if (robloxWindow) {
            HWND foregroundWindow = GetForegroundWindow();
            bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
            if (!robloxInFocus) {
                return; // Don't show window if Roblox is not in focus
            }
        } else {
            return; // Don't show window if Roblox is not found
        }
        
        extern BlurApp g_blurApp;
        extern float g_currentBlurIterations;
        extern float g_blurIterations;
        extern float g_targetBlurIterations;
        g_blurApp.currentIntensity = 0.0f;
        g_blurApp.targetIntensity = m_blurSlider;
        g_currentBlurIterations = 0.0f;
        g_blurIterations = 0.0f;
        g_targetBlurIterations = 5.0f;
        extern std::atomic<bool> g_forceFreshBlurFrame;
        g_forceFreshBlurFrame = true;
        
        // Show blur after animation thread starts
        SetBlurVisible(true);
        
        m_isAnimating = true;
        m_animatingIn = true;
        m_animatingOut = false;
        m_animationStep = 0;
        m_scale = 0.3f;
        m_opacity = 0.0f;
        
        // Ensure main window is on top of blur window
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int windowWidth = static_cast<int>(kWindowWidth);
        int windowHeight = static_cast<int>(kWindowHeight);
        int x = (screenW - windowWidth) / 2;
        int y = (screenH - windowHeight) / 2;
        
        SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, windowWidth, windowHeight, SWP_SHOWWINDOW);
        
        AllowSetForegroundWindow(ASFW_ANY);
        SetForegroundWindow(m_hwnd);
        keybd_event(VK_MENU, 0, 0, 0);
        keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);

        
        
        SetTimer(m_hwnd, 1, 10, nullptr);
    }
    
    void HideWindowWithAnimation() {
        // Unfocus Auto Wisp sequences when menu hides
        if (m_focusedWispSequenceIndex >= 0 && m_focusedWispSequenceIndex < (int)m_wispSequences.size()) {
            auto& seq = m_wispSequences[m_focusedWispSequenceIndex];
            seq.isEditing = false;
            seq.focusedCharIndex = -1;
            for (auto& charBox : seq.charBoxes) {
                charBox.focused = false;
            }
            // Remove empty boxes at the end
            while (seq.charBoxes.size() > 1 && seq.charBoxes.back().ch == 0) {
                seq.charBoxes.pop_back();
            }
            m_focusedWispSequenceIndex = -1;
        }
        m_isAnimating = true;
        m_animatingIn = false;
        m_animatingOut = true;
        m_animationStep = 0;
        m_scale = 1.0f;
        m_opacity = 1.0f;
        
        // Start search border fade out animation
        if (m_searchFocused) {
            m_searchAnimatingBorder = true;
            m_searchFocused = false;
            SetTimer(m_hwnd, 4, 16, nullptr); // Start border animation timer
        }
        
        if (g_crosshairRunning) {
            Crosshair::SetWindowJustHidden(true);
            DWORD currentTime = GetTickCount();
            Crosshair::SetCameraLockTimeWindow(true, currentTime);
        }
        
        // Actually disable crosshair if pending disable
        if (g_crosshairPendingDisable && g_crosshairRunning) {
            Crosshair::SetEnabled(false);
            Crosshair::StopCrosshair();
            g_crosshairRunning = false;
            g_crosshairPendingDisable = false;
        }
        
        // Blur fade-out is handled by the timer-based animation, not here
        
        // Reset animation flags
        g_animatingOut = false;
        
        SetTimer(m_hwnd, 1, 10, nullptr);
    }
    
    void UpdateAnimation() {
        if (m_animatingIn) {
            m_animationStep++;
            if (m_animationStep >= 20) {
                m_scale = 1.0f;
                m_isAnimating = false;
                m_animatingIn = false;
                KillTimer(m_hwnd, 1);
                
                {
                    AllowSetForegroundWindow(ASFW_ANY);
                    SetForegroundWindow(m_hwnd);
                    keybd_event(VK_MENU, 0, 0, 0);
                    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
                }
            } else {
                float progress = static_cast<float>(m_animationStep) / 20.0f;
                float easedProgress = CubicBezierEaseScale(progress);
                if (easedProgress < 0.0f) easedProgress = 0.0f;
                m_scale = 0.3f + (1.0f - 0.3f) * easedProgress;
                if (m_scale < 0.3f) m_scale = 0.3f;
                float maxOvershoot = 1.10f;
                if (m_scale > maxOvershoot) m_scale = maxOvershoot;
                
                // Fade in effect: start at 30% scale, reach 75% opacity by 40% scale
                if (m_scale >= 0.3f && m_scale <= 0.4f) {
                    float fadeProgress = (m_scale - 0.3f) / (0.4f - 0.3f);
                    m_opacity = 0.75f * fadeProgress;
                } else if (m_scale > 0.4f) {
                    m_opacity = 0.75f + (1.0f - 0.75f) * ((m_scale - 0.4f) / (1.0f - 0.4f));
                }
                
            }
        } else if (m_animatingOut) {
            m_animationStep++;
            if (m_animationStep >= 8) {    // Scale animation ends at step 8
                m_scale = 0.3f;
                m_isAnimating = false;
                m_animatingOut = false;
                
                // Reset search state when hide animation completes
                m_searchText.clear();
                m_cursorPosition = 0;
                m_cursorVisible = true;
                m_cursorBlinkTimer = 0;
                
                ShowWindow(m_hwnd, SW_HIDE);
                KillTimer(m_hwnd, 1);
                
                HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                if (robloxWindow) {
                    DWORD robloxThread = GetWindowThreadProcessId(robloxWindow, nullptr);
                    DWORD myThread = GetCurrentThreadId();
                    AttachThreadInput(myThread, robloxThread, TRUE);
                    SetForegroundWindow(robloxWindow);
                    SetActiveWindow(robloxWindow);
                    SetFocus(robloxWindow);
                    AttachThreadInput(myThread, robloxThread, FALSE);
                }
                
                extern void SetBlurVisible(bool visible);
                SetBlurVisible(false);
                
                if (m_exitAfterHide) {
                    if (g_gammaRunning) {
                        StopGammaMonitor();
                    }
                    ExitProcess(0);
                    return;
                }

                if (g_editHudPendingStart) {
                    SetTimer(m_hwnd, 9, 1, nullptr);
                }
                ProcessPendingStarts();
            } else {
                float progress = static_cast<float>(m_animationStep) / 8.0f;
                float easedProgress = CubicBezierEaseScale(progress);
                if (easedProgress < 0.0f) easedProgress = 0.0f;
                m_scale = 1.0f - (1.0f - 0.3f) * easedProgress;
                if (m_scale < 0.3f) m_scale = 0.3f;
                if (m_scale > 1.0f) m_scale = 1.0f;

                if (m_scale >= 0.4f) {
                    m_opacity = 0.75f + (1.0f - 0.75f) * ((m_scale - 0.4f) / (1.0f - 0.4f));
                } else if (m_scale >= 0.3f) {
                    m_opacity = 0.75f * ((m_scale - 0.3f) / (0.4f - 0.3f));
                } else {
                    m_opacity = 0.0f;
                }
            }
        }
        
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int windowWidth = static_cast<int>(kWindowWidth);
        int windowHeight = static_cast<int>(kWindowHeight);
        int x = (screenW - windowWidth) / 2;
        int y = (screenH - windowHeight) / 2;
        SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, windowWidth, windowHeight, SWP_NOZORDER);
        
        Render();
    }
    
    float CubicBezierEase(float t) {
        if (t <= 0) return 0;
        if (t >= 1) return 1;
        
        // Smoother ease-out with cubic bezier curve
        // Uses control points for more natural motion
        float oneMinusT = 1 - t;
        float oneMinusTSquared = oneMinusT * oneMinusT;
        float oneMinusTCubed = oneMinusTSquared * oneMinusT;
        
        return 1 - (oneMinusTCubed * oneMinusT);
    }

    float CubicBezierEaseScale(float t) {
        if (t <= 0) return 0;
        if (t >= 1) return 1;
        float x1 = 0.68f, y1 = -0.55f, x2 = 0.265f, y2 = 1.55f;
        auto cubic = [](float a, float b, float m) {
            float inv = 1.0f - m;
            return 3.0f * inv * inv * m * a + 3.0f * inv * m * m * b + m * m * m;
        };
        float u = t;
        for (int i = 0; i < 5; ++i) {
            float x = cubic(x1, x2, u);
            float u1 = u + 0.001f;
            if (u1 > 1.0f) u1 = 1.0f;
            float dx = (cubic(x1, x2, u1) - x) / 0.001f;
            if (dx == 0.0f) break;
            u = u - (x - t) / dx;
            if (u < 0.0f) u = 0.0f;
            if (u > 1.0f) u = 1.0f;
        }
        return cubic(y1, y2, u);
    }
	void DrawCpsIconAnimated(RECT rect, bool enabled, float animationProgress) {
		float rw = (float)(rect.right - rect.left);
		float rh = (float)(rect.bottom - rect.top);
		float vb = 24.0f;
		float s = min(rw / vb, rh / vb) * 0.65f;
		float ox = rect.left + (rw - vb * s) * 0.5f;
		float oy = rect.top + (rh - vb * s) * 0.5f;
		auto P = [&](float x, float y) { return D2D1::Point2F(ox + x * s, oy + y * s); };
		auto RR = [&](float x, float y, float w, float h, float r) {
			D2D1_POINT_2F p1 = P(x, y);
			D2D1_POINT_2F p2 = P(x + w, y + h);
			return D2D1::RoundedRect(D2D1::RectF(p1.x, p1.y, p2.x, p2.y), r * s, r * s);
		};

		float grayWeight = 1.0f - animationProgress;
		float darkWeight = animationProgress;
		float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
		float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
		float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
		ID2D1SolidColorBrush* brush = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f), &brush);
		if (!brush) return;

		float stroke = 1.5f;
		m_renderTarget->DrawRoundedRectangle(RR(5, 2, 14, 20, 7), brush, stroke);
		m_renderTarget->DrawLine(P(12, 6), P(12, 10), brush, stroke);

		brush->Release();
	}
	
	void DrawZoomIconAnimated(RECT rect, bool enabled, float animationProgress) {
		float rw = (float)(rect.right - rect.left);
		float rh = (float)(rect.bottom - rect.top);
		float size = min(rw, rh) * 0.7f;
		float left = rect.left + (rw - size) * 0.5f - 1.0f;
		float top = rect.top + (rh - size) * 0.5f;
		float centerX = left + size * 0.5f;
		float centerY = top + size * 0.5f;
		float radius = size * 0.25f;
		float handleStartX = centerX + radius * 0.70710678f;
		float handleStartY = centerY + radius * 0.70710678f;
		float handleEndX = left + size * 0.82f;
		float handleEndY = top + size * 0.82f;
		float grayWeight = 1.0f - animationProgress;
		float darkWeight = animationProgress;
		float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
		float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
		float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
		ID2D1SolidColorBrush* brush = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f), &brush);
		if (!brush) return;
		ID2D1PathGeometry* pathGeometry = nullptr;
		if (FAILED(m_d2dFactory->CreatePathGeometry(&pathGeometry))) {
			brush->Release();
			return;
		}
		ID2D1GeometrySink* sink = nullptr;
		if (FAILED(pathGeometry->Open(&sink))) {
			pathGeometry->Release();
			brush->Release();
			return;
		}
		sink->BeginFigure(D2D1::Point2F(centerX + radius, centerY), D2D1_FIGURE_BEGIN_HOLLOW);
		sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(centerX - radius, centerY), D2D1::SizeF(radius, radius), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
		sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(centerX + radius, centerY), D2D1::SizeF(radius, radius), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
		sink->EndFigure(D2D1_FIGURE_END_OPEN);
		sink->BeginFigure(D2D1::Point2F(handleStartX, handleStartY), D2D1_FIGURE_BEGIN_HOLLOW);
		sink->AddLine(D2D1::Point2F(handleEndX, handleEndY));
		sink->EndFigure(D2D1_FIGURE_END_OPEN);
		sink->Close();
		sink->Release();
		float strokeWidth = max(1.4f, size * 0.062f);
		m_renderTarget->DrawGeometry(pathGeometry, brush, strokeWidth);
		pathGeometry->Release();
		brush->Release();
	}
	
	void DrawMotionBlurIconAnimated(RECT rect, bool enabled, float animationProgress) {
		float rw = (float)(rect.right - rect.left);
		float rh = (float)(rect.bottom - rect.top);
		float vb = 5.0f;
		float s = (min(rw, rh) / vb) * 0.5f;
		float ox = rect.left + (rw - vb * s) * 0.5f - 1.0f;
		float oy = rect.top + (rh - vb * s) * 0.5f;
		auto P = [&](float x, float y) { return D2D1::Point2F(ox + x * s, oy + y * s); };
		auto Fill = [&](float x, float y, float w, float h) {
			return D2D1::RectF(P(x, y).x, P(x, y).y, P(x + w, y + h).x, P(x + w, y + h).y);
		};
		
		float grayWeight = 1.0f - animationProgress;
		float darkWeight = animationProgress;
		float finalRed = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
		float finalGreen = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
		float finalBlue = (0x88 * grayWeight + 0x00 * darkWeight) / 255.0f;
		ID2D1SolidColorBrush* brush = nullptr;
		m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(finalRed, finalGreen, finalBlue, 1.0f), &brush);
		if (!brush) return;
		
		m_renderTarget->FillRectangle(Fill(0.72728664f, 0.425354f, 1.0f, 1.0f), brush);
		m_renderTarget->FillRectangle(Fill(2.6883276f, 0.41561422f, 1.0f, 1.0f), brush);
		m_renderTarget->FillRectangle(Fill(0.73048222f, 2.3506315f, 1.0f, 1.0f), brush);
		m_renderTarget->FillRectangle(Fill(1.7175468f, 1.3961347f, 1.0f, 1.0f), brush);
		m_renderTarget->FillRectangle(Fill(3.6785879f, 1.4058745f, 1.0f, 1.0f), brush);
		m_renderTarget->FillRectangle(Fill(1.7597015f, 3.2921929f, 1.0f, 1.0f), brush);
		m_renderTarget->FillRectangle(Fill(2.7078071f, 2.3669155f, 1.0f, 1.0f), brush);
		m_renderTarget->FillRectangle(Fill(3.7044585f, 3.3376961f, 1.0f, 1.0f), brush);
		
		brush->Release();
	}
    
public:
    void Run() {
        ShowWindow(m_hwnd, SW_HIDE);
        
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        PistachioCreamMacro* app = nullptr;
        
        if (message == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            app = reinterpret_cast<PistachioCreamMacro*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        } else {
            app = reinterpret_cast<PistachioCreamMacro*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }
        
        if (app) {
            switch (message) {

                case WM_NCHITTEST: {
                    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    ScreenToClient(hwnd, &pt);
                    if (!app->IsPointWithinVisibleContent(pt)) {
                        return HTTRANSPARENT;
                    }
                    return HTCLIENT;
                }
                    
                case WM_PAINT:
                    app->Render();
                    ValidateRect(hwnd, nullptr);
                    return 0;
                    
                case WM_CHAR:
                    if (app && app->m_showUI) {
                        // Handle Auto Wisp text box input
                        if (app->m_focusedWispSequenceIndex >= 0 && app->m_focusedWispSequenceIndex < (int)app->m_wispSequences.size()) {
                            auto& seq = app->m_wispSequences[app->m_focusedWispSequenceIndex];
                            if (wParam == 8) { // Backspace
                                if (seq.focusedCharIndex >= 0 && seq.focusedCharIndex < (int)seq.charBoxes.size()) {
                                    if (seq.charBoxes[seq.focusedCharIndex].ch != 0) {
                                        seq.charBoxes[seq.focusedCharIndex].ch = 0;
                                        // Remove empty text box if it's not the last one
                                        if (seq.charBoxes.size() > 1) {
                                            seq.charBoxes.erase(seq.charBoxes.begin() + seq.focusedCharIndex);
                                            // Adjust focus index
                                            if (seq.focusedCharIndex >= (int)seq.charBoxes.size()) {
                                                seq.focusedCharIndex = (int)seq.charBoxes.size() - 1;
                                            }
                                            if (seq.focusedCharIndex >= 0 && seq.focusedCharIndex < (int)seq.charBoxes.size()) {
                                                seq.charBoxes[seq.focusedCharIndex].focused = true;
                                            }
                                        }
                                    } else if (seq.focusedCharIndex > 0) {
                                        // Move to previous box and delete it if it has a character
                                        seq.focusedCharIndex--;
                                        seq.charBoxes[seq.focusedCharIndex].focused = true;
                                        if (seq.focusedCharIndex + 1 < (int)seq.charBoxes.size()) {
                                            seq.charBoxes[seq.focusedCharIndex + 1].focused = false;
                                        }
                                        if (seq.charBoxes[seq.focusedCharIndex].ch != 0) {
                                            seq.charBoxes[seq.focusedCharIndex].ch = 0;
                                            // Remove empty text box if it's not the last one
                                            if (seq.charBoxes.size() > 1) {
                                                seq.charBoxes.erase(seq.charBoxes.begin() + seq.focusedCharIndex);
                                                // Adjust focus index
                                                if (seq.focusedCharIndex >= (int)seq.charBoxes.size()) {
                                                    seq.focusedCharIndex = (int)seq.charBoxes.size() - 1;
                                                }
                                                if (seq.focusedCharIndex >= 0 && seq.focusedCharIndex < (int)seq.charBoxes.size()) {
                                                    seq.charBoxes[seq.focusedCharIndex].focused = true;
                                                }
                                            }
                                        }
                                    }
                                }
                                InvalidateRect(hwnd, nullptr, FALSE);
                            } else if (wParam == 27) { // Escape key - finish editing
                                seq.isEditing = false;
                                seq.focusedCharIndex = -1;
                                for (auto& charBox : seq.charBoxes) {
                                    charBox.focused = false;
                                }
                                // Remove empty boxes at the end
                                while (seq.charBoxes.size() > 1 && seq.charBoxes.back().ch == 0) {
                                    seq.charBoxes.pop_back();
                                }
                                app->m_focusedWispSequenceIndex = -1;
                                app->MarkSettingsDirty();
                                InvalidateRect(hwnd, nullptr, FALSE);
                            } else if (wParam == 13) { // Enter key - finish editing
                                seq.isEditing = false;
                                seq.focusedCharIndex = -1;
                                for (auto& charBox : seq.charBoxes) {
                                    charBox.focused = false;
                                }
                                // Remove empty boxes at the end
                                while (seq.charBoxes.size() > 1 && seq.charBoxes.back().ch == 0) {
                                    seq.charBoxes.pop_back();
                                }
                                app->m_focusedWispSequenceIndex = -1;
                                app->MarkSettingsDirty();
                                InvalidateRect(hwnd, nullptr, FALSE);
                            } else if ((wParam >= 'a' && wParam <= 'z') || (wParam >= 'A' && wParam <= 'Z')) { // Only a-z and A-Z
                                wchar_t ch = (wchar_t)wParam;
                                if (ch >= 'a' && ch <= 'z') {
                                    ch = ch - 'a' + 'A';
                                }
                                
                                if (seq.focusedCharIndex >= 0 && seq.focusedCharIndex < (int)seq.charBoxes.size()) {
                                    seq.charBoxes[seq.focusedCharIndex].ch = ch;
                                    seq.charBoxes[seq.focusedCharIndex].focused = false;
                                    
                                    // Move to next char box or create new one
                                    seq.focusedCharIndex++;
                                    if (seq.focusedCharIndex >= (int)seq.charBoxes.size()) {
                                        seq.charBoxes.push_back(WispCharBox());
                                    }
                                    seq.charBoxes[seq.focusedCharIndex].focused = true;
                                }
                                app->MarkSettingsDirty();
                                InvalidateRect(hwnd, nullptr, FALSE);
                            }
                            return 0;
                        }
                        
                        // Handle search bar input
                        if (app->m_searchFocused) {
                            if (wParam == 8) { // Backspace
                                if (app->m_cursorPosition > 0) {
                                    app->m_searchText.erase(app->m_cursorPosition - 1, 1);
                                    app->m_cursorPosition--;
                                    if (!app->m_searchText.empty()) {
                                        app->m_settingsScrollOffset = 0.0f;
                                        app->m_modsScrollOffset = 0.0f;
                                    }
                                    InvalidateRect(hwnd, nullptr, FALSE);
                                }
                            } else if (wParam == 27) { // Escape key
                                app->m_searchText.clear();
                                app->m_cursorPosition = 0;
                                app->m_settingsScrollOffset = 0.0f;
                                app->m_modsScrollOffset = 0.0f;
                                InvalidateRect(hwnd, nullptr, FALSE);
                            } else if (wParam == 13) { // Enter key
                                if (app->m_searchFocused) {
                                    app->m_searchAnimatingBorder = true;
                                }
                                app->m_searchFocused = false;
                                SetTimer(hwnd, 4, 16, nullptr); // Border animation timer
                                InvalidateRect(hwnd, nullptr, FALSE);
                            } else if (wParam >= 32 && wParam <= 126) { // Printable characters
                                app->m_searchText.insert(app->m_cursorPosition, 1, (wchar_t)wParam);
                                app->m_cursorPosition++;
                                app->m_settingsScrollOffset = 0.0f;
                                app->m_modsScrollOffset = 0.0f;
                                // Ensure timer is running for cursor blinking
                                SetTimer(hwnd, 1, 16, nullptr);
                                InvalidateRect(hwnd, nullptr, FALSE);
                            }
                        }
                    }
                    return 0;
                    
				case PistachioCreamMacro::kSettingsLoadedMessage:
					if (app) app->HandleSettingsLoadedMessage();
					return 0;
				
                case WM_TIMER:
                    if (wParam == 1) {
                        app->UpdateAnimation();
                        // Cursor blinking for search bar
                        if (app->m_searchFocused) {
                            app->m_cursorBlinkTimer++;
                            if (app->m_cursorBlinkTimer >= 20) { // Blink every 20 frames (333ms at 60fps)
                                app->m_cursorVisible = !app->m_cursorVisible;
                                app->m_cursorBlinkTimer = 0;
                                InvalidateRect(hwnd, nullptr, FALSE);
                            }
                        } else {
                            // Reset cursor when not focused
                            app->m_cursorVisible = true;
                            app->m_cursorBlinkTimer = 0;
                        }
                        
                        // Cursor blinking for Auto Wisp
                        if (app->m_focusedWispSequenceIndex >= 0 && app->m_focusedWispSequenceIndex < (int)app->m_wispSequences.size()) {
                            auto& seq = app->m_wispSequences[app->m_focusedWispSequenceIndex];
                            if (seq.isEditing) {
                                app->m_wispCursorBlinkTimer++;
                                if (app->m_wispCursorBlinkTimer >= 20) { // Blink every 20 frames
                                    app->m_wispCursorVisible = !app->m_wispCursorVisible;
                                    app->m_wispCursorBlinkTimer = 0;
                                    InvalidateRect(hwnd, nullptr, FALSE);
                                }
                            }
                        } else {
                            app->m_wispCursorVisible = true;
                            app->m_wispCursorBlinkTimer = 0;
                        }
                        
                        bool betterParryEnabled = false;
                        if (app->m_modCards.size() > 13) {
                            betterParryEnabled = app->m_modCards[13].enabled;
                        }
                        if (betterParryEnabled && app->m_parryScanCode != 0 && IsRobloxFound()) {
                            HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                            if (robloxWindow) {
                                HWND foregroundWindow = GetForegroundWindow();
                                bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
                                LightSpeedReflex::UpdateForwarding(app->m_parryScanCode, betterParryEnabled, robloxInFocus, app->m_betterParryAntiShakyBlock);
                            }
                        }
                    } else if (wParam == 2) {
                        return 0;
                    } else if (wParam == 5) {
                        if (app->m_quickTurnSensitivityAutoUpdate) {
                            float newSensitivity = app->ReadRobloxMouseSensitivity();
                            if (fabsf(newSensitivity - app->m_quickTurnSensitivitySlider) > 0.001f) {
                                app->m_quickTurnSensitivitySlider = newSensitivity;
                                if (g_quickTurnRunning) {
                                    QuickTurn::SetSensitivity(newSensitivity);
                                }
                                InvalidateRect(hwnd, nullptr, FALSE);
                            }
                        }
                        // Retry pending-start mods periodically (covers the case where Roblox wasn't running yet at startup).
                        app->ProcessPendingStarts();
                        return 0;
                    } else if (wParam == PistachioCreamMacro::kSettingsSaveTimerId) {
                        if (app && app->m_settingsDirty) {
                            DWORD now = GetTickCount();
                            if (now - app->m_lastSettingsChangeTick >= PistachioCreamMacro::kSettingsSaveDebounceMs) {
                                app->QueueSettingsSave();
                                app->m_settingsDirty = false;
                                KillTimer(hwnd, PistachioCreamMacro::kSettingsSaveTimerId);
                            }
                        } else {
                            KillTimer(hwnd, PistachioCreamMacro::kSettingsSaveTimerId);
                        }
                        return 0;
                    } else if (wParam == 9) {
                        KillTimer(hwnd, 9);
                        if (g_editHudPendingStart) {
                            g_editHudPendingStart = false;
                            EditHUD::StartEditor();
                        }
                        return 0;
                    } else if (wParam == 4) {
                        // Border and icon animation timer
                        bool stillAnimating = false;
                        for (size_t idx = 0; idx < app->m_modCards.size(); idx++) {
                            auto& card = app->m_modCards[idx];
                            if (idx == 10) continue;
                            if (!card.animatingBorder) continue;
                            const float animationSpeed = 0.25f;
                            bool targetOn = card.enabled || card.hovered;
                            if (targetOn) {
                                if (card.borderOpacity < 1.0f) {
                                    card.borderOpacity += animationSpeed;
                                    if (card.borderOpacity >= 1.0f) {
                                        card.borderOpacity = 1.0f;
                                        card.animatingBorder = false;
                                    }
                                    stillAnimating = true;
                                } else {
                                    card.animatingBorder = false;
                                }
                            } else {
                                if (card.borderOpacity > 0.0f) {
                                    card.borderOpacity -= animationSpeed;
                                    if (card.borderOpacity <= 0.0f) {
                                        card.borderOpacity = 0.0f;
                                        card.animatingBorder = false;
                                    }
                                    stillAnimating = true;
                                } else {
                                    card.animatingBorder = false;
                                }
                            }
                        }
                        
                        // Search border animation
                        if (app->m_searchAnimatingBorder) {
                            const float animationSpeed = 0.25f;
                            bool targetOn = app->m_searchFocused;
                            if (targetOn) {
                                if (app->m_searchBorderOpacity < 0.1f) {
                                    app->m_searchBorderOpacity += animationSpeed * 0.05f; // Scale to go from 0.05 to 0.1
                                    if (app->m_searchBorderOpacity >= 0.1f) {
                                        app->m_searchBorderOpacity = 0.1f;
                                        app->m_searchAnimatingBorder = false;
                                    }
                                    stillAnimating = true;
                                } else {
                                    app->m_searchAnimatingBorder = false;
                                }
                            } else {
                                if (app->m_searchBorderOpacity > 0.05f) {
                                    app->m_searchBorderOpacity -= animationSpeed * 0.05f; // Scale to go from 0.1 to 0.05
                                    if (app->m_searchBorderOpacity <= 0.05f) {
                                        app->m_searchBorderOpacity = 0.05f;
                                        app->m_searchAnimatingBorder = false;
                                    }
                                    stillAnimating = true;
                                } else {
                                    app->m_searchAnimatingBorder = false;
                                }
                            }
                        }
                        
                        for (auto& card : app->m_modCards) {
                            float targetIcon = card.enabled ? 1.0f : 0.0f;
                            if (card.iconOpacity < targetIcon) {
                                card.iconOpacity += 0.25f;
                                if (card.iconOpacity >= targetIcon) card.iconOpacity = targetIcon;
                                stillAnimating = true;
                            } else if (card.iconOpacity > targetIcon) {
                                card.iconOpacity -= 0.25f;
                                if (card.iconOpacity <= targetIcon) card.iconOpacity = targetIcon;
                                stillAnimating = true;
                            }
                        }
                        for (size_t idx = 0; idx < app->m_modCards.size(); idx++) {
                            auto& card = app->m_modCards[idx];
                            if (idx == 10) {
                                card.hoverLift = 0.0f;
                                continue;
                            }
                            float targetLift = card.hovered ? 2.0f : 0.0f;
                            float liftSpeed = 0.5f;
                            if (card.hoverLift < targetLift) {
                                card.hoverLift += liftSpeed;
                                if (card.hoverLift > targetLift) card.hoverLift = targetLift;
                                stillAnimating = true;
                            } else if (card.hoverLift > targetLift) {
                                card.hoverLift -= liftSpeed;
                                if (card.hoverLift < targetLift) card.hoverLift = targetLift;
                                stillAnimating = true;
                            }
                        }
                        
                        float panelsTargetScale = app->m_panelsButtonHovered ? 0.70f : 0.85f;
                        float scaleSpeed = 0.08f;
                        if (app->m_panelsButtonScale < panelsTargetScale) {
                            app->m_panelsButtonScale += scaleSpeed;
                            if (app->m_panelsButtonScale > panelsTargetScale) app->m_panelsButtonScale = panelsTargetScale;
                            stillAnimating = true;
                        } else if (app->m_panelsButtonScale > panelsTargetScale) {
                            app->m_panelsButtonScale -= scaleSpeed;
                            if (app->m_panelsButtonScale < panelsTargetScale) app->m_panelsButtonScale = panelsTargetScale;
                            stillAnimating = true;
                        }
                        
                        float exitTargetScale = app->m_exitButtonHovered ? 0.70f : 0.85f;
                        if (app->m_exitButtonScale < exitTargetScale) {
                            app->m_exitButtonScale += scaleSpeed;
                            if (app->m_exitButtonScale > exitTargetScale) app->m_exitButtonScale = exitTargetScale;
                            stillAnimating = true;
                        } else if (app->m_exitButtonScale > exitTargetScale) {
                            app->m_exitButtonScale -= scaleSpeed;
                            if (app->m_exitButtonScale < exitTargetScale) app->m_exitButtonScale = exitTargetScale;
                            stillAnimating = true;
                        }
                        
                        bool modsSelected = !app->m_showSettings && !app->m_showProfile;
                        float modsTargetScale = (modsSelected || app->m_modsIconHovered) ? 1.15f : 1.0f;
                        if (app->m_modsIconScale < modsTargetScale) {
                            app->m_modsIconScale += scaleSpeed;
                            if (app->m_modsIconScale > modsTargetScale) app->m_modsIconScale = modsTargetScale;
                            stillAnimating = true;
                        } else if (app->m_modsIconScale > modsTargetScale) {
                            app->m_modsIconScale -= scaleSpeed;
                            if (app->m_modsIconScale < modsTargetScale) app->m_modsIconScale = modsTargetScale;
                            stillAnimating = true;
                        }
                        
                        bool userSelected = app->m_showProfile;
                        float userTargetScale = (userSelected || app->m_userIconHovered) ? 1.15f : 1.0f;
                        if (app->m_userIconScale < userTargetScale) {
                            app->m_userIconScale += scaleSpeed;
                            if (app->m_userIconScale > userTargetScale) app->m_userIconScale = userTargetScale;
                            stillAnimating = true;
                        } else if (app->m_userIconScale > userTargetScale) {
                            app->m_userIconScale -= scaleSpeed;
                            if (app->m_userIconScale < userTargetScale) app->m_userIconScale = userTargetScale;
                            stillAnimating = true;
                        }
                        
                        bool settingsSelected = app->m_showSettings;
                        float settingsTargetScale = (settingsSelected || app->m_settingsIconHovered) ? 1.15f : 1.0f;
                        if (app->m_settingsIconScale < settingsTargetScale) {
                            app->m_settingsIconScale += scaleSpeed;
                            if (app->m_settingsIconScale > settingsTargetScale) app->m_settingsIconScale = settingsTargetScale;
                            stillAnimating = true;
                        } else if (app->m_settingsIconScale > settingsTargetScale) {
                            app->m_settingsIconScale -= scaleSpeed;
                            if (app->m_settingsIconScale < settingsTargetScale) app->m_settingsIconScale = settingsTargetScale;
                            stillAnimating = true;
                        }
                        
                        if (stillAnimating) {
                            InvalidateRect(hwnd, nullptr, FALSE);
                        } else {
                            KillTimer(hwnd, 4); // Stop animation timer
                        }
                    }
                    return 0;
                    
				case WM_INPUT:
					app->HandleRawInput((HRAWINPUT)lParam);
					return 0;

				case WM_USER + 1:
                    app->ToggleWindow();
                    return 0;
                    
                case WM_MOUSEMOVE: {
                    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
                    pt = app->MapPointToLogical(pt);
                    bool leftButtonDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                    bool showIBeam = false;
                    
                    if (app->m_draggingSlider) {
                        if (!leftButtonDown) {
                            app->m_draggingSlider = false;
                            return 0;
                        }
                        int left = app->m_sliderTrackRect.left;
                        int right = app->m_sliderTrackRect.right;
                        float t = (pt.x - left) / float(right - left);
                        if (t < 0) t = 0; if (t > 1) t = 1;
                        app->m_blurSlider = t;
                        SetBlurIntensity(t);
                        SetCursor(LoadCursor(nullptr, IDC_HAND));
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    if (app->m_draggingCrosshairScale) {
                        if (!leftButtonDown) {
                            app->m_draggingCrosshairScale = false;
                            return 0;
                        }
                        int left = app->m_crosshairScaleTrackRect.left;
                        int right = app->m_crosshairScaleTrackRect.right;
                        float t = (pt.x - left) / float(right - left);
                        if (t < 0) t = 0; if (t > 1) t = 1;
                        app->m_crosshairScale = t;
                        Crosshair::UpdateCrosshairScale(app->m_crosshairScale);
                        SetCursor(LoadCursor(nullptr, IDC_HAND));
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    if (app->m_draggingMotionBlur) {
                        if (!leftButtonDown) {
                            app->m_draggingMotionBlur = false;
                            return 0;
                        }
                        int left = app->m_motionBlurTrackRect.left;
                        int right = app->m_motionBlurTrackRect.right;
                        float t = (pt.x - left) / float(right - left);
                        if (t < 0) t = 0; if (t > 1) t = 1;
                        app->m_motionBlurSlider = t;
                        MotionBlur::SetMotionBlurIntensity(powf(t, 0.35f) * 1.5f);
                        SetCursor(LoadCursor(nullptr, IDC_HAND));
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    if (app->m_draggingQuickTurnSensitivity) {
                        if (!leftButtonDown) {
                            app->m_draggingQuickTurnSensitivity = false;
                            return 0;
                        }
                        int left = app->m_quickTurnSensitivityTrackRect.left;
                        int right = app->m_quickTurnSensitivityTrackRect.right;
                        float t = (pt.x - left) / float(right - left);
                        if (t < 0) t = 0; if (t > 1) t = 1;
                        app->m_quickTurnSensitivitySlider = t;
                        app->m_quickTurnSensitivityAutoUpdate = false;
                        if (g_quickTurnRunning) {
                            QuickTurn::SetSensitivity(t);
                        }
                        SetCursor(LoadCursor(nullptr, IDC_HAND));
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    if (app->m_draggingGammaSlider) {
                        if (!leftButtonDown) {
                            app->m_draggingGammaSlider = false;
                            return 0;
                        }
                        int left = app->m_gammaSliderTrackRect.left;
                        int right = app->m_gammaSliderTrackRect.right;
                        float t = (pt.x - left) / float(right - left);
                        if (t < 0) t = 0; if (t > 1) t = 1;
                        app->m_gammaSlider = t;
                        float gammaValue = 0.1f + t * 3.9f;
                        g_gammaTarget.store(gammaValue);
                        if (g_gammaRunning && g_gammaEnabled.load()) {
                            HWND robloxWindow = WinRTCapture::FindRobloxWindow();
                            HWND fg = GetForegroundWindow();
                            if (robloxWindow && (fg == robloxWindow || GetParent(fg) == robloxWindow)) {
                                SetGamma(gammaValue);
                            }
                        }
                        SetCursor(LoadCursor(nullptr, IDC_HAND));
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    if (app->m_draggingQuickTurnDegrees) {
                        if (!leftButtonDown) {
                            app->m_draggingQuickTurnDegrees = false;
                            return 0;
                        }
                        int left = app->m_quickTurnDegreesTrackRect.left;
                        int right = app->m_quickTurnDegreesTrackRect.right;
                        float t = (pt.x - left) / float(right - left);
                        if (t < 0) t = 0; if (t > 1) t = 1;
                        app->m_quickTurnDegreesSlider = t;
                        if (g_quickTurnRunning) {
                            float degreesValue = t * 360.0f;
                            QuickTurn::SetTargetDegrees(degreesValue);
                        }
                        SetCursor(LoadCursor(nullptr, IDC_HAND));
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    bool showHand = false;
                    
                    // Check if mouse is over close button
                    if (PtInRect(&app->m_closeButtonRect, pt)) {
                        showHand = true;
                    }
                    
                    // Check if mouse is over panels button
                    bool wasPanelsHovered = app->m_panelsButtonHovered;
                    app->m_panelsButtonHovered = PtInRect(&app->m_panelsButtonRect, pt);
                    if (app->m_panelsButtonHovered) {
                        showHand = true;
                        if (!wasPanelsHovered) {
                            SetTimer(hwnd, 4, 16, nullptr);
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                    } else if (wasPanelsHovered) {
                        SetTimer(hwnd, 4, 16, nullptr);
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    
                    // Check if mouse is over exit button
                    bool wasHovered = app->m_exitButtonHovered;
                    app->m_exitButtonHovered = PtInRect(&app->m_exitButtonRect, pt);
                    if (app->m_exitButtonHovered) {
                        showHand = true;
                        if (!wasHovered) {
                            SetTimer(hwnd, 4, 16, nullptr);
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                    } else if (wasHovered) {
                        SetTimer(hwnd, 4, 16, nullptr);
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    
                    // Check if mouse is over search bar
                    if (PtInRect(&app->m_searchRect, pt)) {
                        SetCursor(LoadCursor(nullptr, IDC_IBEAM));
                        return 0;
                    }
                    
                    if (PtInRect(&app->m_settingsRect, pt)) showHand = true;
                    if (PtInRect(&app->m_modsRect, pt)) showHand = true;
                    if (PtInRect(&app->m_profileRect, pt)) showHand = true;
                    
                    bool wasModsHovered = app->m_modsIconHovered;
                    bool wasUserHovered = app->m_userIconHovered;
                    bool wasSettingsHovered = app->m_settingsIconHovered;
                    bool modsSelected = !app->m_showSettings && !app->m_showProfile;
                    bool userSelected = app->m_showProfile;
                    bool settingsSelected = app->m_showSettings;
                    app->m_modsIconHovered = PtInRect(&app->m_modsRect, pt) && !modsSelected;
                    app->m_userIconHovered = PtInRect(&app->m_profileRect, pt) && !userSelected;
                    app->m_settingsIconHovered = PtInRect(&app->m_settingsRect, pt) && !settingsSelected;
                    if (app->m_modsIconHovered != wasModsHovered || app->m_userIconHovered != wasUserHovered || app->m_settingsIconHovered != wasSettingsHovered) {
                        SetTimer(hwnd, 4, 16, nullptr);
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    if (!app->m_showSettings && !app->m_showProfile) {
                        bool hoverChanged = false;
                        for (size_t i = 0; i < app->m_modCards.size(); ++i) {
                            RECT adjustedModCardRect = app->m_modCards[i].rect;
                            adjustedModCardRect.top -= (int)app->m_modsScrollOffset;
                            adjustedModCardRect.bottom -= (int)app->m_modsScrollOffset;
                            bool inside = PtInRect(&adjustedModCardRect, pt);
                            if (inside) showHand = true;
                            if (i != 10 && app->m_modCards[i].hovered != inside) {
                                app->m_modCards[i].hovered = inside;
                                app->m_modCards[i].animatingBorder = true;
                                SetTimer(hwnd, 4, 16, nullptr);
                                hoverChanged = true;
                            }
                        }
                        if (hoverChanged) {
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
                        TrackMouseEvent(&tme);
					} else {
                        RECT adjustedSliderTrack = app->m_sliderTrackRect;
                        RECT adjustedSliderKnob = app->m_sliderKnobRect;
                        adjustedSliderTrack.top -= (int)app->m_settingsScrollOffset;
                        adjustedSliderTrack.bottom -= (int)app->m_settingsScrollOffset;
                        adjustedSliderKnob.top -= (int)app->m_settingsScrollOffset;
                        adjustedSliderKnob.bottom -= (int)app->m_settingsScrollOffset;
                        if (PtInRect(&adjustedSliderTrack, pt) || PtInRect(&adjustedSliderKnob, pt)) {
                            showHand = true;
                        }
						RECT adjustedRollCriticalToggleRect = app->m_rollCriticalToggleRect;
						RECT adjustedRollM1Rect = app->m_rollM1SetButtonRect;
						RECT adjustedRollCritRect = app->m_rollCritSetButtonRect;
						RECT adjustedRollSpitTriggerRect = app->m_rollSpitTriggerSetButtonRect;
						RECT adjustedRollSpitSpitRect = app->m_rollSpitSpitSetButtonRect;
						RECT adjustedRollCastRect = app->m_rollCastSetButtonRect;
					RECT adjustedZoomRect = app->m_zoomSetButtonRect;
					RECT adjustedDodgeRect = app->m_dodgeSetButtonRect;
					RECT adjustedParryRect = app->m_parrySetButtonRect;
					RECT adjustedOpenMapRect = app->m_openMapSetButtonRect;
					RECT adjustedCameraLockRect = app->m_cameraLockSetButtonRect;
					RECT adjustedCrosshairFileRect = app->m_crosshairFileButtonRect;
					RECT adjustedCrosshairScaleTrackRect = app->m_crosshairScaleTrackRect;
					RECT adjustedCrosshairScaleKnobRect = app->m_crosshairScaleKnobRect;
					RECT adjustedMotionBlurTrackRect = app->m_motionBlurTrackRect;
					RECT adjustedMotionBlurKnobRect = app->m_motionBlurKnobRect;
					adjustedRollCriticalToggleRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollCriticalToggleRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollM1Rect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollM1Rect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollCritRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollCritRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollSpitTriggerRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollSpitTriggerRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollSpitSpitRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollSpitSpitRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollCastRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollCastRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedZoomRect.top -= (int)app->m_settingsScrollOffset;
					adjustedZoomRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedDodgeRect.top -= (int)app->m_settingsScrollOffset;
					adjustedDodgeRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedParryRect.top -= (int)app->m_settingsScrollOffset;
					adjustedParryRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedOpenMapRect.top -= (int)app->m_settingsScrollOffset;
					adjustedOpenMapRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedCameraLockRect.top -= (int)app->m_settingsScrollOffset;
					adjustedCameraLockRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairFileRect.top -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairFileRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairScaleTrackRect.top -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairScaleTrackRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairScaleKnobRect.top -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairScaleKnobRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedMotionBlurTrackRect.top -= (int)app->m_settingsScrollOffset;
					adjustedMotionBlurTrackRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedMotionBlurKnobRect.top -= (int)app->m_settingsScrollOffset;
					adjustedMotionBlurKnobRect.bottom -= (int)app->m_settingsScrollOffset;
					RECT adjustedQuickTurnSensitivityTrackRect2 = app->m_quickTurnSensitivityTrackRect;
					RECT adjustedQuickTurnSensitivityKnobRect2 = app->m_quickTurnSensitivityKnobRect;
					RECT adjustedQuickTurnDegreesTrackRect2 = app->m_quickTurnDegreesTrackRect;
					RECT adjustedQuickTurnDegreesKnobRect2 = app->m_quickTurnDegreesKnobRect;
					adjustedQuickTurnSensitivityTrackRect2.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnSensitivityTrackRect2.bottom -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnSensitivityKnobRect2.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnSensitivityKnobRect2.bottom -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnDegreesTrackRect2.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnDegreesTrackRect2.bottom -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnDegreesKnobRect2.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnDegreesKnobRect2.bottom -= (int)app->m_settingsScrollOffset;
					RECT adjustedQuickTurnSetButtonRect = app->m_quickTurnSetButtonRect;
					adjustedQuickTurnSetButtonRect.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnSetButtonRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedRollCriticalToggleRect, pt)) showHand = true;
					if (PtInRect(&adjustedRollM1Rect, pt)) showHand = true;
					if (PtInRect(&adjustedRollCritRect, pt)) showHand = true;
					if (PtInRect(&adjustedRollSpitTriggerRect, pt)) showHand = true;
					if (PtInRect(&adjustedRollSpitSpitRect, pt)) showHand = true;
					if (PtInRect(&adjustedRollCastRect, pt)) showHand = true;
					if (PtInRect(&adjustedZoomRect, pt)) showHand = true;
					if (PtInRect(&adjustedDodgeRect, pt)) showHand = true;
					if (PtInRect(&adjustedParryRect, pt)) showHand = true;
					if (PtInRect(&adjustedOpenMapRect, pt)) showHand = true;
					if (PtInRect(&adjustedCameraLockRect, pt)) showHand = true;
					if (PtInRect(&adjustedCrosshairFileRect, pt)) showHand = true;
					if (PtInRect(&adjustedCrosshairScaleTrackRect, pt) || PtInRect(&adjustedCrosshairScaleKnobRect, pt)) showHand = true;
					if (PtInRect(&adjustedMotionBlurTrackRect, pt) || PtInRect(&adjustedMotionBlurKnobRect, pt)) showHand = true;
					if (PtInRect(&adjustedQuickTurnSensitivityTrackRect2, pt) || PtInRect(&adjustedQuickTurnSensitivityKnobRect2, pt)) showHand = true;
					if (PtInRect(&adjustedQuickTurnDegreesTrackRect2, pt) || PtInRect(&adjustedQuickTurnDegreesKnobRect2, pt)) showHand = true;
					if (PtInRect(&adjustedQuickTurnSetButtonRect, pt)) showHand = true;
					RECT adjustedQuickTurnToggleRect = app->m_quickTurnToggleRect;
					adjustedQuickTurnToggleRect.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnToggleRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedQuickTurnToggleRect, pt)) showHand = true;
					
					RECT adjustedHoldM1ToggleRect = app->m_holdM1ToggleRect;
					adjustedHoldM1ToggleRect.top -= (int)app->m_settingsScrollOffset;
					adjustedHoldM1ToggleRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedHoldM1ToggleRect, pt)) {
						showHand = true;
					}
					
					RECT adjustedBetterParryAntiShakyToggleRect = app->m_betterParryAntiShakyBlockToggleRect;
					adjustedBetterParryAntiShakyToggleRect.top -= (int)app->m_settingsScrollOffset;
					adjustedBetterParryAntiShakyToggleRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedBetterParryAntiShakyToggleRect, pt)) {
						showHand = true;
					}
					
					RECT adjustedBetterParryLightspeedToggleRect = app->m_betterParryLightspeedReflexToggleRect;
					adjustedBetterParryLightspeedToggleRect.top -= (int)app->m_settingsScrollOffset;
					adjustedBetterParryLightspeedToggleRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedBetterParryLightspeedToggleRect, pt)) {
						showHand = true;
					}
					
					RECT adjustedBetterParryShouldParryToggleRect = app->m_betterParryShouldParryToggleRect;
					adjustedBetterParryShouldParryToggleRect.top -= (int)app->m_settingsScrollOffset;
					adjustedBetterParryShouldParryToggleRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedBetterParryShouldParryToggleRect, pt)) {
						showHand = true;
					}

					RECT adjustedRollParryToggleRect = app->m_rollParryToggleRect;
					adjustedRollParryToggleRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollParryToggleRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedRollParryToggleRect, pt)) {
						showHand = true;
					}

					RECT adjustedRollParryButtonRect = app->m_rollParrySetButtonRect;
					adjustedRollParryButtonRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollParryButtonRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedRollParryButtonRect, pt)) {
						showHand = true;
					}

					for (int i = 0; i < 10; i++) {
							RECT adjustedRect = app->m_hotbarSlotSetButtonRect[i];
							adjustedRect.top -= (int)app->m_settingsScrollOffset;
							adjustedRect.bottom -= (int)app->m_settingsScrollOffset;
							if (PtInRect(&adjustedRect, pt)) {
								showHand = true;
								break;
							}
							
							RECT adjustedToggleRect = app->m_hotbarSlotToggleRect[i];
							adjustedToggleRect.top -= (int)app->m_settingsScrollOffset;
							adjustedToggleRect.bottom -= (int)app->m_settingsScrollOffset;
							if (PtInRect(&adjustedToggleRect, pt)) {
								showHand = true;
								break;
							}
							
							RECT adjustedMapCastToggleRect = app->m_hotbarSlotMapCastToggleRect[i];
							adjustedMapCastToggleRect.top -= (int)app->m_settingsScrollOffset;
							adjustedMapCastToggleRect.bottom -= (int)app->m_settingsScrollOffset;
							if (PtInRect(&adjustedMapCastToggleRect, pt)) {
								showHand = true;
								break;
							}
						}
						
						// Auto Wisp cursor handling
						for (size_t wispIdx = 0; wispIdx < app->m_wispSequences.size(); wispIdx++) {
							const auto& seq = app->m_wispSequences[wispIdx];
							
							// Check character boxes - use I-beam cursor (text cursor)
							for (const auto& charBox : seq.charBoxes) {
								RECT adjustedCharBoxRect = charBox.rect;
								adjustedCharBoxRect.top -= (int)app->m_settingsScrollOffset;
								adjustedCharBoxRect.bottom -= (int)app->m_settingsScrollOffset;
								if (PtInRect(&adjustedCharBoxRect, pt)) {
									showIBeam = true;
									break;
								}
							}
							if (showIBeam) break;
							
							// Check buttons - use hand cursor
							RECT adjustedSetKeyRect = seq.setKeyButtonRect;
							adjustedSetKeyRect.top -= (int)app->m_settingsScrollOffset;
							adjustedSetKeyRect.bottom -= (int)app->m_settingsScrollOffset;
							if (PtInRect(&adjustedSetKeyRect, pt)) {
								showHand = true;
								break;
							}
							
							RECT adjustedPlusRect = seq.plusButtonRect;
							adjustedPlusRect.top -= (int)app->m_settingsScrollOffset;
							adjustedPlusRect.bottom -= (int)app->m_settingsScrollOffset;
							if (PtInRect(&adjustedPlusRect, pt)) {
								showHand = true;
								break;
							}
							
							RECT adjustedDeleteRect = seq.deleteButtonRect;
							adjustedDeleteRect.top -= (int)app->m_settingsScrollOffset;
							adjustedDeleteRect.bottom -= (int)app->m_settingsScrollOffset;
							if (PtInRect(&adjustedDeleteRect, pt)) {
								showHand = true;
								break;
							}
							
							RECT adjustedRefreshRect = seq.refreshButtonRect;
							adjustedRefreshRect.top -= (int)app->m_settingsScrollOffset;
							adjustedRefreshRect.bottom -= (int)app->m_settingsScrollOffset;
							if (PtInRect(&adjustedRefreshRect, pt)) {
								showHand = true;
								break;
							}
						}
                    }
                    
                    // Set cursor
                    if (showIBeam) {
                        SetCursor(LoadCursor(nullptr, IDC_IBEAM));
                    } else {
                        SetCursor(LoadCursor(nullptr, showHand ? IDC_HAND : IDC_ARROW));
                    }
                    return 0;
                }

                case WM_MOUSELEAVE: {
                    bool hoverChanged = false;
                    for (size_t idx = 0; idx < app->m_modCards.size(); idx++) {
                        auto& card = app->m_modCards[idx];
                        if (idx == 10) continue;
                        if (card.hovered) {
                            card.hovered = false;
                            card.animatingBorder = true;
                            hoverChanged = true;
                        }
                    }
                    if (hoverChanged) {
                        SetTimer(hwnd, 4, 16, nullptr);
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                }

                case WM_SETCURSOR: {
                    if (LOWORD(lParam) == HTCLIENT) {
                        POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
                        pt = app->MapPointToLogical(pt);
                        bool showHand = false;
                        bool showIBeam = false;
                        if (PtInRect(&app->m_closeButtonRect, pt)) showHand = true;
                        if (PtInRect(&app->m_settingsRect, pt)) showHand = true;
                        if (PtInRect(&app->m_modsRect, pt)) showHand = true;
                        
                        bool wasModsHovered = app->m_modsIconHovered;
                        bool wasUserHovered = app->m_userIconHovered;
                        bool wasSettingsHovered = app->m_settingsIconHovered;
                        bool modsSelected = !app->m_showSettings && !app->m_showProfile;
                        bool userSelected = app->m_showProfile;
                        bool settingsSelected = app->m_showSettings;
                        app->m_modsIconHovered = PtInRect(&app->m_modsRect, pt) && !modsSelected;
                        app->m_userIconHovered = PtInRect(&app->m_profileRect, pt) && !userSelected;
                        app->m_settingsIconHovered = PtInRect(&app->m_settingsRect, pt) && !settingsSelected;
                        if (app->m_modsIconHovered != wasModsHovered || app->m_userIconHovered != wasUserHovered || app->m_settingsIconHovered != wasSettingsHovered) {
                            SetTimer(hwnd, 4, 16, nullptr);
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                        
                        // Check if mouse is over search bar
                        if (PtInRect(&app->m_searchRect, pt)) {
                            SetCursor(LoadCursor(nullptr, IDC_IBEAM));
                            return TRUE;
                        }
                        if (!app->m_showSettings && !app->m_showProfile) {
                            for (size_t idx = 0; idx < app->m_modCards.size(); idx++) {
                                const auto& card = app->m_modCards[idx];
                                RECT r = card.rect;
                                r.top -= (int)app->m_modsScrollOffset;
                                r.bottom -= (int)app->m_modsScrollOffset;
                                if (idx != 10 && PtInRect(&r, pt)) { showHand = true; break; }
                            }
						} else {
                        RECT adjustedSliderTrack2 = app->m_sliderTrackRect;
                        RECT adjustedSliderKnob2 = app->m_sliderKnobRect;
                        adjustedSliderTrack2.top -= (int)app->m_settingsScrollOffset;
                        adjustedSliderTrack2.bottom -= (int)app->m_settingsScrollOffset;
                        adjustedSliderKnob2.top -= (int)app->m_settingsScrollOffset;
                        adjustedSliderKnob2.bottom -= (int)app->m_settingsScrollOffset;
                        if (PtInRect(&adjustedSliderTrack2, pt) || PtInRect(&adjustedSliderKnob2, pt)) showHand = true;
							RECT adjustedRollCriticalToggleRect2 = app->m_rollCriticalToggleRect;
							RECT adjustedRollM1Rect2 = app->m_rollM1SetButtonRect;
							RECT adjustedRollCritRect2 = app->m_rollCritSetButtonRect;
							RECT adjustedDodgeRect2 = app->m_dodgeSetButtonRect;
							RECT adjustedParryRect2 = app->m_parrySetButtonRect;
							RECT adjustedOpenMapRect2 = app->m_openMapSetButtonRect;
							RECT adjustedCriticalAttackRect2 = app->m_criticalAttackSetButtonRect;
							RECT adjustedCameraLockRect2 = app->m_cameraLockSetButtonRect;
							adjustedRollCriticalToggleRect2.top -= (int)app->m_settingsScrollOffset;
							adjustedRollCriticalToggleRect2.bottom -= (int)app->m_settingsScrollOffset;
							adjustedRollM1Rect2.top -= (int)app->m_settingsScrollOffset;
							adjustedRollM1Rect2.bottom -= (int)app->m_settingsScrollOffset;
							adjustedRollCritRect2.top -= (int)app->m_settingsScrollOffset;
							adjustedRollCritRect2.bottom -= (int)app->m_settingsScrollOffset;
							adjustedDodgeRect2.top -= (int)app->m_settingsScrollOffset;
							adjustedDodgeRect2.bottom -= (int)app->m_settingsScrollOffset;
							adjustedParryRect2.top -= (int)app->m_settingsScrollOffset;
							adjustedParryRect2.bottom -= (int)app->m_settingsScrollOffset;
							adjustedOpenMapRect2.top -= (int)app->m_settingsScrollOffset;
							adjustedOpenMapRect2.bottom -= (int)app->m_settingsScrollOffset;
							adjustedCriticalAttackRect2.top -= (int)app->m_settingsScrollOffset;
							adjustedCriticalAttackRect2.bottom -= (int)app->m_settingsScrollOffset;
							adjustedCameraLockRect2.top -= (int)app->m_settingsScrollOffset;
							adjustedCameraLockRect2.bottom -= (int)app->m_settingsScrollOffset;
							if (PtInRect(&adjustedRollCriticalToggleRect2, pt)) showHand = true;
							if (PtInRect(&adjustedRollM1Rect2, pt)) showHand = true;
							if (PtInRect(&adjustedRollCritRect2, pt)) showHand = true;
							if (PtInRect(&adjustedDodgeRect2, pt)) showHand = true;
							if (PtInRect(&adjustedParryRect2, pt)) showHand = true;
							if (PtInRect(&adjustedOpenMapRect2, pt)) showHand = true;
							if (PtInRect(&adjustedCriticalAttackRect2, pt)) showHand = true;
							if (PtInRect(&adjustedCameraLockRect2, pt)) showHand = true;
							RECT adjustedQuickTurnToggleRect2 = app->m_quickTurnToggleRect;
							adjustedQuickTurnToggleRect2.top -= (int)app->m_settingsScrollOffset;
							adjustedQuickTurnToggleRect2.bottom -= (int)app->m_settingsScrollOffset;
							if (PtInRect(&adjustedQuickTurnToggleRect2, pt)) showHand = true;
							for (int i = 0; i < 10; i++) {
								RECT adjustedRect = app->m_hotbarSlotSetButtonRect[i];
								adjustedRect.top -= (int)app->m_settingsScrollOffset;
								adjustedRect.bottom -= (int)app->m_settingsScrollOffset;
								if (PtInRect(&adjustedRect, pt)) {
									showHand = true;
									break;
								}
								
								RECT adjustedToggleRect = app->m_hotbarSlotToggleRect[i];
								adjustedToggleRect.top -= (int)app->m_settingsScrollOffset;
								adjustedToggleRect.bottom -= (int)app->m_settingsScrollOffset;
								if (PtInRect(&adjustedToggleRect, pt)) {
									showHand = true;
									break;
								}
								
								RECT adjustedMapCastToggleRect = app->m_hotbarSlotMapCastToggleRect[i];
								adjustedMapCastToggleRect.top -= (int)app->m_settingsScrollOffset;
								adjustedMapCastToggleRect.bottom -= (int)app->m_settingsScrollOffset;
								if (PtInRect(&adjustedMapCastToggleRect, pt)) {
									showHand = true;
									break;
								}
							}
							
							// Auto Wisp cursor handling
							for (size_t wispIdx = 0; wispIdx < app->m_wispSequences.size(); wispIdx++) {
								const auto& seq = app->m_wispSequences[wispIdx];
								
								// Check character boxes - use I-beam cursor (text cursor)
								for (const auto& charBox : seq.charBoxes) {
									RECT adjustedCharBoxRect = charBox.rect;
									adjustedCharBoxRect.top -= (int)app->m_settingsScrollOffset;
									adjustedCharBoxRect.bottom -= (int)app->m_settingsScrollOffset;
									if (PtInRect(&adjustedCharBoxRect, pt)) {
										showIBeam = true;
										break;
									}
								}
								if (showIBeam) break;
								
								// Check buttons - use hand cursor
								RECT adjustedSetKeyRect = seq.setKeyButtonRect;
								adjustedSetKeyRect.top -= (int)app->m_settingsScrollOffset;
								adjustedSetKeyRect.bottom -= (int)app->m_settingsScrollOffset;
								if (PtInRect(&adjustedSetKeyRect, pt)) {
									showHand = true;
									break;
								}
								
								RECT adjustedPlusRect = seq.plusButtonRect;
								adjustedPlusRect.top -= (int)app->m_settingsScrollOffset;
								adjustedPlusRect.bottom -= (int)app->m_settingsScrollOffset;
								if (PtInRect(&adjustedPlusRect, pt)) {
									showHand = true;
									break;
								}
								
								RECT adjustedDeleteRect = seq.deleteButtonRect;
								adjustedDeleteRect.top -= (int)app->m_settingsScrollOffset;
								adjustedDeleteRect.bottom -= (int)app->m_settingsScrollOffset;
								if (PtInRect(&adjustedDeleteRect, pt)) {
									showHand = true;
									break;
								}
								
								RECT adjustedRefreshRect = seq.refreshButtonRect;
								adjustedRefreshRect.top -= (int)app->m_settingsScrollOffset;
								adjustedRefreshRect.bottom -= (int)app->m_settingsScrollOffset;
								if (PtInRect(&adjustedRefreshRect, pt)) {
									showHand = true;
									break;
								}
							}
                    }
                        if (showIBeam) {
                            SetCursor(LoadCursor(nullptr, IDC_IBEAM));
                        } else {
                            SetCursor(LoadCursor(nullptr, showHand ? IDC_HAND : IDC_ARROW));
                        }
                        return TRUE;
                    }
                    break;
                }
                
                case WM_MOUSEWHEEL: {
                    if (app && app->m_showUI) {
                        int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                        float scrollSpeed = 30.0f;
                        
                        if (app->m_showSettings) {
                            app->m_settingsScrollOffset -= (wheelDelta / WHEEL_DELTA) * scrollSpeed;
                            
                            if (app->m_settingsScrollOffset < 0.0f) {
                                app->m_settingsScrollOffset = 0.0f;
                            }
                            if (app->m_settingsScrollOffset > app->m_settingsMaxScrollOffset) {
                                app->m_settingsScrollOffset = app->m_settingsMaxScrollOffset;
                            }
                        } else if (app->m_showProfile) {
                            // Profile tab scrolling (if needed in future)
                        } else {
                            app->m_modsScrollOffset -= (wheelDelta / WHEEL_DELTA) * scrollSpeed;
                            
                            if (app->m_modsScrollOffset < 0.0f) {
                                app->m_modsScrollOffset = 0.0f;
                            }
                            if (app->m_modsScrollOffset > app->m_modsMaxScrollOffset) {
                                app->m_modsScrollOffset = app->m_modsMaxScrollOffset;
                            }
                        }
                        
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                }
                
                case WM_LBUTTONDOWN: {
                    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
                    pt = app->MapPointToLogical(pt);
                    
                    // Handle search bar focus
                    if (PtInRect(&app->m_searchRect, pt)) {
                        if (!app->m_searchFocused) {
                            app->m_searchAnimatingBorder = true;
                        }
                        app->m_searchFocused = true;
                        app->m_cursorVisible = true;
                        app->m_cursorBlinkTimer = 0;
                        // Ensure timer is running for cursor blinking
                        SetTimer(hwnd, 1, 16, nullptr); // 60fps timer
                        SetTimer(hwnd, 4, 16, nullptr); // Border animation timer
                        InvalidateRect(hwnd, nullptr, FALSE);
                    } else {
                        if (app->m_searchFocused) {
                            app->m_searchAnimatingBorder = true;
                        }
                        app->m_searchFocused = false;
                        SetTimer(hwnd, 4, 16, nullptr); // Border animation timer
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    
                    // Handle close button click
                    if (PtInRect(&app->m_closeButtonRect, pt)) {
                        app->m_showUI = false;
                        app->HideWindowWithAnimation();
                        return 0;
                    }
                    
                    // Handle panels button click
                    if (PtInRect(&app->m_panelsButtonRect, pt)) {
                        app->m_showUI = false;
                        g_editHudPendingStart = true;
                        app->HideWindowWithAnimation();
                        return 0;
                    }
                    
                    // Handle exit button click
                    if (PtInRect(&app->m_exitButtonRect, pt)) {
                        app->m_exitAfterHide = true;
                        app->m_showUI = false;
                        app->HideWindowWithAnimation();
                        return 0;
                    }
                    
                    if (PtInRect(&app->m_profileRect, pt)) {
                        app->m_showProfile = true;
                        app->m_showSettings = false;
                        // Clear search when switching tabs
                        if (app->m_searchFocused) {
                            app->m_searchAnimatingBorder = true;
                        }
                        app->m_searchFocused = false;
                        app->m_searchText.clear();
                        app->m_cursorPosition = 0;
                        app->m_cursorVisible = true;
                        app->m_cursorBlinkTimer = 0;
                        SetTimer(hwnd, 4, 16, nullptr); // Border animation timer
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    if (PtInRect(&app->m_settingsRect, pt)) {
                        app->m_showSettings = true;
                        app->m_showProfile = false;
                        // Clear search when switching tabs
                        if (app->m_searchFocused) {
                            app->m_searchAnimatingBorder = true;
                        }
                        app->m_searchFocused = false;
                        app->m_searchText.clear();
                        app->m_cursorPosition = 0;
                        app->m_cursorVisible = true;
                        app->m_cursorBlinkTimer = 0;
                        SetTimer(hwnd, 4, 16, nullptr); // Border animation timer
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    if (PtInRect(&app->m_modsRect, pt)) {
                        app->m_showSettings = false;
                        app->m_showProfile = false;
                        // Clear search when switching tabs
                        if (app->m_searchFocused) {
                            app->m_searchAnimatingBorder = true;
                        }
                        app->m_searchFocused = false;
                        app->m_searchText.clear();
                        app->m_cursorPosition = 0;
                        app->m_cursorVisible = true;
                        app->m_cursorBlinkTimer = 0;
                        SetTimer(hwnd, 4, 16, nullptr); // Border animation timer
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    if (!app->m_showSettings && !app->m_showProfile) {
                        for (int i = 0; i < app->m_modCards.size(); i++) {
                            RECT adjustedModCardRect = app->m_modCards[i].rect;
                            adjustedModCardRect.top -= (int)app->m_modsScrollOffset;
                            adjustedModCardRect.bottom -= (int)app->m_modsScrollOffset;
                            if (PtInRect(&adjustedModCardRect, pt)) {
                            if (i == 10) {
                                return 0;
                            }
                            // Start animation and toggle enabled state for clicked mod
                            app->m_modCards[i].animatingBorder = true;
                            app->m_modCards[i].enabled = !app->m_modCards[i].enabled;
                            app->MarkSettingsDirty();
                            
                            // Start border animation timer
                            SetTimer(hwnd, 4, 16, nullptr);
                            
                             // Handle specific mod functionality
                             if (i == 0) { // Roll M1 mod (index 0)
                                 if (app->m_modCards[i].enabled) {
                                     // Defer actual start until UI hides
                                     g_rollM1PendingStart = true;
                                 } else {
                                     // Disable Roll M1 - sync for immediate cleanup
                                     if (g_rollM1Running) {
                                         RollM1::SetEnabled(false);
                                         RollM1::StopRollM1();
                                         g_rollM1Running = false;
                                     }
                                     g_rollM1PendingStart = false;
                                 }
                            } else if (i == 1) { // Parry Bar mod (index 1)
                                if (app->m_modCards[i].enabled) {
                                    // Parry bar mod enabled - need UI
                                    if (g_parryBarRunning) {
                                        // Already running, but might be without UI - restart with UI
                                        ParryBar::StopPixelReading();
                                        ParryBar::CleanupPixelReader();
                                        g_parryBarRunning = false;
                                    }
                                    // Defer actual start until UI hides
                                    g_parryBarPendingStart = true;
                                } else {
                                    // Check if lightspeed reflex still needs pixel reading
                                    bool lightspeedNeedsPixelReading = false;
                                    if (app->m_modCards.size() > 13 && app->m_modCards[13].enabled) {
                                        lightspeedNeedsPixelReading = app->m_betterParryAntiShakyBlock;
                                    }
                                    
                                    if (!lightspeedNeedsPixelReading) {
                                        // Both mods disabled - stop pixel reading
                                        if (g_parryBarRunning) {
                                            ParryBar::StopPixelReading();
                                            ParryBar::CleanupPixelReader();
                                            g_parryBarRunning = false;
                                        }
                                        g_parryBarPendingStart = false;
                                    } else {
                                        // Lightspeed still needs it - restart without UI
                                        if (g_parryBarRunning) {
                                            ParryBar::StopPixelReading();
                                            ParryBar::CleanupPixelReader();
                                            g_parryBarRunning = false;
                                        }
                                        g_parryBarPendingStart = true;
                                    }
                                }
                            } else if (i == 13) { // Better Parry mod (index 13)
                                bool betterParryEnabled = app->m_modCards[i].enabled;
                                bool lightspeedNeedsPixelReading = betterParryEnabled && app->m_betterParryAntiShakyBlock;
                                bool parryBarModEnabled = (app->m_modCards.size() > 1 && app->m_modCards[1].enabled);
                                
                                if (lightspeedNeedsPixelReading || parryBarModEnabled) {
                                    // At least one mod needs pixel reading
                                    if (!g_parryBarRunning && !g_parryBarPendingStart) {
                                        g_parryBarPendingStart = true;
                                    } else if (g_parryBarRunning && parryBarModEnabled) {
                                        // Parry bar mod is enabled, so we need UI - restart if needed
                                        // Check if we need to restart (if it was started without UI)
                                        // We'll restart to ensure UI is enabled
                                        ParryBar::StopPixelReading();
                                        ParryBar::CleanupPixelReader();
                                        g_parryBarRunning = false;
                                        g_parryBarPendingStart = true;
                                    }
                                } else {
                                    // Neither mod needs pixel reading
                                    if (g_parryBarRunning) {
                                        ParryBar::StopPixelReading();
                                        ParryBar::CleanupPixelReader();
                                        g_parryBarRunning = false;
                                    }
                                    g_parryBarPendingStart = false;
                                }
                            } else if (i == 2) { // Map Cast mod (index 2)
                                if (app->m_modCards[i].enabled) {
                                    // Defer actual start until UI hides
                                    g_mapCastPendingStart = true;
                                } else {
                                    // Disable Map Cast - sync for immediate cleanup
                                    if (g_mapCastRunning) {
                                        MapCast::SetEnabled(false);
                                        MapCast::StopMapCast();
                                        g_mapCastRunning = false;
                                    }
                                    g_mapCastPendingStart = false;
                                }
                            } else if (i == 3) { // Keystrokes mod (index 3)
                                if (app->m_modCards[i].enabled) {
                                    // Defer actual start until UI hides
                                    g_keystrokesPendingStart = true;
                                } else {
                                    // Disable keystrokes - sync for immediate cleanup
                                    if (g_keystrokesRunning) {
                                        Keystrokes::StopKeystrokes();
                                        g_keystrokesRunning = false;
                                    }
                                    g_keystrokesPendingStart = false;
                                }
                            } else if (i == 5) { // CPS mod (index 5)
                                if (app->m_modCards[i].enabled) {
                                    // Defer actual start until UI hides
                                    g_cpsPendingStart = true;
                                } else {
                                    // Disable CPS - sync for immediate cleanup
                                    if (g_cpsRunning) {
                                        CPS::StopCPS();
                                        g_cpsRunning = false;
                                    }
                                    g_cpsPendingStart = false;
                                }
                            } else if (i == 6) { // Roll Cast mod (index 6)
                                if (app->m_modCards[i].enabled) {
                                    // Defer actual start until UI hides
                                    g_rollCastPendingStart = true;
                                } else {
                                    // Disable Roll Cast - sync for immediate cleanup
                                    if (g_rollCastRunning) {
                                        RollCast::SetEnabled(false);
                                        RollCast::StopRollCast();
                                        g_rollCastRunning = false;
                                    }
                                    g_rollCastPendingStart = false;
                                }
                            } else if (i == 7) { // Hold M1 mod (index 7)
                                if (app->m_modCards[i].enabled) {
                                    // Defer actual start until UI hides
                                    g_holdM1PendingStart = true;
                                } else {
                                    // Disable Hold M1 - sync for immediate cleanup
                                    if (g_holdM1Running) {
                                        HoldM1::SetHoldM1Enabled(false);
                                        HoldM1::StopPixelScanning();
                                        g_holdM1Running = false;
                                    }
                                    g_holdM1PendingStart = false;
                                }
                            } else if (i == 8) { // Crosshair mod (index 8)
                                if (app->m_modCards[i].enabled) {
                                    // Defer actual start until UI hides
                                    g_crosshairPendingStart = true;
                                } else {
                                    // Set pending disable - will actually disable when window hides
                                    if (g_crosshairRunning) {
                                        g_crosshairPendingDisable = true;
                                    }
                                    g_crosshairPendingStart = false;
                                }
                            } else if (i == 9) { // Roll Spit mod (index 9)
                                if (app->m_modCards[i].enabled) {
                                    g_rollSpitPendingStart = true;
                                } else {
                                    if (g_rollSpitRunning) {
                                        RollSpit::SetEnabled(false);
                                        RollSpit::StopRollSpit();
                                        g_rollSpitRunning = false;
                                    }
                                    g_rollSpitPendingStart = false;
                                }
                            } else if (i == 10) { // Motion Blur mod (index 10)
                                if (app->m_modCards[i].enabled) {
                                    g_motionBlurPendingStart = true;
                                } else {
                                    if (g_motionBlurRunning) {
                                        MotionBlur::StopMotionBlur();
                                        g_motionBlurRunning = false;
                                    }
                                    g_motionBlurPendingStart = false;
                                }
                            } else if (i == 11) { // Zoom mod (index 11)
                                if (app->m_modCards[i].enabled) {
                                    g_zoomPendingStart = true;
                                } else {
                                    if (g_zoomRunning) {
                                        Zoom::Shutdown();
                                        g_zoomRunning = false;
                                    }
                                    g_zoomPendingStart = false;
                                }
                            } else if (i == 12) { // Quick Turn mod (index 12)
                                if (app->m_modCards[i].enabled) {
                                    g_quickTurnPendingStart = true;
                                } else {
                                    if (g_quickTurnRunning) {
                                        QuickTurn::SetEnabled(false);
                                        QuickTurn::StopQuickTurn();
                                        g_quickTurnRunning = false;
                                    }
                                    g_quickTurnPendingStart = false;
                                }
                            } else if (i == 14) { // Golden Tongue mod (index 14)
                                if (app->m_modCards[i].enabled) {
                                    g_goldenTonguePendingStart = true;
                                } else {
                                    if (g_goldenTongueRunning) {
                                        GoldenTongue::SetEnabled(false);
                                        GoldenTongue::StopGoldenTongue();
                                        g_goldenTongueRunning = false;
                                    }
                                    g_goldenTonguePendingStart = false;
                                }
                            } else if (i == 15) { // Gamma mod (index 15)
                                if (app->m_modCards[i].enabled) {
                                    g_gammaRunning = true;
                                    g_gammaEnabled.store(true);
                                    float gammaValue = 0.1f + app->m_gammaSlider * 3.9f;
                                    g_gammaTarget.store(gammaValue);
                                    StartGammaMonitor();
                                } else {
                                    if (g_gammaRunning) {
                                        StopGammaMonitor();
                                    }
                                }
                            }
                             
                                InvalidateRect(hwnd, nullptr, FALSE);
                                return 0;
                            }
                        }
                    } else {
                        RECT adjustedSliderTrack3 = app->m_sliderTrackRect;
                        RECT adjustedSliderKnob3 = app->m_sliderKnobRect;
                        adjustedSliderTrack3.top -= (int)app->m_settingsScrollOffset;
                        adjustedSliderTrack3.bottom -= (int)app->m_settingsScrollOffset;
                        adjustedSliderKnob3.top -= (int)app->m_settingsScrollOffset;
                        adjustedSliderKnob3.bottom -= (int)app->m_settingsScrollOffset;
                        if (PtInRect(&adjustedSliderTrack3, pt) || PtInRect(&adjustedSliderKnob3, pt)) {
                            app->m_draggingSlider = true;
                            int left = app->m_sliderTrackRect.left;
                            int right = app->m_sliderTrackRect.right;
                            float t = (pt.x - left) / float(right - left);
                            if (t < 0) t = 0; if (t > 1) t = 1;
                            app->m_blurSlider = t;
                            SetBlurIntensity(t);
                            InvalidateRect(hwnd, nullptr, FALSE);
                            return 0;
                        }
					RECT adjustedRollCriticalToggleClickRect = app->m_rollCriticalToggleRect;
					RECT adjustedRollM1ClickRect = app->m_rollM1SetButtonRect;
					RECT adjustedRollCritClickRect = app->m_rollCritSetButtonRect;
					RECT adjustedRollSpitTriggerClickRect = app->m_rollSpitTriggerSetButtonRect;
					RECT adjustedRollSpitSpitClickRect = app->m_rollSpitSpitSetButtonRect;
					RECT adjustedRollCastClickRect = app->m_rollCastSetButtonRect;
					RECT adjustedZoomClickRect = app->m_zoomSetButtonRect;
					RECT adjustedDodgeClickRect = app->m_dodgeSetButtonRect;
					RECT adjustedParryClickRect = app->m_parrySetButtonRect;
					RECT adjustedOpenMapClickRect = app->m_openMapSetButtonRect;
					RECT adjustedCriticalAttackClickRect = app->m_criticalAttackSetButtonRect;
					RECT adjustedGoldenTongueClickRect = app->m_goldenTongueSetButtonRect;
					RECT adjustedCameraLockClickRect = app->m_cameraLockSetButtonRect;
					RECT adjustedHoldM1ToggleRect = app->m_holdM1ToggleRect;
					RECT adjustedCrosshairFileRect = app->m_crosshairFileButtonRect;
					RECT adjustedCrosshairScaleTrackRect = app->m_crosshairScaleTrackRect;
					RECT adjustedCrosshairScaleKnobRect = app->m_crosshairScaleKnobRect;
					RECT adjustedMotionBlurClickTrackRect = app->m_motionBlurTrackRect;
					RECT adjustedMotionBlurClickKnobRect = app->m_motionBlurKnobRect;
					RECT adjustedGammaSliderTrackRect = app->m_gammaSliderTrackRect;
					RECT adjustedGammaSliderKnobRect = app->m_gammaSliderKnobRect;
					adjustedRollCriticalToggleClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollCriticalToggleClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollM1ClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollM1ClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollCritClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollCritClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollSpitTriggerClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollSpitTriggerClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollSpitSpitClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollSpitSpitClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedRollCastClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollCastClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedZoomClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedZoomClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedDodgeClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedDodgeClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedParryClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedParryClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedOpenMapClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedOpenMapClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedCriticalAttackClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedCriticalAttackClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedGoldenTongueClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedGoldenTongueClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedCameraLockClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedCameraLockClickRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedHoldM1ToggleRect.top -= (int)app->m_settingsScrollOffset;
					adjustedHoldM1ToggleRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairFileRect.top -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairFileRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairScaleTrackRect.top -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairScaleTrackRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairScaleKnobRect.top -= (int)app->m_settingsScrollOffset;
					adjustedCrosshairScaleKnobRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedMotionBlurClickTrackRect.top -= (int)app->m_settingsScrollOffset;
					adjustedMotionBlurClickTrackRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedMotionBlurClickKnobRect.top -= (int)app->m_settingsScrollOffset;
					adjustedMotionBlurClickKnobRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedGammaSliderTrackRect.top -= (int)app->m_settingsScrollOffset;
					adjustedGammaSliderTrackRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedGammaSliderKnobRect.top -= (int)app->m_settingsScrollOffset;
					adjustedGammaSliderKnobRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedRollCriticalToggleClickRect, pt)) {
						app->m_rollCritical = !app->m_rollCritical;

						// Start/stop Roll Critical based on toggle
						if (app->m_rollCritical) {
							g_rollCriticalPendingStart = true;
						} else {
							if (g_rollCriticalRunning) {
								RollCritical::SetEnabled(false);
								RollCritical::StopRollCritical();
								g_rollCriticalRunning = false;
							}
							g_rollCriticalPendingStart = false;
						}

						app->MarkSettingsDirty();
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedRollM1ClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_rollM1Capturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedRollCritClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_rollCritCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedRollSpitTriggerClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_rollSpitTriggerCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedRollSpitSpitClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_rollSpitSpitCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedRollCastClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_rollCastCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedZoomClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_zoomCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedHoldM1ToggleRect, pt)) {
						app->m_holdM1Enabled = !app->m_holdM1Enabled;
						HoldM1::SetIntelligentHoldM1Enabled(app->m_holdM1Enabled);
						app->MarkSettingsDirty();
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					RECT adjustedBetterParryAntiShakyClickRect = app->m_betterParryAntiShakyBlockToggleRect;
					adjustedBetterParryAntiShakyClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedBetterParryAntiShakyClickRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedBetterParryAntiShakyClickRect, pt)) {
						app->m_betterParryAntiShakyBlock = !app->m_betterParryAntiShakyBlock;
						
						// Update pixel reading based on new state
						bool betterParryEnabled = (app->m_modCards.size() > 13 && app->m_modCards[13].enabled);
						bool lightspeedNeedsPixelReading = betterParryEnabled && app->m_betterParryAntiShakyBlock;
						bool parryBarModEnabled = (app->m_modCards.size() > 1 && app->m_modCards[1].enabled);
						
						if (lightspeedNeedsPixelReading || parryBarModEnabled) {
							// At least one mod needs pixel reading
							if (!g_parryBarRunning && !g_parryBarPendingStart) {
								g_parryBarPendingStart = true;
							} else if (g_parryBarRunning && parryBarModEnabled) {
								// Parry bar mod is enabled, so we need UI - restart if needed
								ParryBar::StopPixelReading();
								ParryBar::CleanupPixelReader();
								g_parryBarRunning = false;
								g_parryBarPendingStart = true;
							} else if (g_parryBarRunning && !parryBarModEnabled && lightspeedNeedsPixelReading) {
								// Parry bar disabled but lightspeed needs it - restart without UI
								ParryBar::StopPixelReading();
								ParryBar::CleanupPixelReader();
								g_parryBarRunning = false;
								g_parryBarPendingStart = true;
							}
						} else {
							// Neither mod needs pixel reading
							if (g_parryBarRunning) {
								ParryBar::StopPixelReading();
								ParryBar::CleanupPixelReader();
								g_parryBarRunning = false;
							}
							g_parryBarPendingStart = false;
						}
						
						app->MarkSettingsDirty();
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					RECT adjustedBetterParryLightspeedClickRect = app->m_betterParryLightspeedReflexToggleRect;
					adjustedBetterParryLightspeedClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedBetterParryLightspeedClickRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedBetterParryLightspeedClickRect, pt)) {
						app->m_betterParryLightspeedReflex = !app->m_betterParryLightspeedReflex;
						app->MarkSettingsDirty();
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					RECT adjustedBetterParryShouldParryClickRect = app->m_betterParryShouldParryToggleRect;
					adjustedBetterParryShouldParryClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedBetterParryShouldParryClickRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedBetterParryShouldParryClickRect, pt)) {
						app->m_betterParryShouldParry = !app->m_betterParryShouldParry;
						app->MarkSettingsDirty();
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					RECT adjustedRollParryToggleClickRect = app->m_rollParryToggleRect;
					adjustedRollParryToggleClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollParryToggleClickRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedRollParryToggleClickRect, pt)) {
						app->m_rollParry = !app->m_rollParry;

						// Start/stop Roll Parry based on toggle
						if (app->m_rollParry) {
							g_rollParryPendingStart = true;
						} else {
							if (g_rollParryRunning) {
								RollParry::SetEnabled(false);
								RollParry::StopRollParry();
								g_rollParryRunning = false;
							}
							g_rollParryPendingStart = false;
						}

						app->MarkSettingsDirty();
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					RECT adjustedRollParryButtonClickRect = app->m_rollParrySetButtonRect;
					adjustedRollParryButtonClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedRollParryButtonClickRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedRollParryButtonClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_rollParryCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					RECT adjustedQuickTurnClickRect = app->m_quickTurnSetButtonRect;
					adjustedQuickTurnClickRect.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnClickRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedQuickTurnClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_quickTurnCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					RECT adjustedQuickTurnToggleRect = app->m_quickTurnToggleRect;
					adjustedQuickTurnToggleRect.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnToggleRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedQuickTurnToggleRect, pt)) {
						app->m_quickTurnToggleMode = !app->m_quickTurnToggleMode;
						app->MarkSettingsDirty();
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					RECT adjustedQuickTurnSensitivityTrackRect = app->m_quickTurnSensitivityTrackRect;
					RECT adjustedQuickTurnSensitivityKnobRect = app->m_quickTurnSensitivityKnobRect;
					adjustedQuickTurnSensitivityTrackRect.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnSensitivityTrackRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnSensitivityKnobRect.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnSensitivityKnobRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedQuickTurnSensitivityTrackRect, pt) || PtInRect(&adjustedQuickTurnSensitivityKnobRect, pt)) {
						app->m_draggingQuickTurnSensitivity = true;
						app->m_quickTurnSensitivityAutoUpdate = false;
						int left = app->m_quickTurnSensitivityTrackRect.left;
						int right = app->m_quickTurnSensitivityTrackRect.right;
						float t = (pt.x - left) / float(right - left);
						if (t < 0) t = 0; if (t > 1) t = 1;
						app->m_quickTurnSensitivitySlider = t;
						if (g_quickTurnRunning) {
							QuickTurn::SetSensitivity(t);
						}
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					RECT adjustedQuickTurnDegreesTrackRect = app->m_quickTurnDegreesTrackRect;
					RECT adjustedQuickTurnDegreesKnobRect = app->m_quickTurnDegreesKnobRect;
					adjustedQuickTurnDegreesTrackRect.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnDegreesTrackRect.bottom -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnDegreesKnobRect.top -= (int)app->m_settingsScrollOffset;
					adjustedQuickTurnDegreesKnobRect.bottom -= (int)app->m_settingsScrollOffset;
					if (PtInRect(&adjustedQuickTurnDegreesTrackRect, pt) || PtInRect(&adjustedQuickTurnDegreesKnobRect, pt)) {
						app->m_draggingQuickTurnDegrees = true;
						int left = app->m_quickTurnDegreesTrackRect.left;
						int right = app->m_quickTurnDegreesTrackRect.right;
						float t = (pt.x - left) / float(right - left);
						if (t < 0) t = 0; if (t > 1) t = 1;
						app->m_quickTurnDegreesSlider = t;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedDodgeClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_dodgeCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedParryClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_parryCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedOpenMapClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_openMapCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedCriticalAttackClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_criticalAttackCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedGoldenTongueClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_goldenTongueCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					
					// Handle Auto Wisp section clicks
					for (size_t i = 0; i < app->m_wispSequences.size(); i++) {
						auto& seq = app->m_wispSequences[i];
						
						// Check character boxes
						for (size_t j = 0; j < seq.charBoxes.size(); j++) {
							RECT adjustedCharBoxRect = seq.charBoxes[j].rect;
							adjustedCharBoxRect.top -= (int)app->m_settingsScrollOffset;
							adjustedCharBoxRect.bottom -= (int)app->m_settingsScrollOffset;
							
							if (PtInRect(&adjustedCharBoxRect, pt)) {
								// Start editing this sequence
								seq.isEditing = true;
								seq.focusedCharIndex = (int)j;
								seq.charBoxes[j].focused = true;
								app->m_focusedWispSequenceIndex = (int)i;
								// Ensure there's at least one empty char box at the end
								if (j == seq.charBoxes.size() - 1 && seq.charBoxes[j].ch != 0) {
									seq.charBoxes.push_back(WispCharBox());
								}
								InvalidateRect(hwnd, nullptr, FALSE);
								return 0;
							}
						}
						
						RECT adjustedSetKeyRect = seq.setKeyButtonRect;
						adjustedSetKeyRect.top -= (int)app->m_settingsScrollOffset;
						adjustedSetKeyRect.bottom -= (int)app->m_settingsScrollOffset;
						RECT adjustedPlusRect = seq.plusButtonRect;
						adjustedPlusRect.top -= (int)app->m_settingsScrollOffset;
						adjustedPlusRect.bottom -= (int)app->m_settingsScrollOffset;
						RECT adjustedDeleteRect = seq.deleteButtonRect;
						adjustedDeleteRect.top -= (int)app->m_settingsScrollOffset;
						adjustedDeleteRect.bottom -= (int)app->m_settingsScrollOffset;
						RECT adjustedRefreshRect = seq.refreshButtonRect;
						adjustedRefreshRect.top -= (int)app->m_settingsScrollOffset;
						adjustedRefreshRect.bottom -= (int)app->m_settingsScrollOffset;
						
						// Set Key button click
						if (PtInRect(&adjustedSetKeyRect, pt)) {
							app->m_keyCaptureActive = true;
							seq.capturingKey = true;
							InvalidateRect(hwnd, nullptr, FALSE);
							return 0;
						}
						
						// Plus button click (only on last row)
						if (i == app->m_wispSequences.size() - 1 && PtInRect(&adjustedPlusRect, pt)) {
							WispSequence newSeq;
							newSeq.key = L"F1";
							newSeq.charBoxes.push_back(WispCharBox());
							app->m_wispSequences.push_back(newSeq);
							app->MarkSettingsDirty();
							InvalidateRect(hwnd, nullptr, FALSE);
							return 0;
						}
						
						// Delete button click (can't delete first sequence)
						if (i > 0 && PtInRect(&adjustedDeleteRect, pt)) {
							app->m_wispSequences.erase(app->m_wispSequences.begin() + i);
							if (app->m_focusedWispSequenceIndex == (int)i) {
								app->m_focusedWispSequenceIndex = -1;
							} else if (app->m_focusedWispSequenceIndex > (int)i) {
								app->m_focusedWispSequenceIndex--;
							}
							app->MarkSettingsDirty();
							InvalidateRect(hwnd, nullptr, FALSE);
							return 0;
						}
						
						// Refresh button click
						if (PtInRect(&adjustedRefreshRect, pt)) {
							seq.charBoxes.clear();
							seq.charBoxes.push_back(WispCharBox());
							seq.isEditing = true;
							seq.focusedCharIndex = 0;
							seq.charBoxes[0].focused = true;
							app->m_focusedWispSequenceIndex = (int)i;
							app->m_wispCursorVisible = true;
							app->m_wispCursorBlinkTimer = 0;
							app->MarkSettingsDirty();
							SetTimer(hwnd, 1, 16, nullptr);
							InvalidateRect(hwnd, nullptr, FALSE);
							return 0;
						}
					}
					
					// Click outside - stop editing
					if (app->m_focusedWispSequenceIndex >= 0 && app->m_focusedWispSequenceIndex < (int)app->m_wispSequences.size()) {
						auto& seq = app->m_wispSequences[app->m_focusedWispSequenceIndex];
						seq.isEditing = false;
						seq.focusedCharIndex = -1;
						for (auto& charBox : seq.charBoxes) {
							charBox.focused = false;
						}
						// Remove empty boxes at the end
						while (seq.charBoxes.size() > 1 && seq.charBoxes.back().ch == 0) {
							seq.charBoxes.pop_back();
						}
						app->m_focusedWispSequenceIndex = -1;
						app->MarkSettingsDirty();
						InvalidateRect(hwnd, nullptr, FALSE);
					}
					if (PtInRect(&adjustedCameraLockClickRect, pt)) {
						app->m_keyCaptureActive = true;
						app->m_cameraLockCapturing = true;
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedCrosshairFileRect, pt)) {
						OPENFILENAMEW ofn = {};
						wchar_t filePath[MAX_PATH] = {};
						
						ofn.lStructSize = sizeof(OPENFILENAMEW);
						ofn.hwndOwner = hwnd;
						ofn.lpstrFile = filePath;
						ofn.nMaxFile = MAX_PATH;
						ofn.lpstrFilter = L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.ico\0All Files\0*.*\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrTitle = L"Select Crosshair Image";
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
						
						if (GetOpenFileNameW(&ofn)) {
							app->m_crosshairFilePath = std::wstring(filePath);
							Crosshair::UpdateCrosshairImage(app->m_crosshairFilePath);
							app->MarkSettingsDirty();
							InvalidateRect(hwnd, nullptr, FALSE);
						}
						return 0;
					}
					if (PtInRect(&adjustedCrosshairScaleTrackRect, pt) || PtInRect(&adjustedCrosshairScaleKnobRect, pt)) {
						app->m_draggingCrosshairScale = true;
						int left = app->m_crosshairScaleTrackRect.left;
						int right = app->m_crosshairScaleTrackRect.right;
						float t = (pt.x - left) / float(right - left);
						if (t < 0) t = 0; if (t > 1) t = 1;
						app->m_crosshairScale = t;
						Crosshair::UpdateCrosshairScale(app->m_crosshairScale);
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedMotionBlurClickTrackRect, pt) || PtInRect(&adjustedMotionBlurClickKnobRect, pt)) {
						app->m_draggingMotionBlur = true;
						int left = app->m_motionBlurTrackRect.left;
						int right = app->m_motionBlurTrackRect.right;
						float t = (pt.x - left) / float(right - left);
						if (t < 0) t = 0; if (t > 1) t = 1;
						app->m_motionBlurSlider = t;
						MotionBlur::SetMotionBlurIntensity(powf(t, 0.35f) * 1.5f);
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					if (PtInRect(&adjustedGammaSliderTrackRect, pt) || PtInRect(&adjustedGammaSliderKnobRect, pt)) {
						app->m_draggingGammaSlider = true;
						int left = app->m_gammaSliderTrackRect.left;
						int right = app->m_gammaSliderTrackRect.right;
						float t = (pt.x - left) / float(right - left);
						if (t < 0) t = 0; if (t > 1) t = 1;
						app->m_gammaSlider = t;
						float gammaValue = 0.1f + t * 3.9f;
						g_gammaTarget.store(gammaValue);
						if (g_gammaRunning && g_gammaEnabled.load()) {
							HWND robloxWindow = WinRTCapture::FindRobloxWindow();
							HWND fg = GetForegroundWindow();
							if (robloxWindow && (fg == robloxWindow || GetParent(fg) == robloxWindow)) {
								SetGamma(gammaValue);
							}
						}
						InvalidateRect(hwnd, nullptr, FALSE);
						return 0;
					}
					for (int i = 0; i < 10; i++) {
						RECT adjustedRect = app->m_hotbarSlotSetButtonRect[i];
						adjustedRect.top -= (int)app->m_settingsScrollOffset;
						adjustedRect.bottom -= (int)app->m_settingsScrollOffset;
						if (PtInRect(&adjustedRect, pt)) {
							app->m_keyCaptureActive = true;
							app->m_hotbarSlotCapturing[i] = true;
							InvalidateRect(hwnd, nullptr, FALSE);
							return 0;
						}
						
						// Handle toggle slider click
						RECT adjustedToggleRect = app->m_hotbarSlotToggleRect[i];
						adjustedToggleRect.top -= (int)app->m_settingsScrollOffset;
						adjustedToggleRect.bottom -= (int)app->m_settingsScrollOffset;
						if (PtInRect(&adjustedToggleRect, pt)) {
							app->m_hotbarSlotEnabled[i] = !app->m_hotbarSlotEnabled[i];
							app->MarkSettingsDirty();
							InvalidateRect(hwnd, nullptr, FALSE);
							return 0;
						}
						
						// Handle Map Cast toggle slider click
						RECT adjustedMapCastToggleRect = app->m_hotbarSlotMapCastToggleRect[i];
						adjustedMapCastToggleRect.top -= (int)app->m_settingsScrollOffset;
						adjustedMapCastToggleRect.bottom -= (int)app->m_settingsScrollOffset;
						if (PtInRect(&adjustedMapCastToggleRect, pt)) {
							app->m_hotbarSlotMapCastEnabled[i] = !app->m_hotbarSlotMapCastEnabled[i];
							app->MarkSettingsDirty();
							InvalidateRect(hwnd, nullptr, FALSE);
							return 0;
						}
					}
                    }
                    return 0;
                }
                case WM_LBUTTONUP: {
                    app->m_draggingSlider = false;
                    app->m_draggingCrosshairScale = false;
                    app->m_draggingMotionBlur = false;
                    app->m_draggingGammaSlider = false;
                    app->m_draggingQuickTurnDegrees = false;
                    app->m_draggingQuickTurnSensitivity = false;
                    if (app) app->MarkSettingsDirty();
                    return 0;
                }
                
                
                case WM_DESTROY:
                    if (app) app->StopRegistryWorker(true);
                    StopBlurRendering();
                    if (g_rollM1Running) {
                        RollM1::SetEnabled(false);
                        RollM1::StopRollM1();
                        g_rollM1Running = false;
                    }
                    if (g_rollCriticalRunning) {
                        RollCritical::SetEnabled(false);
                        RollCritical::StopRollCritical();
                        g_rollCriticalRunning = false;
                    }
                    if (g_rollParryRunning) {
                        RollParry::SetEnabled(false);
                        RollParry::StopRollParry();
                        g_rollParryRunning = false;
                    }
                    if (g_rollSpitRunning) {
                        RollSpit::SetEnabled(false);
                        RollSpit::StopRollSpit();
                        g_rollSpitRunning = false;
                    }
                    if (g_parryBarRunning) {
                        ParryBar::StopPixelReading();
                        ParryBar::CleanupPixelReader();
                        g_parryBarRunning = false;
                    }
                    if (g_keystrokesRunning) {
                        Keystrokes::StopKeystrokes();
                        g_keystrokesRunning = false;
                    }
                    if (g_motionBlurRunning) {
                        MotionBlur::StopMotionBlur();
                        g_motionBlurRunning = false;
                    }
                    if (g_cpsRunning) {
                        CPS::StopCPS();
                        g_cpsRunning = false;
                    }
                    if (g_rollCastRunning) {
                        RollCast::SetEnabled(false);
                        RollCast::StopRollCast();
                        g_rollCastRunning = false;
                    }
                    if (g_holdM1Running) {
                        HoldM1::SetHoldM1Enabled(false);
                        HoldM1::StopPixelScanning();
                        g_holdM1Running = false;
                    }
                    if (g_zoomRunning) {
                        Zoom::Shutdown();
                        g_zoomRunning = false;
                    }
                    if (g_gammaRunning) {
                        StopGammaMonitor();
                    }
                    PostQuitMessage(0);
                    return 0;
            }
        }
        
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
};

void HudLayoutSaveKeystrokes(int x, int y, float scale, bool hasPos) {
	auto* inst = PistachioCreamMacro::Instance();
	if (inst) inst->UpdateHudKeystrokesLayout(x, y, scale, hasPos);
}

void HudLayoutSaveCPS(int x, int y, float scale, bool hasPos) {
	auto* inst = PistachioCreamMacro::Instance();
	if (inst) inst->UpdateHudCPSLayout(x, y, scale, hasPos);
}

void HudLayoutSaveParryBar(int x, int y, float scale, bool hasPos) {
	auto* inst = PistachioCreamMacro::Instance();
	if (inst) inst->UpdateHudParryBarLayout(x, y, scale, hasPos);
}

PistachioCreamMacro* PistachioCreamMacro::s_instance = nullptr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    PistachioCreamMacro app;
    app.Run();
    return 0;
}