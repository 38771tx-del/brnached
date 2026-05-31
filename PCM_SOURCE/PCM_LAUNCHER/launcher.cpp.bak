// launcher.cpp

#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <wincodec.h>
#include <dwrite.h>
#include <shellapi.h>
#include <shlobj.h>
#include <winhttp.h>
#include <tlhelp32.h>
#include <algorithm>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <commdlg.h>
#include "background.hpp"
#include "Panton_Black_Caps.hpp"

namespace fs = std::filesystem;

#define NANOSVG_IMPLEMENTATION
#include "svg.h"

#if defined(std)
# undef std
#endif

#pragma comment(lib, "d2d1")
#pragma comment(lib, "windowscodecs")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "winhttp.lib")

constexpr int WINDOW_WIDTH = 902;
constexpr int WINDOW_HEIGHT = 602;

RECT g_XRect{};
RECT g_MinRect{};
RECT g_ValidateRect{};
RECT g_TopRect{};          // Top rectangle (Click to login)
RECT g_SecondRect{};       // Second rectangle (Validate account)
RECT g_LeftSplitRect{};    // Left split rectangle (Website)
RECT g_RightSplitRect{};   // Right split rectangle (Discord)
RECT g_ProfileRect{};      // Profile button rectangle
bool g_HoverX = false;
bool g_HoverMin = false;
bool g_HoverValidate = false;
bool g_HoverTopRect = false;
bool g_HoverSecondRect = false;
bool g_HoverLeftSplit = false;
bool g_HoverRightSplit = false;
bool g_HoverLaunchRect = false;
bool g_HoverProfile = false;

float g_HoverXFade = 0.0f;
float g_HoverMinFade = 0.0f;
float g_HoverTopRectFade = 0.0f;
float g_HoverSecondRectFade = 0.0f;
float g_HoverLeftSplitFade = 0.0f;
float g_HoverRightSplitFade = 0.0f;
float g_HoverLaunchRectFade = 0.0f;
float g_HoverProfileFade = 0.0f;
RECT g_LaunchRect{};
bool g_Closing = false;
int g_CurrentPage = 1;
bool g_IsLoggedIn = false;
std::wstring g_AccessToken = L"";
std::wstring g_DiscordUsername = L"";

// Roblox path settings
std::wstring g_RobloxPath = L"";
std::wstring g_AutoDetectedPath = L"";  // Cached auto-detected path
std::wstring g_ManualPath = L"";         // Cached manual path
bool g_IsAutoDetectMode = true;
bool g_HasAutoDetected = false;          // Track if auto-detection has run

// Roblox instance counter
std::atomic<int> g_RobloxInstanceCount = 0;
HANDLE g_InstanceCounterThread = nullptr;
std::atomic<bool> g_InstanceCounterRunning = true;
std::vector<std::pair<DWORD, DWORD>> g_InstancePIDs; // Pairs of (RobloxPID, PCMPID)
std::mutex g_InstancePIDsMutex;
RECT g_AutoDetectButtonRect{};
RECT g_ManualButtonRect{};
RECT g_CircleCheckboxRect{};
RECT g_BrowseButtonRect{};
RECT g_RobloxPathRect{};
bool g_HoverAutoDetect = false;
bool g_HoverManual = false;
bool g_HoverBrowse = false;
bool g_HoverCircleCheckbox = false;
bool g_CircleCheckboxChecked = true;
float g_HoverAutoDetectFade = 0.0f;
float g_HoverManualFade = 0.0f;
float g_HoverBrowseFade = 0.0f;
float g_HoverCircleCheckboxFade = 0.0f;

// Logout modal state
bool g_ShowLogoutModal = false;
RECT g_LogoutModalRect{};
RECT g_LogoutCancelButtonRect{};
RECT g_LogoutConfirmButtonRect{};
bool g_HoverLogoutCancel = false;
bool g_HoverLogoutConfirm = false;
float g_HoverLogoutCancelFade = 0.0f;
float g_HoverLogoutConfirmFade = 0.0f;

// Input field state variables
RECT g_InputKeyRect{};
RECT g_TopRectInputRect{};  // Top rectangle input area
bool g_InputKeyFocused = false;
bool g_TopRectFocused = false;  // Focus state for top rectangle
std::wstring g_InputKeyText = L"";
std::wstring g_TopRectText = L"";  // Text for top rectangle input
int g_CursorPosition = 0;
int g_TopRectCursorPosition = 0;  // Cursor position for top rectangle
bool g_CursorVisible = true;
int g_CursorBlinkTimer = 0;

ID2D1Factory* pFactory = nullptr;
ID2D1DCRenderTarget* pRT = nullptr;
HDC g_dc = nullptr;
HBITMAP g_bmp = nullptr;
ID2D1Bitmap* pBG = nullptr;
ID2D1SolidColorBrush* pWhite = nullptr;
ID2D1SolidColorBrush* pHover = nullptr;
ID2D1SolidColorBrush* pUIRect = nullptr;
ID2D1SolidColorBrush* pUIOutline = nullptr;
ID2D1SolidColorBrush* pCenterFillBrush = nullptr;
ID2D1SolidColorBrush* pTransparentFillBrush = nullptr;
ID2D1SolidColorBrush* pTextBrush = nullptr;
IDWriteFactory* pWriteFactory = nullptr;
IDWriteTextFormat* pJoinServerTextFormat = nullptr;
IDWriteTextFormat* pInputUserTextFormat = nullptr;
IDWriteTextFormat* pTopRectInputTextFormat = nullptr;  // Text format for top rectangle input
IDWriteTextFormat* pBoldTextFormat = nullptr;  // Bold text format for labels (16.0f for Website/Discord)
IDWriteTextFormat* pBoldTextFormatLeft = nullptr;  // Bold text format left-aligned (16.0f for labels)
IDWriteTextFormat* pBoldTextFormatLarge = nullptr;  // Larger bold text format (18.0f for Click to login/Validate key)
IDWriteTextFormat* pWelcomeTextFormat = nullptr;  // Extra large text format for Welcome text
IDWriteTextFormat* pSubtitleTextFormat = nullptr;  // Smaller gray text format for subtitle
IDWriteTextFormat* pLaunchMenuTextFormat = nullptr;  // Text format for "LAUNCH MENU"
IDWriteTextFormat* pToggleTextFormat = nullptr;  // Smaller bold text format for toggle buttons (12.0f)
IDWriteTextFormat* pTooltipTextFormat = nullptr;  // Text format for profile tooltip
ID2D1SolidColorBrush* pGrayTextBrush = nullptr;
ID2D1SolidColorBrush* pTooltipBrush = nullptr;
ID2D1LinearGradientBrush* pTitlebarGradientBrush = nullptr;
ID2D1SolidColorBrush* pWindowBorderBrush = nullptr;
ID2D1SolidColorBrush* pButtonBgBrush = nullptr;
ID2D1SolidColorBrush* pButtonOutlineBrush = nullptr;
ID2D1SolidColorBrush* pIconBrush = nullptr;
ID2D1SolidColorBrush* pIconHoverBrush = nullptr;
ID2D1SolidColorBrush* pIconRedBrush = nullptr;
ID2D1SolidColorBrush* pSubtitleBrush = nullptr;
IDWriteTextFormat* pPCMTextFormat = nullptr;

BYTE* g_pixels = nullptr;
UINT g_w = 0, g_h = 0, g_stride = 0;

#define WM_BG_READY (WM_USER + 20)
#define WM_AUTH_RESULT (WM_APP + 101)
#define WM_TOKEN_INVALID (WM_APP + 102)
#define WM_TOKEN_COPYDATA 0x50434D54
static HANDLE g_bgThread = nullptr;
static HANDLE g_renderThread = nullptr;
static HANDLE g_tokenVerificationThread = nullptr;
static std::atomic<bool> g_tokenVerificationRunning{ false };
static HWND g_mainHwnd = nullptr;
static volatile LONG g_shouldRender = 1;

static bool PtInR(RECT r, POINT p) {
    return p.x >= r.left && p.x <= r.right &&
           p.y >= r.top && p.y <= r.bottom;
}

static void DrawXIcon(ID2D1DCRenderTarget* rt, ID2D1Factory* factory, ID2D1SolidColorBrush* brush, float x, float y, float size) {
    ID2D1PathGeometry* path = nullptr;
    if (FAILED(factory->CreatePathGeometry(&path))) return;
    ID2D1GeometrySink* sink = nullptr;
    if (FAILED(path->Open(&sink))) { path->Release(); return; }
    float halfSize = size / 2.0f;
    D2D1_POINT_2F p1 = {x - halfSize, y - halfSize};
    D2D1_POINT_2F p2 = {x + halfSize, y + halfSize};
    sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(p2);
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    D2D1_POINT_2F p3 = {x + halfSize, y - halfSize};
    D2D1_POINT_2F p4 = {x - halfSize, y + halfSize};
    sink->BeginFigure(p3, D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(p4);
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    sink->Close();
    sink->Release();
    rt->DrawGeometry(path, brush, 2.0f);
    path->Release();
}

static void DrawMinusIcon(ID2D1DCRenderTarget* rt, ID2D1Factory* factory, ID2D1SolidColorBrush* brush, float x, float y, float size) {
    ID2D1PathGeometry* path = nullptr;
    if (FAILED(factory->CreatePathGeometry(&path))) return;
    ID2D1GeometrySink* sink = nullptr;
    if (FAILED(path->Open(&sink))) { path->Release(); return; }
    float halfSize = size / 2.0f;
    D2D1_POINT_2F p1 = {x - halfSize, y};
    D2D1_POINT_2F p2 = {x + halfSize, y};
    sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(p2);
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    sink->Close();
    sink->Release();
    rt->DrawGeometry(path, brush, 2.0f);
    path->Release();
}

static void FillTopRoundedRect(ID2D1DCRenderTarget* rt, ID2D1Factory* factory, float left, float top, float right, float bottom, float radius, ID2D1Brush* brush) {
    ID2D1PathGeometry* path = nullptr;
    if (FAILED(factory->CreatePathGeometry(&path))) return;
    ID2D1GeometrySink* sink = nullptr;
    if (FAILED(path->Open(&sink))) { path->Release(); return; }
    D2D1_POINT_2F p1 = {left + radius, top};
    sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
    D2D1_POINT_2F p2 = {right - radius, top};
    sink->AddLine(p2);
    D2D1_ARC_SEGMENT arc1 = {0};
    arc1.point.x = right;
    arc1.point.y = top + radius;
    arc1.size.width = radius;
    arc1.size.height = radius;
    arc1.rotationAngle = 0.0f;
    arc1.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
    arc1.arcSize = D2D1_ARC_SIZE_SMALL;
    sink->AddArc(&arc1);
    D2D1_POINT_2F p3 = {right, bottom};
    sink->AddLine(p3);
    D2D1_POINT_2F p4 = {left, bottom};
    sink->AddLine(p4);
    D2D1_POINT_2F p5 = {left, top + radius};
    sink->AddLine(p5);
    D2D1_ARC_SEGMENT arc2 = {0};
    arc2.point.x = left + radius;
    arc2.point.y = top;
    arc2.size.width = radius;
    arc2.size.height = radius;
    arc2.rotationAngle = 0.0f;
    arc2.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;
    arc2.arcSize = D2D1_ARC_SIZE_SMALL;
    sink->AddArc(&arc2);
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    sink->Release();
    rt->FillGeometry(path, brush);
    path->Release();
}

static HRESULT CreateLinearGradientBrushForTitlebar(ID2D1DCRenderTarget* rt, float width, float height, ID2D1LinearGradientBrush** brush) {
    D2D1_GRADIENT_STOP stops[3];
    stops[0].position = 0.0f;
    stops[0].color.r = 9.0f/255.0f;
    stops[0].color.g = 9.0f/255.0f;
    stops[0].color.b = 11.0f/255.0f;
    stops[0].color.a = 1.0f;
    stops[1].position = 0.08f;
    stops[1].color.r = 9.0f/255.0f;
    stops[1].color.g = 9.0f/255.0f;
    stops[1].color.b = 11.0f/255.0f;
    stops[1].color.a = 1.0f;
    stops[2].position = 0.32f;
    stops[2].color.r = 9.0f/255.0f;
    stops[2].color.g = 9.0f/255.0f;
    stops[2].color.b = 11.0f/255.0f;
    stops[2].color.a = 0.0f;
    ID2D1GradientStopCollection* stopCollection = nullptr;
    HRESULT hr = rt->CreateGradientStopCollection(stops, 3, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &stopCollection);
    if (FAILED(hr)) return hr;
    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props = {0};
    props.startPoint.x = 0.0f;
    props.startPoint.y = 0.0f;
    props.endPoint.x = 0.0f;
    props.endPoint.y = height;
    hr = rt->CreateLinearGradientBrush(&props, nullptr, stopCollection, brush);
    stopCollection->Release();
    return hr;
}

// Background thread function
DWORD WINAPI BgThread(LPVOID param) {
    HWND h = (HWND)param;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    IWICImagingFactory* fac = nullptr;
    IWICBitmapDecoder* dec = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* conv = nullptr;
    IWICStream* stream = nullptr;

    if (SUCCEEDED(CoCreateInstance(
        CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fac)))) {

        if (SUCCEEDED(fac->CreateStream(&stream))) {
            if (SUCCEEDED(stream->InitializeFromMemory(
                (BYTE*)background_data,
                sizeof(background_data)))) {

                if (SUCCEEDED(fac->CreateDecoderFromStream(
                    stream, nullptr,
                    WICDecodeMetadataCacheOnLoad, &dec))) {

                    if (SUCCEEDED(dec->GetFrame(0, &frame))) {
                        if (SUCCEEDED(fac->CreateFormatConverter(&conv))) {

                            conv->Initialize(
                                frame,
                                GUID_WICPixelFormat32bppPBGRA,
                                WICBitmapDitherTypeNone,
                                nullptr, 0.f,
                                WICBitmapPaletteTypeCustom);

                            frame->GetSize(&g_w, &g_h);
                            g_stride = g_w * 4;
                            g_pixels = (BYTE*)CoTaskMemAlloc(g_stride * g_h);

                            if (g_pixels) {
                                conv->CopyPixels(
                                    nullptr,
                                    g_stride,
                                    g_stride * g_h,
                                    g_pixels);

                                if (!g_Closing && IsWindow(h))
                                    PostMessage(h, WM_BG_READY, 0, 0);
                            }
                        }
                    }
                }
            }
        }
    }

    if (stream) stream->Release();
    if (frame) frame->Release();
    if (dec) dec->Release();
    if (conv) conv->Release();
    if (fac) fac->Release();

    CoUninitialize();
    return 0;
}

static void StartAuthValidation(HWND hwnd, const std::wstring& token);
static void StartTokenVerification();
static void StopTokenVerification();

// Auto-detect Roblox installation path by finding the latest version folder
static std::wstring AutoDetectRobloxPath() {
    try {
        // Get LOCALAPPDATA path
        wchar_t localAppDataPath[MAX_PATH];
        if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath))) {
            return L"";
        }

        // Recursively search for folders containing "versions-" and having RobloxPlayerBeta.exe
        std::vector<std::pair<fs::path, fs::file_time_type>> candidates;

        try {
            for (const auto& entry : fs::recursive_directory_iterator(localAppDataPath, fs::directory_options::skip_permission_denied)) {
                try {
                    if (entry.is_directory()) {
                        std::wstring folderName = entry.path().filename().wstring();
                        // Convert to lowercase for case-insensitive comparison
                        std::wstring folderNameLower = folderName;
                        std::transform(folderNameLower.begin(), folderNameLower.end(), folderNameLower.begin(), ::towlower);

                        // Check if folder name contains "version-" (matches both "version-" and "versions-")
                        if (folderNameLower.find(L"version-") != std::wstring::npos) {
                            // Check if this folder contains RobloxPlayerBeta.exe
                            fs::path exePath = entry.path() / L"RobloxPlayerBeta.exe";
                            if (fs::exists(exePath) && fs::is_regular_file(exePath)) {
                                // Add to candidates with creation time
                                auto creationTime = fs::last_write_time(entry.path());
                                candidates.push_back({exePath, creationTime});
                            }
                        }
                    }
                } catch (...) {
                    // Skip folders we can't access
                    continue;
                }
            }
        } catch (...) {
            // Continue with whatever candidates we found
        }

        // If no candidates found, return empty
        if (candidates.empty()) {
            return L"";
        }

        // Sort by creation time (newest first) and return the latest
        std::sort(candidates.begin(), candidates.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });

        return candidates[0].first.wstring();
    } catch (...) {
        return L"";
    }

    return L"";
}

// Count instances and store PIDs (both Roblox AND PCM.exe must be running to count as 1 instance)
static int CountRobloxInstances() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(snapshot, &pe32)) {
        CloseHandle(snapshot);
        return 0;
    }

    std::vector<DWORD> robloxPIDs;
    std::vector<DWORD> pcmPIDs;

    do {
        // Collect Roblox PIDs
        if (_wcsicmp(pe32.szExeFile, L"RobloxPlayerBeta.exe") == 0) {
            DWORD pid = pe32.th32ProcessID;

            struct EnumData {
                DWORD processId;
                bool hasWindow;
            };
            EnumData data = { pid, false };

            EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                EnumData* data = reinterpret_cast<EnumData*>(lParam);
                DWORD windowProcessId;
                GetWindowThreadProcessId(hwnd, &windowProcessId);
                if (windowProcessId == data->processId && IsWindowVisible(hwnd)) {
                    wchar_t className[256];
                    GetClassNameW(hwnd, className, 256);
                    if (wcsstr(className, L"WINDOWSCLIENT") != nullptr ||
                        wcsstr(className, L"Roblox") != nullptr) {
                        data->hasWindow = true;
                        return FALSE;
                    }
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(&data));

            if (data.hasWindow) {
                robloxPIDs.push_back(pid);
            }
        }
        // Collect PCM.exe PIDs
        else if (_wcsicmp(pe32.szExeFile, L"PCM.exe") == 0) {
            pcmPIDs.push_back(pe32.th32ProcessID);
        }
    } while (Process32NextW(snapshot, &pe32));

    CloseHandle(snapshot);

    int instanceCount;

    // If Roblox toggle is OFF, only count PCM.exe processes
    if (!g_CircleCheckboxChecked) {
        instanceCount = (int)pcmPIDs.size();

        // Store PCM PIDs with 0 for Roblox PID (since Roblox isn't required)
        std::lock_guard<std::mutex> lock(g_InstancePIDsMutex);
        g_InstancePIDs.clear();
        for (size_t i = 0; i < pcmPIDs.size(); i++) {
            g_InstancePIDs.push_back({0, pcmPIDs[i]});
        }
    }
    // If Roblox toggle is ON, pair up Roblox and PCM PIDs
    else {
        instanceCount = (std::min)(robloxPIDs.size(), pcmPIDs.size());

        // Always clear old PIDs and rebuild fresh list (handles case when user closes manually)
        std::lock_guard<std::mutex> lock(g_InstancePIDsMutex);
        g_InstancePIDs.clear();
        for (int i = 0; i < instanceCount; i++) {
            g_InstancePIDs.push_back({robloxPIDs[i], pcmPIDs[i]});
        }
    }

    return instanceCount;
}

// Kill all stored Roblox and PCM processes
static void KillRobloxAndPCMProcesses() {
    std::vector<std::pair<DWORD, DWORD>> pidsToKill;

    // Get copy of PIDs to kill
    {
        std::lock_guard<std::mutex> lock(g_InstancePIDsMutex);
        pidsToKill = g_InstancePIDs;
    }

    // Kill all paired processes
    for (const auto& pidPair : pidsToKill) {
        DWORD robloxPID = pidPair.first;
        DWORD pcmPID = pidPair.second;

        // Kill Roblox
        HANDLE hRoblox = OpenProcess(PROCESS_TERMINATE, FALSE, robloxPID);
        if (hRoblox) {
            TerminateProcess(hRoblox, 0);
            CloseHandle(hRoblox);
        }

        // Kill PCM
        HANDLE hPCM = OpenProcess(PROCESS_TERMINATE, FALSE, pcmPID);
        if (hPCM) {
            TerminateProcess(hPCM, 0);
            CloseHandle(hPCM);
        }
    }
}

// Background thread to continuously count Roblox instances
static DWORD WINAPI InstanceCounterThread(LPVOID param) {
    while (g_InstanceCounterRunning) {
        int count = CountRobloxInstances();
        g_RobloxInstanceCount = count;
        Sleep(2000); // Check every 2 seconds
    }
    return 0;
}

static void RenderFrame(HWND hwnd);
static DWORD WINAPI RenderThread(LPVOID param) {
    HWND hwnd = (HWND)param;
    while (InterlockedCompareExchange(&g_shouldRender, 1, 1) && IsWindow(hwnd)) {
        RenderFrame(hwnd);
        Sleep(16);
    }
    return 0;
}

static void RenderFrame(HWND hwnd) {
    if (!hwnd || !pRT || !g_dc) return;

    RECT r = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    if (FAILED(pRT->BindDC(g_dc, &r))) return;


    pRT->BeginDraw();
    // Critical for rounded corners on a layered window: outside the clip must be fully transparent.
    pRT->Clear(D2D1::ColorF(0, 0, 0, 0.0f));

    const float cornerRadius = 10.0f;
    ID2D1Layer* layer = nullptr;
    ID2D1RoundedRectangleGeometry* roundedMask = nullptr;
    if (SUCCEEDED(pRT->CreateLayer(nullptr, &layer)) &&
        pFactory &&
        SUCCEEDED(pFactory->CreateRoundedRectangleGeometry(
            D2D1::RoundedRect(D2D1::RectF(0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT), cornerRadius, cornerRadius),
            &roundedMask))) {
        pRT->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), roundedMask), layer);
    }

    if (pBG)
        pRT->DrawBitmap(pBG, D2D1::RectF(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));

    float titlebarHeight = 350.0f;
    if (pTitlebarGradientBrush && pFactory) {
        FillTopRoundedRect(pRT, pFactory, 0.0f, 0.0f, (float)WINDOW_WIDTH, titlebarHeight, cornerRadius, pTitlebarGradientBrush);
    }

    // Draw custom border
    if (pWindowBorderBrush) {
        D2D1_ROUNDED_RECT borderRect = D2D1::RoundedRect(
            D2D1::RectF(0.5f, 0.5f, (float)WINDOW_WIDTH - 0.5f, (float)WINDOW_HEIGHT - 0.5f),
            cornerRadius, cornerRadius
        );
        pRT->DrawRoundedRectangle(borderRect, pWindowBorderBrush, 1.0f);
    }

    // --- TOP LEFT PCM TEXT ---
    if (pPCMTextFormat && pWhite) {
        D2D1_RECT_F pcmTextRect = D2D1::RectF(20.0f, 8.0f, 200.0f, 42.0f);
        pRT->DrawText(L"PCM", wcslen(L"PCM"), pPCMTextFormat, pcmTextRect, pWhite);
    }

    // --- (render body moved from old WM_PAINT) ---
    float fadeSpeed = 0.3f;
    if (g_HoverX) {
        if (g_HoverXFade < 1.0f) {
            g_HoverXFade += fadeSpeed;
            if (g_HoverXFade > 1.0f) g_HoverXFade = 1.0f;
        }
    } else {
        if (g_HoverXFade > 0.0f) {
            g_HoverXFade -= fadeSpeed;
            if (g_HoverXFade < 0.0f) g_HoverXFade = 0.0f;
        }
    }
    if (g_HoverMin) {
        if (g_HoverMinFade < 1.0f) {
            g_HoverMinFade += fadeSpeed;
            if (g_HoverMinFade > 1.0f) g_HoverMinFade = 1.0f;
        }
    } else {
        if (g_HoverMinFade > 0.0f) {
            g_HoverMinFade -= fadeSpeed;
            if (g_HoverMinFade < 0.0f) g_HoverMinFade = 0.0f;
        }
    }

    float buttonSize = 34.0f;
    float buttonRadius = 7.0f;
    float buttonY = 8.0f;
    float closeButtonX = WINDOW_WIDTH - 42.0f;
    float minimizeButtonX = WINDOW_WIDTH - 82.0f;

    g_XRect = { (LONG)closeButtonX, (LONG)buttonY, (LONG)(closeButtonX + buttonSize), (LONG)(buttonY + buttonSize) };
    g_MinRect = { (LONG)minimizeButtonX, (LONG)buttonY, (LONG)(minimizeButtonX + buttonSize), (LONG)(buttonY + buttonSize) };

    ID2D1SolidColorBrush* buttonBgBrush = pButtonBgBrush;
    ID2D1SolidColorBrush* buttonOutlineBrush = pButtonOutlineBrush;
    ID2D1SolidColorBrush* iconBrush = pIconBrush;
    ID2D1SolidColorBrush* iconHoverBrush = pIconHoverBrush;

    D2D1_ROUNDED_RECT closeButtonRect = D2D1::RoundedRect(
        D2D1::RectF(closeButtonX, buttonY, closeButtonX + buttonSize, buttonY + buttonSize),
        buttonRadius, buttonRadius
    );
    if (buttonBgBrush) {
        float bgOpacity = 0.15f + g_HoverXFade * 0.35f;
        buttonBgBrush->SetOpacity(bgOpacity);
        pRT->FillRoundedRectangle(closeButtonRect, buttonBgBrush);
    }
    if (buttonOutlineBrush) {
        D2D1_ROUNDED_RECT closeOutline = D2D1::RoundedRect(
            D2D1::RectF(closeButtonX + 0.5f, buttonY + 0.5f, closeButtonX + buttonSize - 0.5f, buttonY + buttonSize - 0.5f),
            buttonRadius, buttonRadius
        );
        pRT->DrawRoundedRectangle(closeOutline, buttonOutlineBrush, 1.0f);
    }
    float closeX = closeButtonX + buttonSize / 2.0f;
    float closeY = buttonY + buttonSize / 2.0f;
    ID2D1SolidColorBrush* closeIconBrush = (g_HoverXFade > 0.5f && iconHoverBrush) ? iconHoverBrush : iconBrush;
    if (closeIconBrush && pFactory) {
        DrawXIcon(pRT, pFactory, closeIconBrush, closeX, closeY, 12.0f);
    }

    D2D1_ROUNDED_RECT minimizeButtonRect = D2D1::RoundedRect(
        D2D1::RectF(minimizeButtonX, buttonY, minimizeButtonX + buttonSize, buttonY + buttonSize),
        buttonRadius, buttonRadius
    );
    if (buttonBgBrush) {
        float bgOpacity = 0.15f + g_HoverMinFade * 0.35f;
        buttonBgBrush->SetOpacity(bgOpacity);
        pRT->FillRoundedRectangle(minimizeButtonRect, buttonBgBrush);
    }
    if (buttonOutlineBrush) {
        D2D1_ROUNDED_RECT minOutline = D2D1::RoundedRect(
            D2D1::RectF(minimizeButtonX + 0.5f, buttonY + 0.5f, minimizeButtonX + buttonSize - 0.5f, buttonY + buttonSize - 0.5f),
            buttonRadius, buttonRadius
        );
        pRT->DrawRoundedRectangle(minOutline, buttonOutlineBrush, 1.0f);
    }
    float minimizeX = minimizeButtonX + buttonSize / 2.0f;
    float minimizeY = buttonY + buttonSize / 2.0f;
    ID2D1SolidColorBrush* minimizeIconBrush = (g_HoverMinFade > 0.5f && iconHoverBrush) ? iconHoverBrush : iconBrush;
    if (minimizeIconBrush && pFactory) {
        DrawMinusIcon(pRT, pFactory, minimizeIconBrush, minimizeX, minimizeY, 12.0f);
    }

    // Profile button (bottom right corner)
    float profileButtonSize = 34.0f;
    float profileButtonRadius = 7.0f;
    float profileButtonX = WINDOW_WIDTH - 42.0f;
    float profileButtonY = WINDOW_HEIGHT - 42.0f;

    g_ProfileRect = { (LONG)profileButtonX, (LONG)profileButtonY, (LONG)(profileButtonX + profileButtonSize), (LONG)(profileButtonY + profileButtonSize) };

    // Fade animation for profile button hover
    if (g_HoverProfile) {
        if (g_HoverProfileFade < 1.0f) {
            g_HoverProfileFade += fadeSpeed;
            if (g_HoverProfileFade > 1.0f) g_HoverProfileFade = 1.0f;
        }
    } else {
        if (g_HoverProfileFade > 0.0f) {
            g_HoverProfileFade -= fadeSpeed;
            if (g_HoverProfileFade < 0.0f) g_HoverProfileFade = 0.0f;
        }
    }

    D2D1_ROUNDED_RECT profileButtonRect = D2D1::RoundedRect(
        D2D1::RectF(profileButtonX, profileButtonY, profileButtonX + profileButtonSize, profileButtonY + profileButtonSize),
        profileButtonRadius, profileButtonRadius
    );
    if (buttonBgBrush) {
        float bgOpacity = 0.15f + g_HoverProfileFade * 0.35f;
        buttonBgBrush->SetOpacity(bgOpacity);
        pRT->FillRoundedRectangle(profileButtonRect, buttonBgBrush);
    }
    if (buttonOutlineBrush) {
        D2D1_ROUNDED_RECT profileOutline = D2D1::RoundedRect(
            D2D1::RectF(profileButtonX + 0.5f, profileButtonY + 0.5f, profileButtonX + profileButtonSize - 0.5f, profileButtonY + profileButtonSize - 0.5f),
            profileButtonRadius, profileButtonRadius
        );
        pRT->DrawRoundedRectangle(profileOutline, buttonOutlineBrush, 1.0f);
    }

    // Draw user icon SVG
    const char* userIconSvg = R"(
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M19 21v-2a4 4 0 0 0-4-4H9a4 4 0 0 0-4 4v2"/>
    <circle cx="12" cy="7" r="4"/>
</svg>
)";

    auto DrawUserIcon = [&](float rectX, float rectY, float rectSize) {
        char* mutableSvg = (char*)malloc(strlen(userIconSvg) + 1);
        if (!mutableSvg) return;

        strcpy(mutableSvg, userIconSvg);
        NSVGimage* image = nsvgParse(mutableSvg, "px", 96.0f);
        free(mutableSvg);
        if (!image) return;

        float iconWidth = 18.0f;
        float iconHeight = 18.0f;

        float iconX = rectX + (rectSize - iconWidth) * 0.5f;
        float iconY = rectY + (rectSize - iconHeight) * 0.5f;

        float scaleX = iconWidth / image->width;
        float scaleY = iconHeight / image->height;
        float scale = min(scaleX, scaleY);

        // Use red brush when not logged in, white/hover when logged in
        ID2D1SolidColorBrush* profileIconBrush = !g_IsLoggedIn ? pIconRedBrush : ((g_HoverProfileFade > 0.5f && iconHoverBrush) ? iconHoverBrush : iconBrush);
        if (!profileIconBrush) {
            nsvgDelete(image);
            return;
        }

        for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
            if (!(shape->flags & NSVG_FLAGS_VISIBLE)) continue;

            for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
                ID2D1PathGeometry* geometry = nullptr;
                pFactory->CreatePathGeometry(&geometry);

                ID2D1GeometrySink* sink = nullptr;
                geometry->Open(&sink);
                sink->SetFillMode(shape->fillRule == NSVG_FILLRULE_EVENODD ?
                                    D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);

                bool hasStarted = false;
                for (int i = 0; i < path->npts - 1; i += 3) {
                    float* p = &path->pts[i * 2];
                    D2D1_POINT_2F p1 = D2D1::Point2F(iconX + p[0] * scale, iconY + p[1] * scale);
                    D2D1_POINT_2F p2 = D2D1::Point2F(iconX + p[2] * scale, iconY + p[3] * scale);
                    D2D1_POINT_2F p3 = D2D1::Point2F(iconX + p[4] * scale, iconY + p[5] * scale);
                    D2D1_POINT_2F p4 = D2D1::Point2F(iconX + p[6] * scale, iconY + p[7] * scale);

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

                pRT->DrawGeometry(geometry, profileIconBrush, shape->strokeWidth * scale);
                geometry->Release();
            }
        }

        nsvgDelete(image);
    };

    DrawUserIcon(profileButtonX, profileButtonY, profileButtonSize);

    // Draw tooltip when hovering over profile button
    if (g_HoverProfile && pTooltipTextFormat && pTooltipBrush && pWhite) {
        float tooltipPadding = 10.0f;
        float tooltipHeight = 28.0f;

        // Show username if logged in, otherwise "Not logged in"
        std::wstring tooltipText = g_DiscordUsername.empty() ? L"Not logged in" : g_DiscordUsername;

        // Measure text width
        IDWriteTextLayout* layout = nullptr;
        if (pWriteFactory) {
            pWriteFactory->CreateTextLayout(tooltipText.c_str(), (UINT32)tooltipText.length(), pTooltipTextFormat, 1000.0f, 1000.0f, &layout);
            if (layout) {
                DWRITE_TEXT_METRICS metrics;
                layout->GetMetrics(&metrics);
                float tooltipWidth = metrics.width + tooltipPadding * 2.0f;

                // Position tooltip to the left of the profile button
                float tooltipX = profileButtonX - tooltipWidth - 8.0f;
                float tooltipY = profileButtonY + (profileButtonSize - tooltipHeight) * 0.5f;

                // Draw tooltip background
                D2D1_ROUNDED_RECT tooltipRect = D2D1::RoundedRect(
                    D2D1::RectF(tooltipX, tooltipY, tooltipX + tooltipWidth, tooltipY + tooltipHeight),
                    5.0f, 5.0f
                );

                pTooltipBrush->SetOpacity(g_HoverProfileFade);
                pRT->FillRoundedRectangle(tooltipRect, pTooltipBrush);

                // Draw tooltip text
                D2D1_RECT_F tooltipTextRect = D2D1::RectF(tooltipX, tooltipY, tooltipX + tooltipWidth, tooltipY + tooltipHeight);
                pWhite->SetOpacity(g_HoverProfileFade);
                pRT->DrawText(tooltipText.c_str(), (UINT32)tooltipText.length(), pTooltipTextFormat, tooltipTextRect, pWhite);
                pWhite->SetOpacity(1.0f);

                layout->Release();
            }
        }
    }

    // (persistent brushes; no releases here)

    // --- PAGE 1 CONTENT (Hide on page 2) ---
    if (g_CurrentPage == 1) {
        // --- WELCOME TEXT ---
        if (pWelcomeTextFormat && pWhite) {
            float welcomeY = 200.0f;
            D2D1_RECT_F welcomeTextRect = D2D1::RectF(0, welcomeY, WINDOW_WIDTH, welcomeY + 40.0f);
            pRT->DrawText(L"Welcome to PCM!", wcslen(L"Welcome to PCM!"), pWelcomeTextFormat, welcomeTextRect, pWhite);
        }

        // --- SUBTITLE TEXT ---
        if (pSubtitleTextFormat) {
            float subtitleY = 250.0f;
            D2D1_RECT_F subtitleTextRect = D2D1::RectF(0, subtitleY, WINDOW_WIDTH, subtitleY + 25.0f);

            if (pSubtitleBrush) {
                pRT->DrawText(L"Connect your discord account to get started.",
                              wcslen(L"Connect your discord account to get started."),
                              pSubtitleTextFormat,
                              subtitleTextRect,
                              pSubtitleBrush);
            }
        }
            // --- RECTANGLES PATCH ---
            float rectWidth = 288.0f;
            float rectHeight = 36.0f;
            float rectY = 300.0f;  // Moved up more
            float rectX = (WINDOW_WIDTH - rectWidth) / 2.0f;

            const float stroke = 1.0f;
            const float half = 0.5f;

            // Top rectangle (with outline)
            D2D1_ROUNDED_RECT rect1 = D2D1::RoundedRect(
                D2D1::RectF(rectX + half, rectY + half,
                            rectX + rectWidth - half, rectY + rectHeight - half),
                10.0f, 10.0f);

            // Set rectangle bounds for hover detection
            g_TopRect = { (LONG)rectX, (LONG)rectY, (LONG)(rectX + rectWidth), (LONG)(rectY + rectHeight) };

            if (g_IsLoggedIn) {
                if (pBoldTextFormatLarge && pWhite) {
                    std::wstring welcomeText = L"Welcome, " + g_DiscordUsername;
                    D2D1_RECT_F welcomeTextRect = D2D1::RectF(rectX, rectY, rectX + rectWidth, rectY + rectHeight);
                    pRT->DrawText(welcomeText.c_str(), (UINT32)welcomeText.length(), pBoldTextFormatLarge, welcomeTextRect, pWhite);
                }
            } else {
                float topRectOpacity = 0.15f + g_HoverTopRectFade * 0.35f;
                if (pButtonBgBrush) {
                    pButtonBgBrush->SetOpacity(topRectOpacity);
                    pRT->FillRoundedRectangle(rect1, pButtonBgBrush);
                }
                if (pButtonOutlineBrush) {
                    pRT->DrawRoundedRectangle(rect1, pButtonOutlineBrush, stroke);
                }

                if (!g_TopRectFocused && g_TopRectText.empty()) {
                    if (pBoldTextFormatLarge && pWhite) {
                        D2D1_RECT_F loginTextRect = D2D1::RectF(rectX, rectY, rectX + rectWidth, rectY + rectHeight);
                        pRT->DrawText(L"Click to login", wcslen(L"Click to login"), pBoldTextFormatLarge, loginTextRect, pWhite);
                    }
                }
            }

            // Draw input text in top rectangle
            if (g_TopRectFocused || !g_TopRectText.empty()) {
                if (pTopRectInputTextFormat && pTextBrush && pWriteFactory) {
                    float textStartX = rectX + 12.0f;  // Start from left edge with padding
                    float textY = rectY + (rectHeight - 16.0f) * 0.5f;

                    D2D1_RECT_F inputTextRect = D2D1::RectF(
                        textStartX,
                        textY,
                        rectX + rectWidth - 12.0f,  // Right edge with padding
                        textY + 17.0f
                    );

                    pRT->DrawText(
                        g_TopRectText.c_str(),
                        (UINT32)g_TopRectText.length(),
                        pTopRectInputTextFormat,
                        inputTextRect,
                        pTextBrush
                    );

                    // Draw gray cursor if focused
                    if (g_TopRectFocused && g_CursorVisible) {
                        ID2D1SolidColorBrush* cursorBrush = nullptr;
                        pRT->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.6f, 0.6f, 1.0f), &cursorBrush);

                        // Calculate cursor position based on text width
                        float cursorX = textStartX;
                        if (g_TopRectCursorPosition > 0) {
                            std::wstring textToCursor = g_TopRectText.substr(0, g_TopRectCursorPosition);
                            IDWriteTextLayout* layout = nullptr;
                            pWriteFactory->CreateTextLayout(textToCursor.c_str(), (UINT32)textToCursor.length(), pTopRectInputTextFormat, 1000.0f, 1000.0f, &layout);
                            if (layout) {
                                DWRITE_TEXT_METRICS metrics;
                                layout->GetMetrics(&metrics);
                                cursorX += metrics.width;
                                layout->Release();
                            }
                        }

                        pRT->DrawLine(
                            D2D1::Point2F(cursorX, rectY + 10.0f),
                            D2D1::Point2F(cursorX, rectY + rectHeight - 10.0f),
                            cursorBrush, 1.0f
                        );
                        cursorBrush->Release();
                    }
                }
            }

            // ---------------- SECOND RECTANGLE (BELOW TOP) ----------------
            float gap1 = 10.0f;  // Gap between top and second rectangle (reduced)
            float newRectY = rectY + rectHeight + gap1;

            D2D1_ROUNDED_RECT rect2 = D2D1::RoundedRect(
                D2D1::RectF(rectX + half, newRectY + half,
                            rectX + rectWidth - half, newRectY + rectHeight - half),
                10.0f, 10.0f);

            g_SecondRect = { (LONG)rectX, (LONG)newRectY, (LONG)(rectX + rectWidth), (LONG)(newRectY + rectHeight) };

            if (g_IsLoggedIn) {
                float secondRectOpacity = 0.15f + g_HoverSecondRectFade * 0.35f;
                if (pButtonBgBrush) {
                    pButtonBgBrush->SetOpacity(secondRectOpacity);
                    pRT->FillRoundedRectangle(rect2, pButtonBgBrush);
                }
            } else {
                if (pButtonBgBrush) {
                    pButtonBgBrush->SetOpacity(0.15f);
                    pRT->FillRoundedRectangle(rect2, pButtonBgBrush);
                }
            }
            if (pButtonOutlineBrush) {
                pRT->DrawRoundedRectangle(rect2, pButtonOutlineBrush, stroke);
            }

            // Draw "Next" text - white if logged in, gray if disabled
            if (pBoldTextFormatLarge) {
                D2D1_RECT_F validateTextRect = D2D1::RectF(rectX, newRectY, rectX + rectWidth, newRectY + rectHeight);
                if (g_IsLoggedIn && pWhite) {
                    pRT->DrawText(L"Next", wcslen(L"Next"), pBoldTextFormatLarge, validateTextRect, pWhite);
                } else if (pGrayTextBrush) {
                    pRT->DrawText(L"Next", wcslen(L"Next"), pBoldTextFormatLarge, validateTextRect, pGrayTextBrush);
                }
            }

            // ---------------- BOTTOM RECTANGLES ----------------
            float splitRectWidth = (rectWidth - 10.0f) / 2.0f;
            float rect2Height = 40.0f;
            float rectGap = 10.0f;
            float rect2Y = newRectY + rectHeight + rectGap;
            float leftSplitRectX = rectX;
            float rightSplitRectX = rectX + splitRectWidth + 10.0f;

            D2D1_ROUNDED_RECT leftSplitRect = D2D1::RoundedRect(
                D2D1::RectF(leftSplitRectX + half, rect2Y + half,
                            leftSplitRectX + splitRectWidth - half, rect2Y + rect2Height - half),
                10.0f, 10.0f);

            D2D1_ROUNDED_RECT rightSplitRect = D2D1::RoundedRect(
                D2D1::RectF(rightSplitRectX + half, rect2Y + half,
                            rightSplitRectX + splitRectWidth - half, rect2Y + rect2Height - half),
                10.0f, 10.0f);

            g_LeftSplitRect = { (LONG)leftSplitRectX, (LONG)rect2Y, (LONG)(leftSplitRectX + splitRectWidth), (LONG)(rect2Y + rect2Height) };
            g_RightSplitRect = { (LONG)rightSplitRectX, (LONG)rect2Y, (LONG)(rightSplitRectX + splitRectWidth), (LONG)(rect2Y + rect2Height) };

            float leftSplitOpacity = 0.15f + g_HoverLeftSplitFade * 0.35f;
            if (pButtonBgBrush) {
                pButtonBgBrush->SetOpacity(leftSplitOpacity);
                pRT->FillRoundedRectangle(leftSplitRect, pButtonBgBrush);
            }

            float rightSplitOpacity = 0.15f + g_HoverRightSplitFade * 0.35f;
            if (pButtonBgBrush) {
                pButtonBgBrush->SetOpacity(rightSplitOpacity);
                pRT->FillRoundedRectangle(rightSplitRect, pButtonBgBrush);
            }

            if (pButtonOutlineBrush) {
                pRT->DrawRoundedRectangle(leftSplitRect, pButtonOutlineBrush, stroke);
                pRT->DrawRoundedRectangle(rightSplitRect, pButtonOutlineBrush, stroke);
            }

            if (pBoldTextFormat && pWhite) {
                float textOffsetX = -10.0f;
                float textOffsetY = -2.0f;

                D2D1_RECT_F websiteTextRect = D2D1::RectF(
                    leftSplitRectX + textOffsetX,
                    rect2Y + textOffsetY,
                    leftSplitRectX + splitRectWidth + textOffsetX,
                    rect2Y + rect2Height + textOffsetY);
                pRT->DrawText(L"Website", wcslen(L"Website"), pBoldTextFormat, websiteTextRect, pWhite);

                D2D1_RECT_F discordTextRect = D2D1::RectF(
                    rightSplitRectX + textOffsetX,
                    rect2Y + textOffsetY,
                    rightSplitRectX + splitRectWidth + textOffsetX,
                    rect2Y + rect2Height + textOffsetY);
                pRT->DrawText(L"Discord", wcslen(L"Discord"), pBoldTextFormat, discordTextRect, pWhite);
            }

            // ---------------- SVG ICONS FOR BOTTOM RECTANGLES ----------------
            const char* bottomIconSvg = R"(
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M21 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h6"/>
    <path d="m21 3-9 9"/>
    <path d="M15 3h6v6"/>
</svg>
)";

            auto DrawSvgInRect = [&](float rectX, float rectY, float rectWidth, float rectHeight, bool isHovering) {
                char* mutableSvg = (char*)malloc(strlen(bottomIconSvg) + 1);
                if (!mutableSvg) return;

                strcpy(mutableSvg, bottomIconSvg);
                NSVGimage* image = nsvgParse(mutableSvg, "px", 96.0f);
                free(mutableSvg);
                if (!image) return;

                float iconWidth = 20.0f;
                float iconHeight = 20.0f;

                float iconX = rectX + rectWidth - 16.0f - iconWidth;
                float iconY = rectY + (rectHeight - iconHeight) * 0.5f;

                float scaleX = iconWidth / image->width;
                float scaleY = iconHeight / image->height;
                float scale = min(scaleX, scaleY);

                ID2D1SolidColorBrush* svgIconBrush = isHovering ? iconHoverBrush : iconBrush;
                if (!svgIconBrush) {
                    nsvgDelete(image);
                    return;
                }

                for (NSVGshape* shape = image->shapes; shape != nullptr; shape = shape->next) {
                    if (!(shape->flags & NSVG_FLAGS_VISIBLE)) continue;

                    for (NSVGpath* path = shape->paths; path != nullptr; path = path->next) {
                        ID2D1PathGeometry* geometry = nullptr;
                        pFactory->CreatePathGeometry(&geometry);

                        ID2D1GeometrySink* sink = nullptr;
                        geometry->Open(&sink);
                        sink->SetFillMode(shape->fillRule == NSVG_FILLRULE_EVENODD ?
                                            D2D1_FILL_MODE_ALTERNATE : D2D1_FILL_MODE_WINDING);

                        bool hasStarted = false;
                        for (int i = 0; i < path->npts - 1; i += 3) {
                            float* p = &path->pts[i * 2];
                            D2D1_POINT_2F p1 = D2D1::Point2F(iconX + p[0] * scale, iconY + p[1] * scale);
                            D2D1_POINT_2F p2 = D2D1::Point2F(iconX + p[2] * scale, iconY + p[3] * scale);
                            D2D1_POINT_2F p3 = D2D1::Point2F(iconX + p[4] * scale, iconY + p[5] * scale);
                            D2D1_POINT_2F p4 = D2D1::Point2F(iconX + p[6] * scale, iconY + p[7] * scale);

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

                        pRT->DrawGeometry(geometry, svgIconBrush, shape->strokeWidth * scale);
                        geometry->Release();
                    }
                }

                nsvgDelete(image);
            };

            DrawSvgInRect(leftSplitRectX, rect2Y, splitRectWidth, rect2Height, g_HoverLeftSplitFade > 0.5f);
            DrawSvgInRect(rightSplitRectX, rect2Y, splitRectWidth, rect2Height, g_HoverRightSplitFade > 0.5f);
        }  // End of page 1 content

    // --- PAGE 2 CONTENT ---
    if (g_CurrentPage == 2) {
        float rectWidth = 325.0f;
        float rectHeight = 75.0f;
        float rectX = (WINDOW_WIDTH - rectWidth) / 2.0f;
        float rectY = 220.0f;

        const float stroke = 1.0f;
        const float half = 0.5f;

        D2D1_ROUNDED_RECT launchRect = D2D1::RoundedRect(
            D2D1::RectF(rectX + half, rectY + half,
                        rectX + rectWidth - half, rectY + rectHeight - half),
            10.0f, 10.0f);

        g_LaunchRect = { (LONG)rectX, (LONG)rectY, (LONG)(rectX + rectWidth), (LONG)(rectY + rectHeight) };

        // Check instance count to determine button state
        bool hasInstances = g_RobloxInstanceCount.load() > 0;
        
        // Check if button should be disabled (auto-detect mode but path not found yet)
        bool isLaunchDisabled = g_IsAutoDetectMode && (g_RobloxPath.empty() || g_RobloxPath == L"Roblox not found" || !g_HasAutoDetected);

        // Create and fill with gradient (green for launch, red for close)
        ID2D1LinearGradientBrush* buttonGradientBrush = nullptr;
        ID2D1GradientStopCollection* buttonGradientStops = nullptr;

        // Darken gradient subtly on hover (only if not disabled)
        float darkenFactor = isLaunchDisabled ? 1.0f : (1.0f - (g_HoverLaunchRectFade * 0.15f));
        float buttonOpacity = isLaunchDisabled ? 0.5f : 1.0f;

        D2D1_GRADIENT_STOP buttonStops[3];
        if (hasInstances) {
            // Red gradient for "CLOSE MENU"
            buttonStops[0].color = D2D1::ColorF(
                (0xE8/255.0f) * darkenFactor,
                (0x30/255.0f) * darkenFactor,
                (0x30/255.0f) * darkenFactor,
                1.0f);  // Bright red at top
            buttonStops[0].position = 0.0f;
            buttonStops[1].color = D2D1::ColorF(
                (0xC8/255.0f) * darkenFactor,
                (0x20/255.0f) * darkenFactor,
                (0x20/255.0f) * darkenFactor,
                1.0f);  // Mid red
            buttonStops[1].position = 0.5f;
            buttonStops[2].color = D2D1::ColorF(
                (0xA8/255.0f) * darkenFactor,
                (0x15/255.0f) * darkenFactor,
                (0x15/255.0f) * darkenFactor,
                1.0f);  // Darker red at bottom
            buttonStops[2].position = 1.0f;
        } else {
            // Green gradient for "LAUNCH MENU"
            buttonStops[0].color = D2D1::ColorF(
                (0x20/255.0f) * darkenFactor,
                (0xD8/255.0f) * darkenFactor,
                (0x6E/255.0f) * darkenFactor,
                1.0f);  // Bright green at top
            buttonStops[0].position = 0.0f;
            buttonStops[1].color = D2D1::ColorF(
                (0x18/255.0f) * darkenFactor,
                (0xC0/255.0f) * darkenFactor,
                (0x65/255.0f) * darkenFactor,
                1.0f);  // Mid green
            buttonStops[1].position = 0.5f;
            buttonStops[2].color = D2D1::ColorF(
                (0x12/255.0f) * darkenFactor,
                (0xAA/255.0f) * darkenFactor,
                (0x5C/255.0f) * darkenFactor,
                1.0f);  // Darker green at bottom
            buttonStops[2].position = 1.0f;
        }

        HRESULT gradientHr = pRT->CreateGradientStopCollection(buttonStops, 3, &buttonGradientStops);
        if (SUCCEEDED(gradientHr)) {
            gradientHr = pRT->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(launchRect.rect.left, launchRect.rect.top),
                    D2D1::Point2F(launchRect.rect.left, launchRect.rect.bottom)
                ),
                buttonGradientStops,
                &buttonGradientBrush
            );
            if (SUCCEEDED(gradientHr)) {
                buttonGradientBrush->SetOpacity(buttonOpacity);
                pRT->FillRoundedRectangle(launchRect, buttonGradientBrush);
            }
        }

        // Release gradient resources
        if (buttonGradientBrush) buttonGradientBrush->Release();
        if (buttonGradientStops) buttonGradientStops->Release();

        // No border for the gradient button

        if (pLaunchMenuTextFormat && pWhite) {
            D2D1_RECT_F launchTextRect = D2D1::RectF(rectX, rectY, rectX + rectWidth, rectY + rectHeight);
            const wchar_t* buttonText = hasInstances ? L"CLOSE MENU" : L"LAUNCH MENU";
            pWhite->SetOpacity(buttonOpacity);
            pRT->DrawText(buttonText, wcslen(buttonText), pLaunchMenuTextFormat, launchTextRect, pWhite);
            pWhite->SetOpacity(1.0f);
        }

        // Instance counter indicator (top bar, left of minimize button)
        float instanceWidth = 140.0f;
        float instanceHeight = 26.0f;
        float minimizeButtonX = WINDOW_WIDTH - 82.0f;
        float buttonY = 8.0f;
        float buttonSize = 34.0f;
        float instanceX = minimizeButtonX - instanceWidth - 10.0f;
        float instanceY = buttonY + (buttonSize - instanceHeight) / 2.0f;

        D2D1_ROUNDED_RECT instanceRect = D2D1::RoundedRect(
            D2D1::RectF(instanceX, instanceY, instanceX + instanceWidth, instanceY + instanceHeight),
            4.0f, 4.0f);

        // Draw background (same opacity as X and minimize buttons)
        if (pButtonBgBrush) {
            pButtonBgBrush->SetOpacity(0.15f);
            pRT->FillRoundedRectangle(instanceRect, pButtonBgBrush);
        }

        // Draw border
        if (pButtonOutlineBrush) {
            pRT->DrawRoundedRectangle(instanceRect, pButtonOutlineBrush, 1.0f);
        }

        // Draw text with actual instance count
        if (pToggleTextFormat && pWhite) {
            int count = g_RobloxInstanceCount.load();
            wchar_t instanceText[64];
            swprintf_s(instanceText, L"%d Instance%s Running", count, count == 1 ? L"" : L"s");
            D2D1_RECT_F instanceTextRect = D2D1::RectF(instanceX, instanceY, instanceX + instanceWidth, instanceY + instanceHeight);
            pRT->DrawText(instanceText, wcslen(instanceText), pToggleTextFormat, instanceTextRect, pWhite);
        }

        // --- SETTINGS SECTION ---
        float settingsY = rectY + rectHeight + 100.0f;
        float settingsSectionWidth = 500.0f;
        float settingsSectionX = (WINDOW_WIDTH - settingsSectionWidth) / 2.0f;

        // "Roblox Path:" label on top (left-aligned)
        float labelHeight = 20.0f;
        if (pBoldTextFormatLeft && pWhite) {
            D2D1_RECT_F labelRect = D2D1::RectF(settingsSectionX, settingsY, settingsSectionX + 200.0f, settingsY + labelHeight);
            pRT->DrawText(L"Roblox Path:", wcslen(L"Roblox Path:"), pBoldTextFormatLeft, labelRect, pWhite);
        }

        // Unified toggle control (embedded together, no gaps) - on second line, left-aligned
        float toggleY = settingsY + labelHeight + 5.0f;
        float tabStartX = settingsSectionX;
        float tabWidth = 85.0f;
        float tabHeight = 26.0f;

        // Update hit test rectangles (no gap between them)
        g_AutoDetectButtonRect = { (LONG)tabStartX, (LONG)toggleY, (LONG)(tabStartX + tabWidth), (LONG)(toggleY + tabHeight) };
        g_ManualButtonRect = { (LONG)(tabStartX + tabWidth), (LONG)toggleY, (LONG)(tabStartX + tabWidth * 2), (LONG)(toggleY + tabHeight) };

        // Draw single outer border around both tabs
        D2D1_ROUNDED_RECT outerToggleBorder = D2D1::RoundedRect(
            D2D1::RectF(tabStartX, toggleY, tabStartX + tabWidth * 2, toggleY + tabHeight),
            4.0f, 4.0f);
        if (pButtonOutlineBrush) {
            pRT->DrawRoundedRectangle(outerToggleBorder, pButtonOutlineBrush, 1.0f);
        }

        // Fill background only for selected tab
        if (pButtonBgBrush) {
            if (g_IsAutoDetectMode) {
                // Fill auto-detect tab (left side with left corners rounded)
                D2D1_ROUNDED_RECT autoDetectFill = D2D1::RoundedRect(
                    D2D1::RectF(tabStartX, toggleY, tabStartX + tabWidth, toggleY + tabHeight),
                    4.0f, 4.0f);
                pButtonBgBrush->SetOpacity(0.50f);
                pRT->FillRoundedRectangle(autoDetectFill, pButtonBgBrush);
            } else {
                // Fill manual tab (right side with right corners rounded)
                D2D1_ROUNDED_RECT manualFill = D2D1::RoundedRect(
                    D2D1::RectF(tabStartX + tabWidth, toggleY, tabStartX + tabWidth * 2, toggleY + tabHeight),
                    4.0f, 4.0f);
                pButtonBgBrush->SetOpacity(0.50f);
                pRT->FillRoundedRectangle(manualFill, pButtonBgBrush);
            }
        }

        // Draw subtle divider line between tabs
        if (pButtonOutlineBrush) {
            pButtonOutlineBrush->SetOpacity(0.3f);
            pRT->DrawLine(
                D2D1::Point2F(tabStartX + tabWidth, toggleY + 4.0f),
                D2D1::Point2F(tabStartX + tabWidth, toggleY + tabHeight - 4.0f),
                pButtonOutlineBrush,
                1.0f
            );
            pButtonOutlineBrush->SetOpacity(1.0f);
        }

        // Draw text for both tabs (centered, using smaller bold font)
        if (pToggleTextFormat && pWhite) {
            D2D1_RECT_F autoTextRect = D2D1::RectF(tabStartX, toggleY, tabStartX + tabWidth, toggleY + tabHeight);
            pRT->DrawText(L"Auto-detect", wcslen(L"Auto-detect"), pToggleTextFormat, autoTextRect, pWhite);

            D2D1_RECT_F manualTextRect = D2D1::RectF(tabStartX + tabWidth, toggleY, tabStartX + tabWidth * 2, toggleY + tabHeight);
            pRT->DrawText(L"Manual", wcslen(L"Manual"), pToggleTextFormat, manualTextRect, pWhite);
        }

        // Circle checkbox to the right of Manual tab
        float circleSize = 18.0f;
        float circleX = tabStartX + tabWidth * 2 + 12.0f;
        float circleY = toggleY + (tabHeight - circleSize) / 2.0f;

        D2D1_ELLIPSE circle = D2D1::Ellipse(
            D2D1::Point2F(circleX + circleSize / 2.0f, circleY + circleSize / 2.0f),
            circleSize / 2.0f,
            circleSize / 2.0f
        );

        g_CircleCheckboxRect = { (LONG)circleX, (LONG)circleY, (LONG)(circleX + circleSize), (LONG)(circleY + circleSize) };

        // Draw circle border
        if (pButtonOutlineBrush) {
            pRT->DrawEllipse(circle, pButtonOutlineBrush, 1.5f);
        }

        // Draw checkmark if checked
        if (g_CircleCheckboxChecked && pWhite) {
            float checkSize = circleSize * 0.5f;
            float checkCenterX = circleX + circleSize / 2.0f;
            float checkCenterY = circleY + circleSize / 2.0f;

            // Draw checkmark as two lines
            pRT->DrawLine(
                D2D1::Point2F(checkCenterX - checkSize * 0.4f, checkCenterY),
                D2D1::Point2F(checkCenterX - checkSize * 0.1f, checkCenterY + checkSize * 0.35f),
                pWhite,
                2.0f
            );
            pRT->DrawLine(
                D2D1::Point2F(checkCenterX - checkSize * 0.1f, checkCenterY + checkSize * 0.35f),
                D2D1::Point2F(checkCenterX + checkSize * 0.45f, checkCenterY - checkSize * 0.4f),
                pWhite,
                2.0f
            );
        }

        // Draw tooltip when hovering over circle checkbox
        if (g_HoverCircleCheckbox && pTooltipTextFormat && pTooltipBrush && pWhite) {
            float tooltipPadding = 10.0f;
            float tooltipHeight = 28.0f;

            std::wstring tooltipText = L"Launch Roblox with menu";

            // Measure text width
            IDWriteTextLayout* layout = nullptr;
            if (pWriteFactory) {
                pWriteFactory->CreateTextLayout(tooltipText.c_str(), (UINT32)tooltipText.length(), pTooltipTextFormat, 1000.0f, 1000.0f, &layout);
                if (layout) {
                    DWRITE_TEXT_METRICS metrics;
                    layout->GetMetrics(&metrics);
                    float tooltipWidth = metrics.width + tooltipPadding * 2.0f;

                    // Position tooltip to the right of the circle checkbox
                    float tooltipX = circleX + circleSize + 8.0f;
                    float tooltipY = circleY + (circleSize - tooltipHeight) * 0.5f;

                    // Draw tooltip background
                    D2D1_ROUNDED_RECT tooltipRect = D2D1::RoundedRect(
                        D2D1::RectF(tooltipX, tooltipY, tooltipX + tooltipWidth, tooltipY + tooltipHeight),
                        5.0f, 5.0f
                    );

                    pTooltipBrush->SetOpacity(g_HoverCircleCheckboxFade);
                    pRT->FillRoundedRectangle(tooltipRect, pTooltipBrush);
                    pTooltipBrush->SetOpacity(1.0f);

                    // Draw tooltip text
                    D2D1_RECT_F tooltipTextRect = D2D1::RectF(tooltipX, tooltipY, tooltipX + tooltipWidth, tooltipY + tooltipHeight);
                    pWhite->SetOpacity(g_HoverCircleCheckboxFade);
                    pRT->DrawText(tooltipText.c_str(), (UINT32)tooltipText.length(), pTooltipTextFormat, tooltipTextRect, pWhite);
                    pWhite->SetOpacity(1.0f);

                    layout->Release();
                }
            }
        }

        // Roblox path text box
        float pathY = toggleY + tabHeight + 10.0f;
        float pathWidth = settingsSectionWidth - 50.0f;
        float pathHeight = 35.0f;

        D2D1_ROUNDED_RECT pathBox = D2D1::RoundedRect(
            D2D1::RectF(settingsSectionX, pathY, settingsSectionX + pathWidth, pathY + pathHeight),
            5.0f, 5.0f);
        g_RobloxPathRect = { (LONG)settingsSectionX, (LONG)pathY, (LONG)(settingsSectionX + pathWidth), (LONG)(pathY + pathHeight) };

        // Draw path box (darker if disabled in auto mode)
        float pathOpacity = g_IsAutoDetectMode ? 0.10f : 0.15f;
        if (pButtonBgBrush) {
            pButtonBgBrush->SetOpacity(pathOpacity);
            pRT->FillRoundedRectangle(pathBox, pButtonBgBrush);
        }
        if (pButtonOutlineBrush) {
            pRT->DrawRoundedRectangle(pathBox, pButtonOutlineBrush, 1.0f);
        }

        // Draw path text (truncate in middle if too long)
        if (pInputUserTextFormat && pTextBrush && !g_RobloxPath.empty()) {
            std::wstring displayPath = g_RobloxPath;

            // Truncate middle if path is too long (more than 60 characters)
            if (displayPath.length() > 60) {
                size_t startLen = 30;  // Show first 30 chars
                size_t endLen = 27;    // Show last 27 chars
                displayPath = displayPath.substr(0, startLen) + L"..." + displayPath.substr(displayPath.length() - endLen);
            }

            D2D1_RECT_F pathTextRect = D2D1::RectF(settingsSectionX + 10.0f, pathY, settingsSectionX + pathWidth - 10.0f, pathY + pathHeight);
            float textOpacity = g_IsAutoDetectMode ? 0.5f : 1.0f;
            pTextBrush->SetOpacity(textOpacity);
            pRT->DrawText(displayPath.c_str(), (UINT32)displayPath.length(), pInputUserTextFormat, pathTextRect, pTextBrush);
            pTextBrush->SetOpacity(1.0f);
        }

        // Browse button (three dots)
        float browseX = settingsSectionX + pathWidth + 10.0f;
        float browseSize = 35.0f;

        D2D1_ROUNDED_RECT browseButton = D2D1::RoundedRect(
            D2D1::RectF(browseX, pathY, browseX + browseSize, pathY + browseSize),
            5.0f, 5.0f);
        g_BrowseButtonRect = { (LONG)browseX, (LONG)pathY, (LONG)(browseX + browseSize), (LONG)(pathY + browseSize) };

        // Draw browse button (disabled look in auto mode)
        float browseOpacity = g_IsAutoDetectMode ? 0.10f : (0.15f + g_HoverBrowseFade * 0.35f);
        if (pButtonBgBrush) {
            pButtonBgBrush->SetOpacity(browseOpacity);
            pRT->FillRoundedRectangle(browseButton, pButtonBgBrush);
        }
        if (pButtonOutlineBrush) {
            pRT->DrawRoundedRectangle(browseButton, pButtonOutlineBrush, 1.0f);
        }

        // Draw "..." text
        if (pBoldTextFormatLarge && pWhite) {
            D2D1_RECT_F dotsRect = D2D1::RectF(browseX, pathY, browseX + browseSize, pathY + browseSize);
            float dotsOpacity = g_IsAutoDetectMode ? 0.3f : 1.0f;
            pWhite->SetOpacity(dotsOpacity);
            pRT->DrawText(L"...", wcslen(L"..."), pBoldTextFormatLarge, dotsRect, pWhite);
            pWhite->SetOpacity(1.0f);
        }
    }

    // --- LOGOUT CONFIRMATION MODAL ---
    if (g_ShowLogoutModal && g_IsLoggedIn) {
        // Modal dimensions
        float modalWidth = 400.0f;
        float modalHeight = 140.0f;
        float modalX = (WINDOW_WIDTH - modalWidth) / 2.0f;
        float modalY = (WINDOW_HEIGHT - modalHeight) / 2.0f;

        g_LogoutModalRect = { (LONG)modalX, (LONG)modalY, (LONG)(modalX + modalWidth), (LONG)(modalY + modalHeight) };

        // Draw modal background (full opacity)
        D2D1_ROUNDED_RECT modalRect = D2D1::RoundedRect(
            D2D1::RectF(modalX, modalY, modalX + modalWidth, modalY + modalHeight),
            10.0f, 10.0f
        );

        if (pCenterFillBrush) {
            pRT->FillRoundedRectangle(modalRect, pCenterFillBrush);
        }
        if (pButtonOutlineBrush) {
            pRT->DrawRoundedRectangle(modalRect, pButtonOutlineBrush, 1.0f);
        }

        // Modal title text
        if (pBoldTextFormatLarge && pWhite) {
            D2D1_RECT_F titleRect = D2D1::RectF(modalX, modalY + 20.0f, modalX + modalWidth, modalY + 60.0f);
            pRT->DrawText(L"Confirm Logout", wcslen(L"Confirm Logout"), pBoldTextFormatLarge, titleRect, pWhite);
        }

        // Modal message text
        if (pSubtitleTextFormat && pSubtitleBrush) {
            D2D1_RECT_F messageRect = D2D1::RectF(modalX, modalY + 50.0f, modalX + modalWidth, modalY + 80.0f);
            pRT->DrawText(L"Are you sure you want to log out?", wcslen(L"Are you sure you want to log out?"), pSubtitleTextFormat, messageRect, pSubtitleBrush);
        }

        // Buttons
        float buttonWidth = 150.0f;
        float buttonHeight = 36.0f;
        float buttonGap = 20.0f;
        float totalButtonWidth = buttonWidth * 2 + buttonGap;
        float buttonStartX = modalX + (modalWidth - totalButtonWidth) / 2.0f;
        float buttonY = modalY + modalHeight - buttonHeight - 20.0f;

        // Cancel button (left)
        float cancelX = buttonStartX;
        g_LogoutCancelButtonRect = { (LONG)cancelX, (LONG)buttonY, (LONG)(cancelX + buttonWidth), (LONG)(buttonY + buttonHeight) };

        // Fade animations
        if (g_HoverLogoutCancel) {
            if (g_HoverLogoutCancelFade < 1.0f) {
                g_HoverLogoutCancelFade += 0.3f;
                if (g_HoverLogoutCancelFade > 1.0f) g_HoverLogoutCancelFade = 1.0f;
            }
        } else {
            if (g_HoverLogoutCancelFade > 0.0f) {
                g_HoverLogoutCancelFade -= 0.3f;
                if (g_HoverLogoutCancelFade < 0.0f) g_HoverLogoutCancelFade = 0.0f;
            }
        }

        if (g_HoverLogoutConfirm) {
            if (g_HoverLogoutConfirmFade < 1.0f) {
                g_HoverLogoutConfirmFade += 0.3f;
                if (g_HoverLogoutConfirmFade > 1.0f) g_HoverLogoutConfirmFade = 1.0f;
            }
        } else {
            if (g_HoverLogoutConfirmFade > 0.0f) {
                g_HoverLogoutConfirmFade -= 0.3f;
                if (g_HoverLogoutConfirmFade < 0.0f) g_HoverLogoutConfirmFade = 0.0f;
            }
        }

        D2D1_ROUNDED_RECT cancelButtonRect = D2D1::RoundedRect(
            D2D1::RectF(cancelX, buttonY, cancelX + buttonWidth, buttonY + buttonHeight),
            10.0f, 10.0f
        );

        if (pButtonBgBrush) {
            float bgOpacity = 0.15f + g_HoverLogoutCancelFade * 0.35f;
            pButtonBgBrush->SetOpacity(bgOpacity);
            pRT->FillRoundedRectangle(cancelButtonRect, pButtonBgBrush);
        }
        if (pButtonOutlineBrush) {
            pRT->DrawRoundedRectangle(cancelButtonRect, pButtonOutlineBrush, 1.0f);
        }

        if (pBoldTextFormatLarge && pWhite) {
            D2D1_RECT_F cancelTextRect = D2D1::RectF(cancelX, buttonY, cancelX + buttonWidth, buttonY + buttonHeight);
            pRT->DrawText(L"Cancel", wcslen(L"Cancel"), pBoldTextFormatLarge, cancelTextRect, pWhite);
        }

        // Logout button (right) - red
        float logoutX = cancelX + buttonWidth + buttonGap;
        g_LogoutConfirmButtonRect = { (LONG)logoutX, (LONG)buttonY, (LONG)(logoutX + buttonWidth), (LONG)(buttonY + buttonHeight) };

        // Create red gradient for logout button
        ID2D1LinearGradientBrush* logoutGradientBrush = nullptr;
        ID2D1GradientStopCollection* logoutGradientStops = nullptr;

        float darkenFactor = 1.0f - (g_HoverLogoutConfirmFade * 0.15f);
        D2D1_GRADIENT_STOP logoutStops[3];
        logoutStops[0].color = D2D1::ColorF(
            (0xE8/255.0f) * darkenFactor,
            (0x30/255.0f) * darkenFactor,
            (0x30/255.0f) * darkenFactor,
            1.0f);
        logoutStops[0].position = 0.0f;
        logoutStops[1].color = D2D1::ColorF(
            (0xC8/255.0f) * darkenFactor,
            (0x20/255.0f) * darkenFactor,
            (0x20/255.0f) * darkenFactor,
            1.0f);
        logoutStops[1].position = 0.5f;
        logoutStops[2].color = D2D1::ColorF(
            (0xA8/255.0f) * darkenFactor,
            (0x15/255.0f) * darkenFactor,
            (0x15/255.0f) * darkenFactor,
            1.0f);
        logoutStops[2].position = 1.0f;

        HRESULT gradientHr = pRT->CreateGradientStopCollection(logoutStops, 3, &logoutGradientStops);
        if (SUCCEEDED(gradientHr)) {
            gradientHr = pRT->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(logoutX, buttonY),
                    D2D1::Point2F(logoutX, buttonY + buttonHeight)
                ),
                logoutGradientStops,
                &logoutGradientBrush
            );
            if (SUCCEEDED(gradientHr)) {
                D2D1_ROUNDED_RECT logoutButtonRect = D2D1::RoundedRect(
                    D2D1::RectF(logoutX, buttonY, logoutX + buttonWidth, buttonY + buttonHeight),
                    10.0f, 10.0f
                );
                pRT->FillRoundedRectangle(logoutButtonRect, logoutGradientBrush);
            }
        }

        if (logoutGradientBrush) logoutGradientBrush->Release();
        if (logoutGradientStops) logoutGradientStops->Release();

        if (pBoldTextFormatLarge && pWhite) {
            D2D1_RECT_F logoutTextRect = D2D1::RectF(logoutX, buttonY, logoutX + buttonWidth, buttonY + buttonHeight);
            pRT->DrawText(L"Logout", wcslen(L"Logout"), pBoldTextFormatLarge, logoutTextRect, pWhite);
        }
    }

    if (layer && roundedMask) {
        pRT->PopLayer();
    }
    if (layer) layer->Release();
    if (roundedMask) roundedMask->Release();

    pRT->EndDraw();

    HDC s = GetDC(NULL);
    if (s) {
        POINT src = {0, 0}, dst;
        RECT wr;
        GetWindowRect(hwnd, &wr);
        dst.x = wr.left; dst.y = wr.top;
        SIZE sz = {WINDOW_WIDTH, WINDOW_HEIGHT};
        BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        UpdateLayeredWindow(hwnd, s, &dst, &sz, g_dc, &src, 0, &bf, ULW_ALPHA);
        ReleaseDC(NULL, s);
    }
}

// ---------------- WINDOW PROC ----------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {

        case WM_AUTH_RESULT: {
            g_IsLoggedIn = (w != 0);
            if (g_IsLoggedIn) {
                StartTokenVerification();
            } else {
                StopTokenVerification();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_TOKEN_INVALID: {
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                RegDeleteValueW(hKey, L"AccessToken");
                RegCloseKey(hKey);
            }

            g_AccessToken.clear();
            g_DiscordUsername.clear();
            g_IsLoggedIn = false;
            g_ShowLogoutModal = false;
            g_CurrentPage = 1;

            StopTokenVerification();

            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_COPYDATA: {
            COPYDATASTRUCT* cds = (COPYDATASTRUCT*)l;
            if (cds && cds->dwData == WM_TOKEN_COPYDATA && cds->lpData && cds->cbData >= sizeof(wchar_t)) {
                const wchar_t* s = (const wchar_t*)cds->lpData;
                std::wstring token(s);
                if (!token.empty()) {
                    StartAuthValidation(hwnd, token);
                }
                return TRUE;
            }
            break;
        }

        case WM_CREATE: {
            g_mainHwnd = hwnd;
            g_bgThread = CreateThread(NULL, 0, BgThread, hwnd, 0, NULL);

            // Start instance counter thread
            g_InstanceCounterRunning = true;
            g_InstanceCounterThread = CreateThread(NULL, 0, InstanceCounterThread, NULL, 0, NULL);

            // Auto-detect Roblox path once on startup in background
            std::thread([]() {
                std::wstring detectedPath = AutoDetectRobloxPath();
                g_AutoDetectedPath = detectedPath.empty() ? L"Roblox not found" : detectedPath;
                // Only update g_RobloxPath if in auto-detect mode
                if (g_IsAutoDetectMode) {
                    g_RobloxPath = g_AutoDetectedPath;
                }
                g_HasAutoDetected = true;
            }).detach();

            return 0;
        }

        case WM_BG_READY: {
            if (!pRT) return 0;

            if (!pWhite) {
                pRT->SetTextAntialiasMode(
                    D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
                pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

                pRT->CreateSolidColorBrush(
                    D2D1::ColorF(1, 1, 1), &pWhite);

                pRT->CreateSolidColorBrush(
                    D2D1::ColorF(1, 0.55f, 0.55f), &pHover);

                pRT->CreateSolidColorBrush(
                    D2D1::ColorF(0.035f, 0.035f, 0.043f, 0.40f),
                    &pUIRect);

                pRT->CreateSolidColorBrush(
                    D2D1::ColorF(
                        0x3F / 255.f,
                        0x3F / 255.f,
                        0x47 / 255.f,
                        1.f),
                    &pUIOutline);

                pRT->CreateSolidColorBrush(
                    D2D1::ColorF(9 / 255.f, 9 / 255.f, 11 / 255.f, 1.f),
                    &pCenterFillBrush);

                pRT->CreateSolidColorBrush(
                    D2D1::ColorF(
                        0x0D / 255.f,
                        0x0D / 255.f,
                        0x10 / 255.f,
                        0.5f),
                    &pTransparentFillBrush);

                pRT->CreateSolidColorBrush(
                    D2D1::ColorF(1, 1, 1), &pTextBrush);

                // Create gray brush for input user text
                pRT->CreateSolidColorBrush(
                    D2D1::ColorF(0.6f, 0.6f, 0.6f, 1.0f), &pGrayTextBrush);

                // Create tooltip brush (dark background like in screenshot)
                pRT->CreateSolidColorBrush(
                    D2D1::ColorF(0x0C / 255.0f, 0x0D / 255.0f, 0x12 / 255.0f, 1.0f), &pTooltipBrush);

                // Create titlebar gradient brush
                CreateLinearGradientBrushForTitlebar(pRT, (float)WINDOW_WIDTH, 350.0f, &pTitlebarGradientBrush);

                // Persistent brushes for Astral-like window controls/border
                pRT->CreateSolidColorBrush(D2D1::ColorF(0x10 / 255.f, 0x10 / 255.f, 0x13 / 255.f, 1.0f), &pWindowBorderBrush);
                pRT->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.08f, 0.08f, 1.0f), &pButtonBgBrush);
                if (pButtonBgBrush) pButtonBgBrush->SetOpacity(0.15f);
                pRT->CreateSolidColorBrush(D2D1::ColorF(0x10 / 255.f, 0x10 / 255.f, 0x13 / 255.f, 1.0f), &pButtonOutlineBrush);
                pRT->CreateSolidColorBrush(D2D1::ColorF(180.0f/255.0f, 180.0f/255.0f, 185.0f/255.0f, 1.0f), &pIconBrush);
                pRT->CreateSolidColorBrush(D2D1::ColorF(220.0f/255.0f, 220.0f/255.0f, 225.0f/255.0f, 1.0f), &pIconHoverBrush);
                pRT->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.25f, 0.25f, 1.0f), &pIconRedBrush);
                pRT->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0.7f, 0.7f, 1.0f), &pSubtitleBrush);

                // Create DirectWrite factory
                DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(&pWriteFactory));

                // Create text format for "input user" - normal weight, gray, smaller
                if (pWriteFactory) {
                    // "PCM" top-left text format (cached; avoids per-frame allocations)
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_BOLD,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        28.0f,
                        L"en-us",
                        &pPCMTextFormat);
                    if (pPCMTextFormat) {
                        pPCMTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                        pPCMTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
                        pPCMTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Create text format for "input user" - normal weight, gray, smaller
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_NORMAL,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        14.0f,
                        L"en-us",
                        &pInputUserTextFormat);
                    
                    if (pInputUserTextFormat) {
                        pInputUserTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                        pInputUserTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                        pInputUserTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Create text format for top rectangle input
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_NORMAL,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        16.0f,
                        L"en-us",
                        &pTopRectInputTextFormat);

                    if (pTopRectInputTextFormat) {
                        pTopRectInputTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                        pTopRectInputTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                        pTopRectInputTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Create bold text format for Website/Discord (16.0f)
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_BOLD,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        16.0f,
                        L"en-us",
                        &pBoldTextFormat);

                    if (pBoldTextFormat) {
                        pBoldTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                        pBoldTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                        pBoldTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Create left-aligned bold text format for labels (16.0f)
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_BOLD,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        16.0f,
                        L"en-us",
                        &pBoldTextFormatLeft);

                    if (pBoldTextFormatLeft) {
                        pBoldTextFormatLeft->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                        pBoldTextFormatLeft->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                        pBoldTextFormatLeft->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Create smaller bold text format for toggle buttons (12.0f)
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_BOLD,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        12.0f,
                        L"en-us",
                        &pToggleTextFormat);

                    if (pToggleTextFormat) {
                        pToggleTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                        pToggleTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                        pToggleTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Create larger bold text format for Click to login/Validate key (18.0f)
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_BOLD,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        17.0f,
                        L"en-us",
                        &pBoldTextFormatLarge);

                    if (pBoldTextFormatLarge) {
                        pBoldTextFormatLarge->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                        pBoldTextFormatLarge->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                        pBoldTextFormatLarge->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Create extra large bold text format for Welcome text (28.0f)
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_BOLD,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        28.0f,
                        L"en-us",
                        &pWelcomeTextFormat);

                    if (pWelcomeTextFormat) {
                        pWelcomeTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                        pWelcomeTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                        pWelcomeTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Create subtitle text format (14.0f, normal weight)
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_NORMAL,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        14.0f,
                        L"en-us",
                        &pSubtitleTextFormat);

                    if (pSubtitleTextFormat) {
                        pSubtitleTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                        pSubtitleTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                        pSubtitleTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Create tooltip text format (12.0f, normal weight)
                    pWriteFactory->CreateTextFormat(
                        L"Segoe UI",
                        nullptr,
                        DWRITE_FONT_WEIGHT_NORMAL,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        12.0f,
                        L"en-us",
                        &pTooltipTextFormat);

                    if (pTooltipTextFormat) {
                        pTooltipTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                        pTooltipTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                        pTooltipTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }

                    // Load custom font from memory (Panton Black Caps)
                    DWORD numFonts = 0;
                    HANDLE fontHandle = AddFontMemResourceEx(
                        (void*)panton_black_caps,
                        sizeof(panton_black_caps),
                        nullptr,
                        &numFonts
                    );

                    // Create text format for "LAUNCH MENU" using Panton Black Caps
                    if (fontHandle) {
                        pWriteFactory->CreateTextFormat(
                            L"Panton-BlackCaps",
                            nullptr,
                            DWRITE_FONT_WEIGHT_BLACK,
                            DWRITE_FONT_STYLE_NORMAL,
                            DWRITE_FONT_STRETCH_NORMAL,
                            25.0f,
                            L"en-us",
                            &pLaunchMenuTextFormat);

                        if (pLaunchMenuTextFormat) {
                            pLaunchMenuTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                            pLaunchMenuTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                            pLaunchMenuTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                        }
                    }
                }
            }

            if (g_pixels) {
                pRT->CreateBitmap(
                    D2D1::SizeU(g_w, g_h),
                    g_pixels,
                    g_stride,
                    D2D1::BitmapProperties(
                        D2D1::PixelFormat(
                            DXGI_FORMAT_B8G8R8A8_UNORM,
                            D2D1_ALPHA_MODE_PREMULTIPLIED)),
                    &pBG);

                CoTaskMemFree(g_pixels);
                g_pixels = nullptr;
            }


            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_MOUSEMOVE: {
            POINT pt{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };

            bool oldX = g_HoverX;
            bool oldM = g_HoverMin;
            bool oldV = g_HoverValidate;
            bool oldTopRect = g_HoverTopRect;
            bool oldSecondRect = g_HoverSecondRect;
            bool oldLeftSplit = g_HoverLeftSplit;
            bool oldRightSplit = g_HoverRightSplit;
            bool oldLaunchRect = g_HoverLaunchRect;
            bool oldProfile = g_HoverProfile;

            g_HoverX = PtInR(g_XRect, pt);
            g_HoverMin = PtInR(g_MinRect, pt);
            g_HoverProfile = PtInR(g_ProfileRect, pt);

            // Check modal hover states if modal is open
            if (g_ShowLogoutModal) {
                g_HoverLogoutCancel = PtInR(g_LogoutCancelButtonRect, pt);
                g_HoverLogoutConfirm = PtInR(g_LogoutConfirmButtonRect, pt);
            } else {
                g_HoverLogoutCancel = false;
                g_HoverLogoutConfirm = false;
            }

            // Only check page 1 elements when on page 1
            if (g_CurrentPage == 1) {
                g_HoverValidate = PtInR(g_ValidateRect, pt);
                g_HoverTopRect = (!g_IsLoggedIn) && PtInR(g_TopRect, pt);
                g_HoverSecondRect = PtInR(g_SecondRect, pt);
                g_HoverLeftSplit = PtInR(g_LeftSplitRect, pt);
                g_HoverRightSplit = PtInR(g_RightSplitRect, pt);
                g_HoverLaunchRect = false;
            } else {
                // Reset page 1 hover states when not on page 1
                g_HoverValidate = false;
                g_HoverTopRect = false;
                g_HoverSecondRect = false;
                g_HoverLeftSplit = false;
                g_HoverRightSplit = false;
                // Check page 2 hover states
                bool isLaunchDisabled = g_IsAutoDetectMode && (g_RobloxPath.empty() || g_RobloxPath == L"Roblox not found" || !g_HasAutoDetected);
                g_HoverLaunchRect = !isLaunchDisabled && PtInR(g_LaunchRect, pt);
                g_HoverAutoDetect = PtInR(g_AutoDetectButtonRect, pt);
                g_HoverManual = PtInR(g_ManualButtonRect, pt);
                g_HoverBrowse = !g_IsAutoDetectMode && PtInR(g_BrowseButtonRect, pt);
                g_HoverCircleCheckbox = PtInR(g_CircleCheckboxRect, pt);
            }

            bool overInput = (g_CurrentPage == 1) && PtInR(g_InputKeyRect, pt);

            float fadeSpeed = 0.3f;
            if (g_HoverTopRect) {
                if (g_HoverTopRectFade < 1.0f) {
                    g_HoverTopRectFade += fadeSpeed;
                    if (g_HoverTopRectFade > 1.0f) g_HoverTopRectFade = 1.0f;
                }
            } else {
                if (g_HoverTopRectFade > 0.0f) {
                    g_HoverTopRectFade -= fadeSpeed;
                    if (g_HoverTopRectFade < 0.0f) g_HoverTopRectFade = 0.0f;
                }
            }
            if (g_HoverSecondRect) {
                if (g_HoverSecondRectFade < 1.0f) {
                    g_HoverSecondRectFade += fadeSpeed;
                    if (g_HoverSecondRectFade > 1.0f) g_HoverSecondRectFade = 1.0f;
                }
            } else {
                if (g_HoverSecondRectFade > 0.0f) {
                    g_HoverSecondRectFade -= fadeSpeed;
                    if (g_HoverSecondRectFade < 0.0f) g_HoverSecondRectFade = 0.0f;
                }
            }
            if (g_HoverLeftSplit) {
                if (g_HoverLeftSplitFade < 1.0f) {
                    g_HoverLeftSplitFade += fadeSpeed;
                    if (g_HoverLeftSplitFade > 1.0f) g_HoverLeftSplitFade = 1.0f;
                }
            } else {
                if (g_HoverLeftSplitFade > 0.0f) {
                    g_HoverLeftSplitFade -= fadeSpeed;
                    if (g_HoverLeftSplitFade < 0.0f) g_HoverLeftSplitFade = 0.0f;
                }
            }
            if (g_HoverRightSplit) {
                if (g_HoverRightSplitFade < 1.0f) {
                    g_HoverRightSplitFade += fadeSpeed;
                    if (g_HoverRightSplitFade > 1.0f) g_HoverRightSplitFade = 1.0f;
                }
            } else {
                if (g_HoverRightSplitFade > 0.0f) {
                    g_HoverRightSplitFade -= fadeSpeed;
                    if (g_HoverRightSplitFade < 0.0f) g_HoverRightSplitFade = 0.0f;
                }
            }

            if (g_HoverLaunchRect) {
                if (g_HoverLaunchRectFade < 1.0f) {
                    g_HoverLaunchRectFade += fadeSpeed;
                    if (g_HoverLaunchRectFade > 1.0f) g_HoverLaunchRectFade = 1.0f;
                }
            } else {
                if (g_HoverLaunchRectFade > 0.0f) {
                    g_HoverLaunchRectFade -= fadeSpeed;
                    if (g_HoverLaunchRectFade < 0.0f) g_HoverLaunchRectFade = 0.0f;
                }
            }

            // Settings buttons fade
            if (g_HoverAutoDetect) {
                if (g_HoverAutoDetectFade < 1.0f) {
                    g_HoverAutoDetectFade += fadeSpeed;
                    if (g_HoverAutoDetectFade > 1.0f) g_HoverAutoDetectFade = 1.0f;
                }
            } else {
                if (g_HoverAutoDetectFade > 0.0f) {
                    g_HoverAutoDetectFade -= fadeSpeed;
                    if (g_HoverAutoDetectFade < 0.0f) g_HoverAutoDetectFade = 0.0f;
                }
            }

            if (g_HoverManual) {
                if (g_HoverManualFade < 1.0f) {
                    g_HoverManualFade += fadeSpeed;
                    if (g_HoverManualFade > 1.0f) g_HoverManualFade = 1.0f;
                }
            } else {
                if (g_HoverManualFade > 0.0f) {
                    g_HoverManualFade -= fadeSpeed;
                    if (g_HoverManualFade < 0.0f) g_HoverManualFade = 0.0f;
                }
            }

            if (g_HoverBrowse) {
                if (g_HoverBrowseFade < 1.0f) {
                    g_HoverBrowseFade += fadeSpeed;
                    if (g_HoverBrowseFade > 1.0f) g_HoverBrowseFade = 1.0f;
                }
            } else {
                if (g_HoverBrowseFade > 0.0f) {
                    g_HoverBrowseFade -= fadeSpeed;
                    if (g_HoverBrowseFade < 0.0f) g_HoverBrowseFade = 0.0f;
                }
            }

            if (g_HoverCircleCheckbox) {
                if (g_HoverCircleCheckboxFade < 1.0f) {
                    g_HoverCircleCheckboxFade += fadeSpeed;
                    if (g_HoverCircleCheckboxFade > 1.0f) g_HoverCircleCheckboxFade = 1.0f;
                }
            } else {
                if (g_HoverCircleCheckboxFade > 0.0f) {
                    g_HoverCircleCheckboxFade -= fadeSpeed;
                    if (g_HoverCircleCheckboxFade < 0.0f) g_HoverCircleCheckboxFade = 0.0f;
                }
            }

            if (overInput)
                SetCursor(LoadCursor(nullptr, IDC_IBEAM));
            else if (g_HoverSecondRect && !g_IsLoggedIn)
                SetCursor(LoadCursor(nullptr, IDC_NO));
            else if (g_CurrentPage == 2 && g_IsAutoDetectMode && (g_RobloxPath.empty() || g_RobloxPath == L"Roblox not found" || !g_HasAutoDetected) && PtInR(g_LaunchRect, pt))
                SetCursor(LoadCursor(nullptr, IDC_NO));
            else if (g_HoverX || g_HoverMin || g_HoverValidate ||
                     (!g_IsLoggedIn && g_HoverTopRect) || g_HoverSecondRect || g_HoverLeftSplit || g_HoverRightSplit ||
                     g_HoverLaunchRect || g_HoverProfile || g_HoverAutoDetect || g_HoverManual || g_HoverBrowse || g_HoverCircleCheckbox ||
                     g_HoverLogoutCancel || g_HoverLogoutConfirm)
                SetCursor(LoadCursor(nullptr, IDC_HAND));
            else
                SetCursor(LoadCursor(nullptr, IDC_ARROW));

            bool oldAutoDetect = g_HoverAutoDetect;
            bool oldManual = g_HoverManual;
            bool oldBrowse = g_HoverBrowse;

            if (oldX != g_HoverX || oldM != g_HoverMin ||
                oldV != g_HoverValidate ||
                oldTopRect != g_HoverTopRect || oldSecondRect != g_HoverSecondRect ||
                oldLeftSplit != g_HoverLeftSplit || oldRightSplit != g_HoverRightSplit ||
                oldLaunchRect != g_HoverLaunchRect || oldProfile != g_HoverProfile ||
                oldAutoDetect != g_HoverAutoDetect || oldManual != g_HoverManual || oldBrowse != g_HoverBrowse) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };

            if (pt.y <= 40)
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);

            // Handle top rectangle input focus (only on page 1)
            if (g_CurrentPage == 1 && PtInR(g_TopRectInputRect, pt)) {
                if (!g_TopRectFocused) {
                    g_TopRectFocused = true;
                    g_CursorVisible = true;
                    g_CursorBlinkTimer = 0;
                    SetTimer(hwnd, 1, 16, nullptr);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            } else {
                if (g_TopRectFocused) {
                    g_TopRectFocused = false;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }

            // Handle "Click to login" button click - open login page with intent=app (only if not logged in, only on page 1)
            if (g_CurrentPage == 1 && !g_IsLoggedIn && PtInR(g_TopRect, pt) && !PtInR(g_TopRectInputRect, pt)) {
                ShellExecuteW(nullptr, L"open", L"https://pistachiocreammenu.com/api/login?intent=app", nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            }

            // Handle X icon click - close application
            if (PtInR(g_XRect, pt)) {
                DestroyWindow(hwnd);
                return 0;
            }

            // Handle Minus icon click - minimize window
            if (PtInR(g_MinRect, pt)) {
                SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                return 0;
            }

            // Square icon does nothing

            // Handle "Next" button click - go to page 2 (enabled only if logged in, only on page 1)
            if (g_CurrentPage == 1 && PtInR(g_SecondRect, pt) && g_IsLoggedIn) {
                g_CurrentPage = 2;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            // Handle Website button click (only on page 1)
            if (g_CurrentPage == 1 && PtInR(g_LeftSplitRect, pt)) {
                ShellExecuteW(nullptr, L"open", L"https://pistachiocreammenu.com/", nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            }

            // Handle Discord button click (only on page 1)
            if (g_CurrentPage == 1 && PtInR(g_RightSplitRect, pt)) {
                ShellExecuteW(nullptr, L"open", L"https://discord.gg/yRS3rRRdxG", nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            }

            // Handle Launch/Close Menu button click (only on page 2)
            if (g_CurrentPage == 2 && PtInR(g_LaunchRect, pt)) {
                // Check if button is disabled (auto-detect mode but path not found yet)
                bool isLaunchDisabled = g_IsAutoDetectMode && (g_RobloxPath.empty() || g_RobloxPath == L"Roblox not found" || !g_HasAutoDetected);
                if (isLaunchDisabled) {
                    return 0;
                }
                
                // Check if instances are running
                if (g_RobloxInstanceCount.load() > 0) {
                    // Close all Roblox and PCM instances
                    KillRobloxAndPCMProcesses();
                } else {
                    // Launch menu
                    if (!g_RobloxPath.empty() && g_RobloxPath != L"Roblox not found") {
                        // Launch RobloxPlayerBeta.exe only if circle checkbox is checked
                        if (g_CircleCheckboxChecked) {
                            ShellExecuteW(nullptr, L"open", g_RobloxPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                        }

                        // Also try to launch PCM.exe from current directory if it exists
                        wchar_t currentDir[MAX_PATH];
                        GetCurrentDirectoryW(MAX_PATH, currentDir);
                        std::wstring pcmPath = std::wstring(currentDir) + L"\\PCM.exe";

                        // Check if PCM.exe exists before trying to launch
                        if (fs::exists(pcmPath)) {
                            ShellExecuteW(nullptr, L"open", pcmPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                        }
                    }
                }
                return 0;
            }

            // Handle Auto-detect button click (only on page 2)
            if (g_CurrentPage == 2 && PtInR(g_AutoDetectButtonRect, pt)) {
                if (!g_IsAutoDetectMode) {
                    // Save current manual path before switching
                    g_ManualPath = g_RobloxPath;
                }
                g_IsAutoDetectMode = true;
                // Restore auto-detected path from cache
                g_RobloxPath = g_AutoDetectedPath;

                // Save mode to registry
                HKEY hKey;
                if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, NULL,
                                   REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                    DWORD modeValue = 1; // 1 = auto-detect
                    RegSetValueExW(hKey, L"IsAutoDetectMode", 0, REG_DWORD,
                                  (BYTE*)&modeValue, sizeof(DWORD));
                    RegCloseKey(hKey);
                }

                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            // Handle Manual button click (only on page 2)
            if (g_CurrentPage == 2 && PtInR(g_ManualButtonRect, pt)) {
                if (g_IsAutoDetectMode) {
                    // Switching from auto to manual - restore previous manual path
                    g_IsAutoDetectMode = false;
                    if (!g_ManualPath.empty()) {
                        g_RobloxPath = g_ManualPath;
                    }

                    // Save mode to registry
                    HKEY hKey;
                    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, NULL,
                                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                        DWORD modeValue = 0; // 0 = manual
                        RegSetValueExW(hKey, L"IsAutoDetectMode", 0, REG_DWORD,
                                      (BYTE*)&modeValue, sizeof(DWORD));
                        RegCloseKey(hKey);
                    }
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            // Handle circle checkbox click (only on page 2)
            if (g_CurrentPage == 2 && PtInR(g_CircleCheckboxRect, pt)) {
                g_CircleCheckboxChecked = !g_CircleCheckboxChecked;

                // Save checkbox state to registry
                HKEY hKey;
                if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, NULL,
                                   REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                    DWORD checkboxValue = g_CircleCheckboxChecked ? 1 : 0;
                    RegSetValueExW(hKey, L"LaunchWithRoblox", 0, REG_DWORD,
                                  (BYTE*)&checkboxValue, sizeof(DWORD));
                    RegCloseKey(hKey);
                }

                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            // Handle Browse button click (only on page 2, only when in manual mode)
            if (g_CurrentPage == 2 && !g_IsAutoDetectMode && PtInR(g_BrowseButtonRect, pt)) {
                OPENFILENAMEW ofn = {};
                wchar_t fileName[MAX_PATH] = L"";
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"Roblox Player (RobloxPlayerBeta.exe)\0RobloxPlayerBeta.exe\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = fileName;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                ofn.lpstrTitle = L"Select RobloxPlayerBeta.exe";

                if (GetOpenFileNameW(&ofn)) {
                    g_RobloxPath = fileName;
                    g_ManualPath = fileName;  // Save to manual path cache

                    // Save manual path to registry
                    HKEY hKey;
                    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, NULL,
                                       REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                        RegSetValueExW(hKey, L"ManualPath", 0, REG_SZ,
                                      (BYTE*)g_ManualPath.c_str(),
                                      (DWORD)((g_ManualPath.length() + 1) * sizeof(wchar_t)));
                        RegCloseKey(hKey);
                    }

                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }

            // Handle logout modal button clicks
            if (g_ShowLogoutModal) {
                // Cancel button - close modal
                if (PtInR(g_LogoutCancelButtonRect, pt)) {
                    g_ShowLogoutModal = false;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }

                // Logout button - delete token, reset state, go to page 1
                if (PtInR(g_LogoutConfirmButtonRect, pt)) {
                    // Delete access token from registry
                    HKEY hKey;
                    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                        RegDeleteValueW(hKey, L"AccessToken");
                        RegCloseKey(hKey);
                    }

                    // Clear session data
                    g_AccessToken.clear();
                    g_DiscordUsername.clear();
                    g_IsLoggedIn = false;
                    g_ShowLogoutModal = false;
                    g_CurrentPage = 1;

                    StopTokenVerification();

                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }

                // If modal is open, prevent clicks outside modal
                return 0;
            }

            // Handle profile icon click - show logout modal (only when logged in)
            if (g_IsLoggedIn && PtInR(g_ProfileRect, pt)) {
                g_ShowLogoutModal = true;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            return 0;
        }

case WM_PAINT: {
    // Layered window rendering happens in the render thread (UpdateLayeredWindow).
    ValidateRect(hwnd, nullptr);
    return 0;
}

        case WM_CHAR: {
            if (g_TopRectFocused) {
                // Get rectangle dimensions for max text calculation
                float rectWidth = 288.0f;
                float rectX = (WINDOW_WIDTH - rectWidth) / 2.0f;
                
                if (w == 8) { // Backspace
                    if (g_TopRectCursorPosition > 0) {
                        g_TopRectText.erase(g_TopRectCursorPosition - 1, 1);
                        g_TopRectCursorPosition--;
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                } else if (w == 27) { // Escape key
                    g_TopRectText.clear();
                    g_TopRectCursorPosition = 0;
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (w == 13) { // Enter key
                    g_TopRectFocused = false;
                    InvalidateRect(hwnd, nullptr, FALSE);
                } else if (w >= 32 && w <= 126) { // Printable characters
                    // Check max text capacity - calculate if text would fit
                    std::wstring testText = g_TopRectText;
                    testText.insert(g_TopRectCursorPosition, 1, (wchar_t)w);
                    
                    if (pTopRectInputTextFormat && pWriteFactory) {
                        IDWriteTextLayout* testLayout = nullptr;
                        pWriteFactory->CreateTextLayout(testText.c_str(), (UINT32)testText.length(), pTopRectInputTextFormat, 1000.0f, 1000.0f, &testLayout);
                        if (testLayout) {
                            DWRITE_TEXT_METRICS metrics;
                            testLayout->GetMetrics(&metrics);
                            float textStartX = rectX + 16.0f + 20.0f + 10.0f;
                            float maxTextWidth = (rectX + rectWidth - 10.0f) - textStartX;
                            
                            if (metrics.width <= maxTextWidth) {
                                g_TopRectText.insert(g_TopRectCursorPosition, 1, (wchar_t)w);
                                g_TopRectCursorPosition++;
                            }
                            testLayout->Release();
                        }
                    }
                    
                    // Ensure timer is running for cursor blinking
                    SetTimer(hwnd, 1, 16, nullptr);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        }

        case WM_TIMER: {
            if (w == 1) {  // Fixed: using w instead of wParam
                // Cursor blinking for top rectangle
                if (g_TopRectFocused) {
                    g_CursorBlinkTimer++;
                    if (g_CursorBlinkTimer >= 20) { // Blink every 20 frames (333ms at 60fps)
                        g_CursorVisible = !g_CursorVisible;
                        g_CursorBlinkTimer = 0;
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                } else {
                    // Reset cursor when not focused
                    g_CursorVisible = true;
                    g_CursorBlinkTimer = 0;
                }
            }
            return 0;
        }

        case WM_DESTROY: {
            g_Closing = true;
            KillTimer(hwnd, 1);

            StopTokenVerification();

            if (g_bgThread) {
                WaitForSingleObject(g_bgThread, 2000);
                CloseHandle(g_bgThread);
                g_bgThread = nullptr;
            }

            // Stop and clean up instance counter thread
            if (g_InstanceCounterThread) {
                g_InstanceCounterRunning = false;
                WaitForSingleObject(g_InstanceCounterThread, 3000);
                CloseHandle(g_InstanceCounterThread);
                g_InstanceCounterThread = nullptr;
            }

            InterlockedExchange(&g_shouldRender, 0);
            PostQuitMessage(0);
            return 0;
        }

        case WM_SYSCOMMAND: {
            if (w == SC_MINIMIZE || w == SC_RESTORE) {
                return DefWindowProc(hwnd, msg, w, l);
            }
            if (w == SC_SIZE) {
                return 0;
            }
            return DefWindowProc(hwnd, msg, w, l);
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)l;
            mmi->ptMinTrackSize.x = WINDOW_WIDTH;
            mmi->ptMinTrackSize.y = WINDOW_HEIGHT;
            mmi->ptMaxTrackSize.x = WINDOW_WIDTH;
            mmi->ptMaxTrackSize.y = WINDOW_HEIGHT;
            return 0;
        }

        case WM_NCCALCSIZE: {
            if (w == TRUE) {
                RECT* rect = (RECT*)l;
                RECT client = *rect;
                DefWindowProc(hwnd, msg, w, l);
                *rect = client;
                return 0;
            }
    return DefWindowProc(hwnd, msg, w, l);
        }

        case WM_NCACTIVATE: {
            return DefWindowProc(hwnd, msg, w, -1);
        }
    }

    return DefWindowProc(hwnd, msg, w, l);
}

void RegisterProtocolHandler() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    HKEY hKey;
    std::wstring protocolKey = L"Software\\Classes\\pistachiocreammenu";
    
    if (RegCreateKeyExW(HKEY_CURRENT_USER, protocolKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        std::wstring urlDesc = L"URL:pistachiocreammenu Protocol";
        RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)urlDesc.c_str(), (DWORD)((urlDesc.length() + 1) * sizeof(wchar_t)));
        
        RegSetValueExW(hKey, L"URL Protocol", 0, REG_SZ, (BYTE*)L"", sizeof(wchar_t));
        
        HKEY shellKey;
        if (RegCreateKeyExW(hKey, L"shell", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &shellKey, NULL) == ERROR_SUCCESS) {
            HKEY openKey;
            if (RegCreateKeyExW(shellKey, L"open", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &openKey, NULL) == ERROR_SUCCESS) {
                HKEY commandKey;
                if (RegCreateKeyExW(openKey, L"command", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &commandKey, NULL) == ERROR_SUCCESS) {
                    std::wstring command = std::wstring(exePath) + L" \"%1\"";
                    RegSetValueExW(commandKey, NULL, 0, REG_SZ, (BYTE*)command.c_str(), (DWORD)((command.length() + 1) * sizeof(wchar_t)));
                    RegCloseKey(commandKey);
                }
                RegCloseKey(openKey);
            }
            RegCloseKey(shellKey);
        }
        RegCloseKey(hKey);
    }
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

static std::wstring ParseAccessTokenFromProtocolURL(const std::wstring& url) {
    if (url.rfind(L"pistachiocreammenu://", 0) != 0) return L"";
    size_t p = url.find(L"access_token=");
    if (p == std::wstring::npos) return L"";
    size_t start = p + wcslen(L"access_token=");
    size_t end = url.find_first_of(L"&?#/", start);
    if (end == std::wstring::npos) end = url.size();
    return url.substr(start, end - start);
}

static bool CheckTokenInDb(const std::wstring& accessToken) {
    std::wstring query = L"/api/db?access_token=" + accessToken;
    HINTERNET hSession = WinHttpOpen(L"PCM_LAUNCHER/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    HINTERNET hConnect = WinHttpConnect(hSession, L"pistachiocreammenu.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", query.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!ok) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    ok = WinHttpReceiveResponse(hRequest, nullptr);
    if (!ok) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
    if (status != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    char buf[16];
    memset(buf, 0, sizeof(buf));
    DWORD avail = 0;
    WinHttpQueryDataAvailable(hRequest, &avail);
    if (avail > sizeof(buf) - 1) avail = sizeof(buf) - 1;
    DWORD read = 0;
    if (avail > 0) WinHttpReadData(hRequest, buf, avail, &read);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return read >= 4 && memcmp(buf, "true", 4) == 0;
}

static std::wstring GetDiscordUsername(const std::wstring& accessToken) {
    HINTERNET hSession = WinHttpOpen(L"PCM_LAUNCHER/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return L"";
    HINTERNET hConnect = WinHttpConnect(hSession, L"discord.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return L"";
    }
    std::wstring authHeader = L"Authorization: Bearer " + accessToken;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/api/users/@me", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    WinHttpAddRequestHeaders(hRequest, authHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    BOOL ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!ok) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    ok = WinHttpReceiveResponse(hRequest, nullptr);
    if (!ok) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
    if (status != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return L"";
    }
    char buf[4096];
    memset(buf, 0, sizeof(buf));
    DWORD avail = 0;
    WinHttpQueryDataAvailable(hRequest, &avail);
    if (avail > sizeof(buf) - 1) avail = sizeof(buf) - 1;
    DWORD read = 0;
    if (avail > 0) WinHttpReadData(hRequest, buf, avail, &read);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    if (read == 0) return L"";
    buf[read] = 0;
    std::string json(buf);
    size_t usernamePos = json.find("\"username\":\"");
    if (usernamePos == std::string::npos) return L"";
    usernamePos += 12;
    size_t usernameEnd = json.find("\"", usernamePos);
    if (usernameEnd == std::string::npos) return L"";
    std::string usernameStr = json.substr(usernamePos, usernameEnd - usernamePos);
    std::wstring username(usernameStr.begin(), usernameStr.end());
    return username;
}

struct AuthWork {
    HWND hwnd;
    wchar_t* token;
};

static DWORD WINAPI AuthThread(LPVOID param) {
    AuthWork* w = (AuthWork*)param;
    std::wstring token = w->token ? w->token : L"";
    bool ok = false;

    if (!token.empty()) {
        ok = CheckTokenInDb(token);
    }

    if (ok) {
        g_AccessToken = token;
        g_DiscordUsername = GetDiscordUsername(token);

        // Save valid token to registry
        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, NULL,
                           REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"AccessToken", 0, REG_SZ,
                          (BYTE*)token.c_str(),
                          (DWORD)((token.length() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
    } else {
        g_AccessToken.clear();
        g_DiscordUsername.clear();

        // Delete invalid token from registry
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValueW(hKey, L"AccessToken");
            RegCloseKey(hKey);
        }
    }

    PostMessageW(w->hwnd, WM_AUTH_RESULT, (WPARAM)(ok ? 1 : 0), 0);
    if (w->token) free(w->token);
    free(w);
    return 0;
}

static void StartAuthValidation(HWND hwnd, const std::wstring& token) {
    AuthWork* w = (AuthWork*)malloc(sizeof(AuthWork));
    if (!w) return;
    w->hwnd = hwnd;
    w->token = _wcsdup(token.c_str());
    CreateThread(nullptr, 0, AuthThread, w, 0, nullptr);
}

// Load saved access token from registry
static std::wstring LoadAccessToken() {
    HKEY hKey;
    std::wstring token = L"";

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buffer[1024];
        DWORD bufferSize = sizeof(buffer);
        DWORD type = REG_SZ;

        if (RegQueryValueExW(hKey, L"AccessToken", NULL, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            token = buffer;
        }

        RegCloseKey(hKey);
    }

    return token;
}

static DWORD WINAPI TokenVerificationThread(LPVOID param) {
    while (g_tokenVerificationRunning.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (!g_tokenVerificationRunning.load()) break;

        if (!g_IsLoggedIn) continue;

        std::wstring token = LoadAccessToken();
        bool isValid = !token.empty() && CheckTokenInDb(token);

        if (!isValid) {
            PostMessageW(g_mainHwnd, WM_TOKEN_INVALID, 0, 0);
            break;
        }
    }
    return 0;
}

static void StartTokenVerification() {
    if (g_tokenVerificationThread) return;
    if (!g_mainHwnd) return;
    g_tokenVerificationRunning = true;
    g_tokenVerificationThread = CreateThread(nullptr, 0, TokenVerificationThread, nullptr, 0, nullptr);
}

static void StopTokenVerification() {
    g_tokenVerificationRunning = false;
    if (g_tokenVerificationThread) {
        WaitForSingleObject(g_tokenVerificationThread, 2000);
        CloseHandle(g_tokenVerificationThread);
        g_tokenVerificationThread = nullptr;
    }
}

// ---------------- ENTRY ----------------
int WINAPI wWinMain(HINSTANCE h, HINSTANCE, PWSTR cmdLine, int cmd) {
    std::wstring token = L"";
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc > 1 && argv[1]) {
        token = ParseAccessTokenFromProtocolURL(argv[1]);
    }
    if (argv) LocalFree(argv);
    HANDLE single = CreateMutexW(nullptr, TRUE, L"PCM_LAUNCHER_SINGLE_INSTANCE");
    DWORD singleErr = GetLastError();
    if (singleErr == ERROR_ALREADY_EXISTS || singleErr == ERROR_ACCESS_DENIED) {
        if (!token.empty()) {
            HWND existing = FindWindowW(L"PCMWND", nullptr);
            if (existing) {
                COPYDATASTRUCT cds;
                cds.dwData = WM_TOKEN_COPYDATA;
                cds.cbData = (DWORD)((token.size() + 1) * sizeof(wchar_t));
                cds.lpData = (void*)token.c_str();
                SendMessageW(existing, WM_COPYDATA, 0, (LPARAM)&cds);
            }
        }
        return 0;
    }
    RegisterProtocolHandler();
    CoInitialize(NULL);
    D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);

    WNDCLASSW wc{};
    wc.hInstance = h;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = L"PCMWND";
    RegisterClassW(&wc);

    int px = (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2;
    int py = (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED,
        L"PCMWND",
        L"PCM",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        px, py,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        nullptr,
        nullptr,
        h,
        nullptr);

    // Astral-style layered rendering: memory DC + DIB + DCRenderTarget
    g_dc = CreateCompatibleDC(NULL);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WINDOW_WIDTH;
    bmi.bmiHeader.biHeight = -WINDOW_HEIGHT; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    g_bmp = CreateDIBSection(g_dc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    if (g_bmp) {
        SelectObject(g_dc, g_bmp);
    }

    D2D1_RENDER_TARGET_PROPERTIES props = {
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
        0, 0,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT
    };
    if (pFactory) {
        pFactory->CreateDCRenderTarget(&props, &pRT);
        if (pRT) {
            pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }
    }

    // Initialize brushes/text formats early (background may come later)
    if (pRT) {
        SendMessageW(hwnd, WM_BG_READY, 0, 0);
    }

    // Load saved manual path and mode from registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t pathBuffer[1024];
        DWORD pathBufferSize = sizeof(pathBuffer);
        DWORD type = REG_SZ;

        // Load manual path
        if (RegQueryValueExW(hKey, L"ManualPath", NULL, &type, (LPBYTE)pathBuffer, &pathBufferSize) == ERROR_SUCCESS) {
            g_ManualPath = pathBuffer;
        }

        // Load auto-detect mode setting
        DWORD modeBuffer = 1; // Default to auto-detect
        DWORD modeBufferSize = sizeof(modeBuffer);
        if (RegQueryValueExW(hKey, L"IsAutoDetectMode", NULL, &type, (LPBYTE)&modeBuffer, &modeBufferSize) == ERROR_SUCCESS) {
            g_IsAutoDetectMode = (modeBuffer != 0);
        }

        // Apply manual path to g_RobloxPath if in manual mode
        if (!g_IsAutoDetectMode && !g_ManualPath.empty()) {
            g_RobloxPath = g_ManualPath;
        }

        // Load circle checkbox state (launch with Roblox)
        DWORD checkboxBuffer = 1; // Default to checked
        DWORD checkboxBufferSize = sizeof(checkboxBuffer);
        if (RegQueryValueExW(hKey, L"LaunchWithRoblox", NULL, &type, (LPBYTE)&checkboxBuffer, &checkboxBufferSize) == ERROR_SUCCESS) {
            g_CircleCheckboxChecked = (checkboxBuffer != 0);
        }

        RegCloseKey(hKey);
    }

    // Check for saved token first (before showing UI)
    if (token.empty()) {
        std::wstring savedToken = LoadAccessToken();
        if (!savedToken.empty()) {
            token = savedToken;
        }
    }

    // Validate token synchronously before showing UI
    if (!token.empty()) {
        bool ok = CheckTokenInDb(token);
        if (ok) {
            g_AccessToken = token;
            g_DiscordUsername = GetDiscordUsername(token);
            g_IsLoggedIn = true;
            g_CurrentPage = 2;  // Go directly to page 2 if already logged in
        } else {
            // Clear invalid token
            g_AccessToken.clear();
            g_DiscordUsername.clear();
            g_IsLoggedIn = false;
            // Delete invalid token from registry
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PCM_LAUNCHER", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                RegDeleteValueW(hKey, L"AccessToken");
                RegCloseKey(hKey);
            }
        }
    }

    g_mainHwnd = hwnd;
    if (g_IsLoggedIn) {
        StartTokenVerification();
    }

    ShowWindow(hwnd, cmd);
    UpdateWindow(hwnd);
    SetWindowPos(hwnd, NULL, px, py, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    if (!g_renderThread) {
        g_renderThread = CreateThread(nullptr, 0, RenderThread, hwnd, 0, nullptr);
    }

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    InterlockedExchange(&g_shouldRender, 0);
    if (g_renderThread) {
        WaitForSingleObject(g_renderThread, 2000);
        CloseHandle(g_renderThread);
        g_renderThread = nullptr;
    }
    if (g_bmp) { DeleteObject(g_bmp); g_bmp = nullptr; }
    if (g_dc) { DeleteDC(g_dc); g_dc = nullptr; }

    if (pBG) pBG->Release();
    if (pWhite) pWhite->Release();
    if (pHover) pHover->Release();
    if (pUIRect) pUIRect->Release();
    if (pUIOutline) pUIOutline->Release();
    if (pCenterFillBrush) pCenterFillBrush->Release();
    if (pTransparentFillBrush) pTransparentFillBrush->Release();
    if (pTextBrush) pTextBrush->Release();
    if (pGrayTextBrush) pGrayTextBrush->Release();
    if (pTooltipBrush) pTooltipBrush->Release();
    if (pTitlebarGradientBrush) pTitlebarGradientBrush->Release();
    if (pWindowBorderBrush) pWindowBorderBrush->Release();
    if (pButtonBgBrush) pButtonBgBrush->Release();
    if (pButtonOutlineBrush) pButtonOutlineBrush->Release();
    if (pIconBrush) pIconBrush->Release();
    if (pIconHoverBrush) pIconHoverBrush->Release();
    if (pIconRedBrush) pIconRedBrush->Release();
    if (pSubtitleBrush) pSubtitleBrush->Release();
    if (pJoinServerTextFormat) pJoinServerTextFormat->Release();
    if (pInputUserTextFormat) pInputUserTextFormat->Release();
    if (pTopRectInputTextFormat) pTopRectInputTextFormat->Release();
    if (pBoldTextFormat) pBoldTextFormat->Release();
    if (pBoldTextFormatLeft) pBoldTextFormatLeft->Release();
    if (pToggleTextFormat) pToggleTextFormat->Release();
    if (pBoldTextFormatLarge) pBoldTextFormatLarge->Release();
    if (pWelcomeTextFormat) pWelcomeTextFormat->Release();
    if (pSubtitleTextFormat) pSubtitleTextFormat->Release();
    if (pTooltipTextFormat) pTooltipTextFormat->Release();
    if (pLaunchMenuTextFormat) pLaunchMenuTextFormat->Release();
    if (pPCMTextFormat) pPCMTextFormat->Release();
    if (pWriteFactory) pWriteFactory->Release();
    if (pRT) pRT->Release();
    if (pFactory) pFactory->Release();

    CoUninitialize();
    return 0;
}