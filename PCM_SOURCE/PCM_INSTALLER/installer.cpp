#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <d2d1.h>
#include <dwmapi.h>
#include <dwrite.h>
#include <winhttp.h>
#include <shlobj.h>
#include <filesystem>
#include <string>
#include <atomic>
#include <thread>
#include <fstream>
#include <comdef.h>
#include <shlwapi.h>
#include <algorithm>

namespace fs = std::filesystem;

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "winhttp")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "ole32")
#pragma comment(lib, "oleaut32")

ID2D1Factory* g_pD2DFactory = nullptr;
ID2D1HwndRenderTarget* g_pRenderTarget = nullptr;
ID2D1SolidColorBrush* g_pBackgroundBrush = nullptr;
ID2D1SolidColorBrush* g_pTopBarBrush = nullptr;
ID2D1SolidColorBrush* g_pButtonBrush = nullptr;
ID2D1SolidColorBrush* g_pButtonHoverBrush = nullptr;
ID2D1SolidColorBrush* g_pXIconBrush = nullptr;
ID2D1SolidColorBrush* g_pButton2Brush = nullptr;
ID2D1SolidColorBrush* g_pButton2OutlineBrush = nullptr;
ID2D1SolidColorBrush* g_pTextBrush = nullptr;
ID2D1SolidColorBrush* g_pWhiteTextBrush = nullptr;
ID2D1SolidColorBrush* g_pGrayTextBrush = nullptr;

IDWriteFactory* g_pDWriteFactory = nullptr;
IDWriteTextFormat* g_pTextFormat = nullptr;
IDWriteTextFormat* g_pWelcomeTextFormat = nullptr;
IDWriteTextFormat* g_pButtonTextFormat = nullptr;
IDWriteTextFormat* g_pHeaderTextFormat = nullptr;
IDWriteTextFormat* g_pBodyTextFormat = nullptr;

// Button state
bool g_ButtonHover = false;
bool g_Button3Hover = false;
bool g_Button4Hover = false;
bool g_Button5Hover = false;
const float BUTTON_WIDTH = 205.0f;
const float BUTTON_HEIGHT = 60.0f;
const float BUTTON_X = (600.0f - BUTTON_WIDTH) / 2.0f;
const float BUTTON_Y = 450.0f - 180.0f;

// Page 2 button (bottom right, smaller)
const float BUTTON2_WIDTH = 84.0f;
const float BUTTON2_HEIGHT = 41.0f;
const float BUTTON2_X = 600.0f - BUTTON2_WIDTH - 20.0f;
const float BUTTON2_Y = 450.0f - BUTTON2_HEIGHT - 20.0f;

// Page 2 second button (next to first, on the left)
const float BUTTON3_WIDTH = 84.0f;
const float BUTTON3_HEIGHT = 41.0f;
const float BUTTON3_X = BUTTON2_X - BUTTON3_WIDTH - 10.0f; // 10px gap from BUTTON2
const float BUTTON3_Y = 450.0f - BUTTON3_HEIGHT - 20.0f;

// Page 3 UPDATE button (centered)
const float BUTTON4_WIDTH = 120.0f;
const float BUTTON4_HEIGHT = 35.0f;
const float BUTTON4_X = (600.0f - BUTTON4_WIDTH) / 2.0f; // Centered
const float BUTTON4_Y = 310.0f;

// Page 3 BACK button (on the left side)
const float BUTTON5_WIDTH = 84.0f;
const float BUTTON5_HEIGHT = 41.0f;
const float BUTTON5_X = 20.0f;
const float BUTTON5_Y = 450.0f - BUTTON5_HEIGHT - 20.0f;

// X button state
bool g_XButtonHover = false;
const float X_BUTTON_SIZE = 20.0f;
const float X_BUTTON_X = 600.0f - X_BUTTON_SIZE - 5.0f;
const float X_BUTTON_Y = 0.0f;

// Page state
int g_CurrentPage = 1;

// Scroll state for terms box
float g_TermsScrollOffset = 0.0f;
bool g_ScrollbarDragging = false;

// Checkbox state
bool g_CheckboxChecked = false;
bool g_CheckboxHover = false;

// Installation state
std::atomic<float> g_InstallProgress(0.0f);
std::atomic<bool> g_IsInstalling(false);
std::atomic<bool> g_InstallComplete(false);
std::atomic<bool> g_IsUpdateMode(false);
HWND g_MainHwnd = nullptr;

void CleanupDeviceResources()
{
    if (g_pGrayTextBrush) {
        g_pGrayTextBrush->Release();
        g_pGrayTextBrush = nullptr;
    }
    if (g_pWhiteTextBrush) {
        g_pWhiteTextBrush->Release();
        g_pWhiteTextBrush = nullptr;
    }
    if (g_pTextBrush) {
        g_pTextBrush->Release();
        g_pTextBrush = nullptr;
    }
    if (g_pButton2OutlineBrush) {
        g_pButton2OutlineBrush->Release();
        g_pButton2OutlineBrush = nullptr;
    }
    if (g_pButton2Brush) {
        g_pButton2Brush->Release();
        g_pButton2Brush = nullptr;
    }
    if (g_pXIconBrush) {
        g_pXIconBrush->Release();
        g_pXIconBrush = nullptr;
    }
    if (g_pButtonHoverBrush) {
        g_pButtonHoverBrush->Release();
        g_pButtonHoverBrush = nullptr;
    }
    if (g_pButtonBrush) {
        g_pButtonBrush->Release();
        g_pButtonBrush = nullptr;
    }
    if (g_pTopBarBrush) {
        g_pTopBarBrush->Release();
        g_pTopBarBrush = nullptr;
    }
    if (g_pBackgroundBrush) {
        g_pBackgroundBrush->Release();
        g_pBackgroundBrush = nullptr;
    }
    if (g_pRenderTarget) {
        g_pRenderTarget->Release();
        g_pRenderTarget = nullptr;
    }
}

HRESULT CreateDeviceResources(HWND hwnd)
{
    HRESULT hr = S_OK;

    if (!g_pRenderTarget) {
        RECT rc;
        GetClientRect(hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        hr = g_pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &g_pRenderTarget
        );

        if (SUCCEEDED(hr)) {
            g_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0x0f0f10),
                &g_pBackgroundBrush
            );
        }

        if (SUCCEEDED(hr)) {
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0x1a1a1c),
                &g_pTopBarBrush
            );
        }

        if (SUCCEEDED(hr)) {
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0xFFFFFF),
                &g_pButtonBrush
            );
        }

        if (SUCCEEDED(hr)) {
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0xE8E8E8),
                &g_pButtonHoverBrush
            );
        }

        if (SUCCEEDED(hr)) {
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0xFFFFFF),
                &g_pXIconBrush
            );
        }

        if (SUCCEEDED(hr)) {
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0x1a1a1c), // Same color as top bar
                &g_pButton2Brush
            );
        }

        if (SUCCEEDED(hr)) {
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0x2a2a2c), // Lighter outline
                &g_pButton2OutlineBrush
            );
        }

        if (SUCCEEDED(hr)) {
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Black),
                &g_pTextBrush
            );
        }

        if (SUCCEEDED(hr)) {
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0xFFFFFF),
                &g_pWhiteTextBrush
            );
        }

        if (SUCCEEDED(hr)) {
            hr = g_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0xC8C8C8),
                &g_pGrayTextBrush
            );
        }
    }

    return hr;
}

void OnPaint(HWND hwnd)
{
    HRESULT hr = CreateDeviceResources(hwnd);

    if (SUCCEEDED(hr)) {
        g_pRenderTarget->BeginDraw();

        g_pRenderTarget->Clear(D2D1::ColorF(0x0f0f10));

        // Draw custom top bar
        D2D1_RECT_F topBar = D2D1::RectF(0, 0, 600, 20);
        g_pRenderTarget->FillRectangle(topBar, g_pTopBarBrush);

        // Draw button based on current page
        if (g_CurrentPage == 1) {
            // Draw welcome text at top
            if (g_pWelcomeTextFormat && g_pWhiteTextBrush) {
                D2D1_RECT_F welcomeRect = D2D1::RectF(0, 100, 600, 120);
                g_pRenderTarget->DrawText(
                    L"WELCOME TO PISTACIHO CREAM MENU",
                    32,
                    g_pWelcomeTextFormat,
                    welcomeRect,
                    g_pWhiteTextBrush
                );
            }

            // Page 1: centered button with rounded corners
            D2D1_ROUNDED_RECT roundedButton = D2D1::RoundedRect(
                D2D1::RectF(BUTTON_X, BUTTON_Y, BUTTON_X + BUTTON_WIDTH, BUTTON_Y + BUTTON_HEIGHT),
                3.0f, 3.0f
            );
            ID2D1SolidColorBrush* buttonBrush = g_ButtonHover ? g_pButtonHoverBrush : g_pButtonBrush;
            g_pRenderTarget->FillRoundedRectangle(roundedButton, buttonBrush);

            // Draw "INSTALL" text on button
            if (g_pTextFormat && g_pTextBrush) {
                D2D1_RECT_F buttonRect = D2D1::RectF(BUTTON_X, BUTTON_Y, BUTTON_X + BUTTON_WIDTH, BUTTON_Y + BUTTON_HEIGHT);
                g_pRenderTarget->DrawText(
                    L"INSTALL",
                    7,
                    g_pTextFormat,
                    buttonRect,
                    g_pTextBrush
                );
            }
        } else if (g_CurrentPage == 2) {
            // Draw "TERMS AND CONDITIONS" text at top
            if (g_pWelcomeTextFormat && g_pWhiteTextBrush) {
                D2D1_RECT_F termsRect = D2D1::RectF(0, 65, 600, 85);
                g_pRenderTarget->DrawText(
                    L"TERMS AND CONDITIONS",
                    21,
                    g_pWelcomeTextFormat,
                    termsRect,
                    g_pWhiteTextBrush
                );
            }

            // Draw terms box
            const float BOX_WIDTH = 560.0f;
            const float BOX_HEIGHT = 201.0f;
            const float BOX_X = (600.0f - BOX_WIDTH) / 2.0f;
            const float BOX_Y = 120.0f;

            // Fill the box with rounded corners
            D2D1_ROUNDED_RECT roundedBox = D2D1::RoundedRect(
                D2D1::RectF(BOX_X, BOX_Y, BOX_X + BOX_WIDTH, BOX_Y + BOX_HEIGHT),
                3.0f, 3.0f
            );
            g_pRenderTarget->FillRoundedRectangle(roundedBox, g_pButton2Brush);

            // Draw lighter outline with rounded corners
            D2D1_ROUNDED_RECT roundedBoxOutline = D2D1::RoundedRect(
                D2D1::RectF(BOX_X + 0.5f, BOX_Y + 0.5f, BOX_X + BOX_WIDTH - 0.5f, BOX_Y + BOX_HEIGHT - 0.5f),
                2.5f, 2.5f
            );
            g_pRenderTarget->DrawRoundedRectangle(roundedBoxOutline, g_pButton2OutlineBrush, 1.0f);

            // Enable clipping for the terms box content
            g_pRenderTarget->PushAxisAlignedClip(
                D2D1::RectF(BOX_X, BOX_Y, BOX_X + BOX_WIDTH, BOX_Y + BOX_HEIGHT),
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
            );

            // Draw header text inside the box (with scroll offset)
            if (g_pHeaderTextFormat && g_pWhiteTextBrush) {
                D2D1_RECT_F headerRect = D2D1::RectF(BOX_X + 18.0f, BOX_Y + 13.0f - g_TermsScrollOffset, BOX_X + BOX_WIDTH - 15.0f, BOX_Y + 40.0f - g_TermsScrollOffset);
                g_pRenderTarget->DrawText(
                    L"Liscense and Agreement",
                    22,
                    g_pHeaderTextFormat,
                    headerRect,
                    g_pWhiteTextBrush
                );
            }

            // Draw body text inside the box (with scroll offset)
            if (g_pBodyTextFormat && g_pGrayTextBrush) {
                D2D1_RECT_F bodyRect = D2D1::RectF(BOX_X + 18.0f, BOX_Y + 43.0f - g_TermsScrollOffset, BOX_X + BOX_WIDTH - 45.0f, BOX_Y + BOX_HEIGHT - 10.0f - g_TermsScrollOffset);
                g_pRenderTarget->DrawText(
                    L"By purchasing, downloading, or using this software, you agree to be bound by these Terms & Conditions. If you do not agree, do not use the Menu.",
                    148,
                    g_pBodyTextFormat,
                    bodyRect,
                    g_pGrayTextBrush
                );
            }

            // Draw second header text (with scroll offset)
            if (g_pHeaderTextFormat && g_pWhiteTextBrush) {
                D2D1_RECT_F header2Rect = D2D1::RectF(BOX_X + 18.0f, BOX_Y + 120.0f - g_TermsScrollOffset, BOX_X + BOX_WIDTH - 15.0f, BOX_Y + 147.0f - g_TermsScrollOffset);
                g_pRenderTarget->DrawText(
                    L"User Responsibility",
                    19,
                    g_pHeaderTextFormat,
                    header2Rect,
                    g_pWhiteTextBrush
                );
            }

            // Draw second body text (with scroll offset)
            if (g_pBodyTextFormat && g_pGrayTextBrush) {
                D2D1_RECT_F body2Rect = D2D1::RectF(BOX_X + 18.0f, BOX_Y + 150.0f - g_TermsScrollOffset, BOX_X + BOX_WIDTH - 45.0f, BOX_Y + BOX_HEIGHT + 150.0f - g_TermsScrollOffset);
                const wchar_t* text2 = L"The user acknowledges that they are solely responsible for how the Menu is used. Any consequences resulting from use of the Menu, including but not limited to account suspensions, bans, or other penalties, are entirely the responsibility of the user.";
                g_pRenderTarget->DrawText(
                    text2,
                    wcslen(text2),
                    g_pBodyTextFormat,
                    body2Rect,
                    g_pGrayTextBrush
                );
            }

            // Draw third header text (with scroll offset)
            if (g_pHeaderTextFormat && g_pWhiteTextBrush) {
                D2D1_RECT_F header3Rect = D2D1::RectF(BOX_X + 18.0f, BOX_Y + 250.0f - g_TermsScrollOffset, BOX_X + BOX_WIDTH - 15.0f, BOX_Y + 277.0f - g_TermsScrollOffset);
                g_pRenderTarget->DrawText(
                    L"Limitation of Liability",
                    23,
                    g_pHeaderTextFormat,
                    header3Rect,
                    g_pWhiteTextBrush
                );
            }

            // Draw third body text (with scroll offset)
            if (g_pBodyTextFormat && g_pGrayTextBrush) {
                D2D1_RECT_F body3Rect = D2D1::RectF(BOX_X + 18.0f, BOX_Y + 280.0f - g_TermsScrollOffset, BOX_X + BOX_WIDTH - 45.0f, BOX_Y + BOX_HEIGHT + 300.0f - g_TermsScrollOffset);
                const wchar_t* text3 = L"Under no circumstances shall the developers be liable for any direct, indirect, incidental, special, or consequential damages arising out of or in any way connected with the use of this product.";
                g_pRenderTarget->DrawText(
                    text3,
                    wcslen(text3),
                    g_pBodyTextFormat,
                    body3Rect,
                    g_pGrayTextBrush
                );
            }

            // Pop the clip
            g_pRenderTarget->PopAxisAlignedClip();

            // Draw scrollbar
            const float SCROLLBAR_WIDTH = 4.0f;
            const float SCROLLBAR_X = BOX_X + BOX_WIDTH - 10.0f;
            const float SCROLLBAR_Y = BOX_Y + 5.0f;
            const float SCROLLBAR_HEIGHT = BOX_HEIGHT - 10.0f;

            // Calculate scrollbar thumb size and position
            const float CONTENT_HEIGHT = 380.0f; // Approximate total content height
            const float VISIBLE_HEIGHT = BOX_HEIGHT;
            const float THUMB_RATIO = VISIBLE_HEIGHT / CONTENT_HEIGHT;
            const float THUMB_HEIGHT = SCROLLBAR_HEIGHT * THUMB_RATIO;
            const float MAX_SCROLL = 220.0f; // Should match maxScroll in WM_MOUSEWHEEL
            const float SCROLL_RATIO = g_TermsScrollOffset / MAX_SCROLL;
            const float THUMB_Y = SCROLLBAR_Y + (SCROLLBAR_HEIGHT - THUMB_HEIGHT) * SCROLL_RATIO;

            // Draw scrollbar thumb
            D2D1_ROUNDED_RECT scrollbarThumb = D2D1::RoundedRect(
                D2D1::RectF(SCROLLBAR_X, THUMB_Y, SCROLLBAR_X + SCROLLBAR_WIDTH, THUMB_Y + THUMB_HEIGHT),
                2.0f, 2.0f
            );
            g_pRenderTarget->FillRoundedRectangle(scrollbarThumb, g_pButton2OutlineBrush);

            // Draw checkbox below the main box on the left
            const float CHECKBOX_SIZE = 20.0f;
            const float CHECKBOX_X = BOX_X + 10.0f;
            const float CHECKBOX_Y = BOX_Y + BOX_HEIGHT + 30.0f;

            D2D1_ROUNDED_RECT checkbox = D2D1::RoundedRect(
                D2D1::RectF(CHECKBOX_X, CHECKBOX_Y, CHECKBOX_X + CHECKBOX_SIZE, CHECKBOX_Y + CHECKBOX_SIZE),
                3.0f, 3.0f
            );
            g_pRenderTarget->FillRoundedRectangle(checkbox, g_pButtonBrush);

            // Draw lighter outline for checkbox (white on hover, gray normally)
            D2D1_ROUNDED_RECT checkboxOutline = D2D1::RoundedRect(
                D2D1::RectF(CHECKBOX_X + 0.5f, CHECKBOX_Y + 0.5f, CHECKBOX_X + CHECKBOX_SIZE - 0.5f, CHECKBOX_Y + CHECKBOX_SIZE - 0.5f),
                2.5f, 2.5f
            );
            g_pRenderTarget->DrawRoundedRectangle(checkboxOutline, g_CheckboxHover ? g_pWhiteTextBrush : g_pButton2OutlineBrush, g_CheckboxHover ? 1.5f : 1.0f);

            // Draw checkmark if checked
            if (g_CheckboxChecked) {
                ID2D1PathGeometry* checkGeometry = nullptr;
                if (SUCCEEDED(g_pD2DFactory->CreatePathGeometry(&checkGeometry))) {
                    ID2D1GeometrySink* sink = nullptr;
                    if (SUCCEEDED(checkGeometry->Open(&sink))) {
                        float centerX = CHECKBOX_X + CHECKBOX_SIZE / 2.0f;
                        float centerY = CHECKBOX_Y + CHECKBOX_SIZE / 2.0f;
                        float size = CHECKBOX_SIZE * 0.5f;

                        // Draw checkmark path
                        sink->BeginFigure(D2D1::Point2F(centerX - size * 0.4f, centerY), D2D1_FIGURE_BEGIN_FILLED);
                        sink->AddLine(D2D1::Point2F(centerX - size * 0.1f, centerY + size * 0.4f));
                        sink->AddLine(D2D1::Point2F(centerX + size * 0.5f, centerY - size * 0.3f));
                        sink->EndFigure(D2D1_FIGURE_END_OPEN);

                        sink->Close();
                        sink->Release();

                        // Draw the checkmark with black color
                        g_pRenderTarget->DrawGeometry(checkGeometry, g_pTextBrush, 2.0f);
                        checkGeometry->Release();
                    }
                }
            }

            // Draw checkbox label text
            if (g_pBodyTextFormat && g_pGrayTextBrush) {
                D2D1_RECT_F checkboxLabelRect = D2D1::RectF(CHECKBOX_X + CHECKBOX_SIZE + 8.0f, CHECKBOX_Y - 2.0f, BOX_X + BOX_WIDTH, CHECKBOX_Y + CHECKBOX_SIZE + 2.0f);
                g_pRenderTarget->DrawText(
                    L"I agree to the terms and conditions",
                    36,
                    g_pBodyTextFormat,
                    checkboxLabelRect,
                    g_pGrayTextBrush
                );
            }

            // Page 2: bottom right smaller button with rounded corners (white if checkbox checked, gray if not)
            D2D1_ROUNDED_RECT roundedButton2 = D2D1::RoundedRect(
                D2D1::RectF(BUTTON2_X, BUTTON2_Y, BUTTON2_X + BUTTON2_WIDTH, BUTTON2_Y + BUTTON2_HEIGHT),
                3.0f, 3.0f
            );
            g_pRenderTarget->FillRoundedRectangle(roundedButton2, g_CheckboxChecked ? g_pButtonBrush : g_pButton2Brush);

            // Draw "Next" text on right button (black if enabled, gray if disabled)
            if (g_pButtonTextFormat) {
                D2D1_RECT_F button2Rect = D2D1::RectF(BUTTON2_X, BUTTON2_Y, BUTTON2_X + BUTTON2_WIDTH, BUTTON2_Y + BUTTON2_HEIGHT);
                g_pRenderTarget->DrawText(
                    L"NEXT",
                    4,
                    g_pButtonTextFormat,
                    button2Rect,
                    g_CheckboxChecked ? g_pTextBrush : g_pButton2OutlineBrush
                );
            }

            // Page 2: second button (to the left of first) with rounded corners
            D2D1_ROUNDED_RECT roundedButton3 = D2D1::RoundedRect(
                D2D1::RectF(BUTTON3_X, BUTTON3_Y, BUTTON3_X + BUTTON3_WIDTH, BUTTON3_Y + BUTTON3_HEIGHT),
                3.0f, 3.0f
            );
            g_pRenderTarget->FillRoundedRectangle(roundedButton3, g_pButton2Brush);
            // Draw lighter outline inside
            D2D1_ROUNDED_RECT roundedButton3Outline = D2D1::RoundedRect(
                D2D1::RectF(BUTTON3_X + 0.5f, BUTTON3_Y + 0.5f, BUTTON3_X + BUTTON3_WIDTH - 0.5f, BUTTON3_Y + BUTTON3_HEIGHT - 0.5f),
                2.5f, 2.5f
            );
            g_pRenderTarget->DrawRoundedRectangle(roundedButton3Outline, g_pButton2OutlineBrush, 1.0f);

            // Draw "Back" text on left button
            if (g_pButtonTextFormat && g_pWhiteTextBrush) {
                D2D1_RECT_F button3Rect = D2D1::RectF(BUTTON3_X, BUTTON3_Y, BUTTON3_X + BUTTON3_WIDTH, BUTTON3_Y + BUTTON3_HEIGHT);
                g_pRenderTarget->DrawText(
                    L"BACK",
                    4,
                    g_pButtonTextFormat,
                    button3Rect,
                    g_pWhiteTextBrush
                );
            }
        } else if (g_CurrentPage == 3) {
            bool installDirExists = g_IsUpdateMode.load();
            if (!g_IsInstalling.load() && !g_InstallComplete.load()) {
                wchar_t appdataPath[MAX_PATH];
                if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appdataPath))) {
                    fs::path installDir = fs::path(appdataPath) / L"PistachioCreamMenu";
                    installDirExists = fs::exists(installDir) && fs::is_directory(installDir);
                    g_IsUpdateMode = installDirExists;
                }
            }
            const wchar_t* buttonText = g_InstallComplete.load() ? L"EXIT" : (installDirExists ? L"UPDATE" : L"INSTALL");
                
                if (g_pWelcomeTextFormat && g_pWhiteTextBrush) {
                    D2D1_RECT_F titleRect = D2D1::RectF(0, 80, 600, 120);
                    g_pRenderTarget->DrawText(
                        L"PISTACHIO CREAM MENU",
                        20,
                        g_pWelcomeTextFormat,
                        titleRect,
                        g_pWhiteTextBrush
                    );
                }

                const float PROGRESS_WIDTH = 350.0f;
                const float PROGRESS_HEIGHT = 4.0f;
                const float PROGRESS_X = (600.0f - PROGRESS_WIDTH) / 2.0f;
                const float PROGRESS_Y = 200.0f;

                D2D1_ROUNDED_RECT progressBg = D2D1::RoundedRect(
                    D2D1::RectF(PROGRESS_X, PROGRESS_Y, PROGRESS_X + PROGRESS_WIDTH, PROGRESS_Y + PROGRESS_HEIGHT),
                    2.0f, 2.0f
                );
                g_pRenderTarget->FillRoundedRectangle(progressBg, g_pButton2OutlineBrush);

                float progressPercent = g_InstallProgress.load();
                if (progressPercent > 0.0f) {
                    D2D1_ROUNDED_RECT progressFill = D2D1::RoundedRect(
                        D2D1::RectF(PROGRESS_X, PROGRESS_Y, PROGRESS_X + (PROGRESS_WIDTH * progressPercent), PROGRESS_Y + PROGRESS_HEIGHT),
                        2.0f, 2.0f
                    );
                    g_pRenderTarget->FillRoundedRectangle(progressFill, g_pGrayTextBrush);
                }

                const float UPDATE_BUTTON_WIDTH = 120.0f;
                const float UPDATE_BUTTON_HEIGHT = 35.0f;
                const float UPDATE_BUTTON_X = (600.0f - UPDATE_BUTTON_WIDTH) / 2.0f;
                const float UPDATE_BUTTON_Y = 310.0f;

                D2D1_ROUNDED_RECT updateButton = D2D1::RoundedRect(
                    D2D1::RectF(UPDATE_BUTTON_X, UPDATE_BUTTON_Y, UPDATE_BUTTON_X + UPDATE_BUTTON_WIDTH, UPDATE_BUTTON_Y + UPDATE_BUTTON_HEIGHT),
                    3.0f, 3.0f
                );
                bool buttonDisabled = g_IsInstalling.load() && !g_InstallComplete.load();
                if (buttonDisabled) {
                    g_pButton2Brush->SetOpacity(0.5f);
                }
                g_pRenderTarget->FillRoundedRectangle(updateButton, g_pButton2Brush);
                if (buttonDisabled) {
                    g_pButton2Brush->SetOpacity(1.0f);
                }

                D2D1_ROUNDED_RECT updateButtonOutline = D2D1::RoundedRect(
                    D2D1::RectF(UPDATE_BUTTON_X + 0.5f, UPDATE_BUTTON_Y + 0.5f, UPDATE_BUTTON_X + UPDATE_BUTTON_WIDTH - 0.5f, UPDATE_BUTTON_Y + UPDATE_BUTTON_HEIGHT - 0.5f),
                    2.5f, 2.5f
                );
                g_pRenderTarget->DrawRoundedRectangle(updateButtonOutline, g_pButton2OutlineBrush, 1.0f);

                if (g_pButtonTextFormat && g_pWhiteTextBrush) {
                    D2D1_RECT_F updateTextRect = D2D1::RectF(UPDATE_BUTTON_X, UPDATE_BUTTON_Y, UPDATE_BUTTON_X + UPDATE_BUTTON_WIDTH, UPDATE_BUTTON_Y + UPDATE_BUTTON_HEIGHT);
                    if (buttonDisabled) {
                        g_pWhiteTextBrush->SetOpacity(0.5f);
                    }
                    g_pRenderTarget->DrawText(
                        buttonText,
                        (UINT32)wcslen(buttonText),
                        g_pButtonTextFormat,
                        updateTextRect,
                        g_pWhiteTextBrush
                    );
                    if (buttonDisabled) {
                        g_pWhiteTextBrush->SetOpacity(1.0f);
                    }
                }

            // Draw BACK button (bottom left)
            D2D1_ROUNDED_RECT backButton = D2D1::RoundedRect(
                D2D1::RectF(BUTTON5_X, BUTTON5_Y, BUTTON5_X + BUTTON5_WIDTH, BUTTON5_Y + BUTTON5_HEIGHT),
                3.0f, 3.0f
            );
            g_pRenderTarget->FillRoundedRectangle(backButton, g_pButton2Brush);

            // Draw BACK button outline
            D2D1_ROUNDED_RECT backButtonOutline = D2D1::RoundedRect(
                D2D1::RectF(BUTTON5_X + 0.5f, BUTTON5_Y + 0.5f, BUTTON5_X + BUTTON5_WIDTH - 0.5f, BUTTON5_Y + BUTTON5_HEIGHT - 0.5f),
                2.5f, 2.5f
            );
            g_pRenderTarget->DrawRoundedRectangle(backButtonOutline, g_pButton2OutlineBrush, 1.0f);

            // Draw "BACK" text
            if (g_pButtonTextFormat && g_pWhiteTextBrush) {
                D2D1_RECT_F backTextRect = D2D1::RectF(BUTTON5_X, BUTTON5_Y, BUTTON5_X + BUTTON5_WIDTH, BUTTON5_Y + BUTTON5_HEIGHT);
                g_pRenderTarget->DrawText(
                    L"BACK",
                    4,
                    g_pButtonTextFormat,
                    backTextRect,
                    g_pWhiteTextBrush
                );
            }
        }

        // Draw X icon
        ID2D1PathGeometry* pathGeometry = nullptr;
        if (SUCCEEDED(g_pD2DFactory->CreatePathGeometry(&pathGeometry))) {
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(pathGeometry->Open(&sink))) {
                float centerX = X_BUTTON_X + X_BUTTON_SIZE / 2.0f;
                float centerY = X_BUTTON_Y + X_BUTTON_SIZE / 2.0f;
                float size = 8.0f;
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

                // Draw the X with proper stroke
                float opacity = g_XButtonHover ? 1.0f : 0.7f;
                g_pXIconBrush->SetOpacity(opacity);
                g_pRenderTarget->DrawGeometry(pathGeometry, g_pXIconBrush, 1.5f);

                pathGeometry->Release();
            }
        }

        hr = g_pRenderTarget->EndDraw();

        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) {
            CleanupDeviceResources();
        }
    }

    ValidateRect(hwnd, nullptr);
}

std::wstring GetDesktopPath()
{
    wchar_t pathBuf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, pathBuf))) {
        return std::wstring(pathBuf);
    }
    wchar_t userProfile[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, userProfile))) {
        return fs::path(userProfile) / L"Desktop";
    }
    return L"";
}

bool DownloadFile(const std::wstring& url, const std::wstring& filepath)
{
    HINTERNET hSession = WinHttpOpen(L"PCM_INSTALLER/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"pistachiocreammenu.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring path = L"/files/" + fs::path(filepath).filename().wstring();
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!result) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    result = WinHttpReceiveResponse(hRequest, nullptr);
    if (!result) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD bytesAvailable = 0;
    char buffer[8192];
    DWORD bytesRead = 0;

    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        DWORD bytesToRead = (bytesAvailable < sizeof(buffer)) ? bytesAvailable : sizeof(buffer);
        if (WinHttpReadData(hRequest, buffer, bytesToRead, &bytesRead) && bytesRead > 0) {
            file.write(buffer, bytesRead);
        } else {
            break;
        }
    }

    file.close();
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return true;
}

std::wstring GetStartMenuPath()
{
    wchar_t pathBuf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, SHGFP_TYPE_CURRENT, pathBuf))) {
        return std::wstring(pathBuf);
    }
    return L"";
}

bool CreateDesktopShortcut(const std::wstring& targetPath, const std::wstring& shortcutName)
{
    CoInitialize(nullptr);
    
    IShellLinkW* pShellLink = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    IPersistFile* pPersistFile = nullptr;
    pShellLink->SetPath(targetPath.c_str());
    pShellLink->SetWorkingDirectory(fs::path(targetPath).parent_path().wstring().c_str());
    
    hr = pShellLink->QueryInterface(IID_PPV_ARGS(&pPersistFile));
    if (SUCCEEDED(hr)) {
        std::wstring desktopPath = GetDesktopPath();
        std::wstring shortcutPath = (fs::path(desktopPath) / (shortcutName + L".lnk")).wstring();
        hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
        pPersistFile->Release();
    }
    
    pShellLink->Release();
    CoUninitialize();
    return SUCCEEDED(hr);
}

bool CreateStartMenuShortcut(const std::wstring& targetPath, const std::wstring& shortcutName)
{
    CoInitialize(nullptr);
    
    IShellLinkW* pShellLink = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink));
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    IPersistFile* pPersistFile = nullptr;
    pShellLink->SetPath(targetPath.c_str());
    pShellLink->SetWorkingDirectory(fs::path(targetPath).parent_path().wstring().c_str());
    
    hr = pShellLink->QueryInterface(IID_PPV_ARGS(&pPersistFile));
    if (SUCCEEDED(hr)) {
        std::wstring startMenuPath = GetStartMenuPath();
        if (!startMenuPath.empty()) {
            std::wstring shortcutPath = (fs::path(startMenuPath) / (shortcutName + L".lnk")).wstring();
            hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
        }
        pPersistFile->Release();
    }
    
    pShellLink->Release();
    CoUninitialize();
    return SUCCEEDED(hr);
}

DWORD WINAPI InstallThread(LPVOID param)
{
    HWND hwnd = (HWND)param;
    g_IsInstalling = true;
    g_InstallProgress = 0.0f;
    g_InstallComplete = false;
    InvalidateRect(hwnd, nullptr, FALSE);

    wchar_t tempPath[MAX_PATH];
    wchar_t appdataPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempPath) || !SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appdataPath))) {
        g_IsInstalling = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 1;
    }

    fs::path installDir = fs::path(appdataPath) / L"PistachioCreamMenu";
    std::wstring desktopPath = GetDesktopPath();
    fs::path desktopShortcutPath = fs::path(desktopPath) / L"PCM.lnk";
    std::wstring startMenuPath = GetStartMenuPath();
    fs::path startMenuShortcutPath = fs::path(startMenuPath) / L"Pistachio Cream Menu.lnk";

    g_InstallProgress = 0.1f;
    InvalidateRect(hwnd, nullptr, FALSE);

    if (fs::exists(desktopShortcutPath)) {
        try {
            fs::remove(desktopShortcutPath);
        } catch (...) {}
    }

    if (!startMenuPath.empty() && fs::exists(startMenuShortcutPath)) {
        try {
            fs::remove(startMenuShortcutPath);
        } catch (...) {}
    }

    for (const wchar_t* filename : {L"PCM.exe", L"LAUNCHER.exe"}) {
        fs::path tempFile = fs::path(tempPath) / filename;
        if (fs::exists(tempFile)) {
            try {
                fs::remove(tempFile);
            } catch (...) {}
        }
    }

    if (fs::exists(installDir) && fs::is_directory(installDir)) {
        try {
            fs::remove_all(installDir);
        } catch (...) {}
    }

    g_InstallProgress = 0.2f;
    InvalidateRect(hwnd, nullptr, FALSE);

    try {
        fs::create_directories(installDir);
    } catch (...) {
        g_IsInstalling = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 1;
    }

    const wchar_t* filesToDownload[] = {L"PCM.exe", L"LAUNCHER.exe"};
    const int numFiles = 2;

    for (int i = 0; i < numFiles; i++) {
        const wchar_t* filename = filesToDownload[i];
        fs::path tempFile = fs::path(tempPath) / filename;
        fs::path finalFile = installDir / filename;

        float progressStart = 0.2f + (i * 0.35f);
        float progressEnd = 0.2f + ((i + 1) * 0.35f);

        g_InstallProgress = progressStart;
        InvalidateRect(hwnd, nullptr, FALSE);

        if (!DownloadFile(L"", tempFile.wstring())) {
            g_IsInstalling = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 1;
        }

        g_InstallProgress = progressStart + 0.3f;
        InvalidateRect(hwnd, nullptr, FALSE);

        if (fs::exists(finalFile)) {
            try {
                fs::remove(finalFile);
            } catch (...) {}
        }

        try {
            fs::rename(tempFile, finalFile);
        } catch (...) {
            g_IsInstalling = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 1;
        }

        g_InstallProgress = progressEnd;
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    g_InstallProgress = 0.9f;
    InvalidateRect(hwnd, nullptr, FALSE);

    fs::path launcherExe = installDir / L"LAUNCHER.exe";
    if (fs::exists(launcherExe)) {
        CreateDesktopShortcut(launcherExe.wstring(), L"PCM");
        CreateStartMenuShortcut(launcherExe.wstring(), L"Pistachio Cream Menu");
    }

    g_InstallProgress = 1.0f;
    g_IsInstalling = false;
    g_InstallComplete = true;
    InvalidateRect(hwnd, nullptr, FALSE);

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_PAINT:
        OnPaint(hwnd);
        return 0;

    case WM_SIZE:
        if (g_pRenderTarget) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            g_pRenderTarget->Resize(D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top));
        }
        return 0;

    case WM_MOUSEWHEEL:
    {
        if (g_CurrentPage == 2) {
            // Get scroll delta
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            const float scrollSpeed = 20.0f;

            // Update scroll offset
            g_TermsScrollOffset -= (delta / 120.0f) * scrollSpeed;

            // Clamp scroll offset (approximate max scroll based on content height)
            const float maxScroll = 220.0f; // Adjust based on total content height
            if (g_TermsScrollOffset < 0.0f) g_TermsScrollOffset = 0.0f;
            if (g_TermsScrollOffset > maxScroll) g_TermsScrollOffset = maxScroll;

            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        bool wasButtonHover = g_ButtonHover;
        bool wasButton3Hover = g_Button3Hover;
        bool wasButton4Hover = g_Button4Hover;
        bool wasButton5Hover = g_Button5Hover;
        bool wasXHover = g_XButtonHover;

        // Handle scrollbar dragging
        if (g_ScrollbarDragging && g_CurrentPage == 2) {
            const float BOX_WIDTH = 560.0f;
            const float BOX_HEIGHT = 201.0f;
            const float BOX_X = (600.0f - BOX_WIDTH) / 2.0f;
            const float BOX_Y = 120.0f;
            const float SCROLLBAR_Y = BOX_Y + 5.0f;
            const float SCROLLBAR_HEIGHT = BOX_HEIGHT - 10.0f;
            const float MAX_SCROLL = 220.0f;
            const float CONTENT_HEIGHT = 380.0f;
            const float THUMB_RATIO = BOX_HEIGHT / CONTENT_HEIGHT;
            const float THUMB_HEIGHT = SCROLLBAR_HEIGHT * THUMB_RATIO;

            // Calculate scroll position from mouse Y
            float relativeY = pt.y - SCROLLBAR_Y;
            float scrollRatio = relativeY / (SCROLLBAR_HEIGHT - THUMB_HEIGHT);
            g_TermsScrollOffset = scrollRatio * MAX_SCROLL;

            // Clamp
            if (g_TermsScrollOffset < 0.0f) g_TermsScrollOffset = 0.0f;
            if (g_TermsScrollOffset > MAX_SCROLL) g_TermsScrollOffset = MAX_SCROLL;

            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        // Check if mouse is over button
        if (g_CurrentPage == 1) {
            g_ButtonHover = (pt.x >= BUTTON_X && pt.x <= BUTTON_X + BUTTON_WIDTH &&
                            pt.y >= BUTTON_Y && pt.y <= BUTTON_Y + BUTTON_HEIGHT);
            g_Button3Hover = false;
            g_Button4Hover = false;
        } else if (g_CurrentPage == 2) {
            g_ButtonHover = g_CheckboxChecked && (pt.x >= BUTTON2_X && pt.x <= BUTTON2_X + BUTTON2_WIDTH &&
                            pt.y >= BUTTON2_Y && pt.y <= BUTTON2_Y + BUTTON2_HEIGHT);
            g_Button3Hover = (pt.x >= BUTTON3_X && pt.x <= BUTTON3_X + BUTTON3_WIDTH &&
                             pt.y >= BUTTON3_Y && pt.y <= BUTTON3_Y + BUTTON3_HEIGHT);
            g_Button4Hover = false;
            g_Button5Hover = false;
        } else if (g_CurrentPage == 3) {
            g_ButtonHover = false;
            g_Button3Hover = false;
            g_Button4Hover = (pt.x >= BUTTON4_X && pt.x <= BUTTON4_X + BUTTON4_WIDTH &&
                             pt.y >= BUTTON4_Y && pt.y <= BUTTON4_Y + BUTTON4_HEIGHT);
            g_Button5Hover = !g_IsInstalling.load() && (pt.x >= BUTTON5_X && pt.x <= BUTTON5_X + BUTTON5_WIDTH &&
                             pt.y >= BUTTON5_Y && pt.y <= BUTTON5_Y + BUTTON5_HEIGHT);
        } else {
            g_ButtonHover = false;
            g_Button3Hover = false;
            g_Button4Hover = false;
            g_Button5Hover = false;
        }

        // Check if mouse is over X button
        g_XButtonHover = (pt.x >= X_BUTTON_X && pt.x <= X_BUTTON_X + X_BUTTON_SIZE &&
                         pt.y >= X_BUTTON_Y && pt.y <= X_BUTTON_Y + X_BUTTON_SIZE);

        // Check if mouse is over checkbox (page 2 only)
        bool wasCheckboxHover = g_CheckboxHover;
        if (g_CurrentPage == 2) {
            const float BOX_WIDTH = 560.0f;
            const float BOX_HEIGHT = 201.0f;
            const float BOX_X = (600.0f - BOX_WIDTH) / 2.0f;
            const float BOX_Y = 120.0f;
            const float CHECKBOX_SIZE = 20.0f;
            const float CHECKBOX_X = BOX_X + 10.0f;
            const float CHECKBOX_Y = BOX_Y + BOX_HEIGHT + 30.0f;

            g_CheckboxHover = (pt.x >= CHECKBOX_X - 2.0f && pt.x <= CHECKBOX_X + CHECKBOX_SIZE + 2.0f &&
                              pt.y >= CHECKBOX_Y - 2.0f && pt.y <= CHECKBOX_Y + CHECKBOX_SIZE + 2.0f);
        } else {
            g_CheckboxHover = false;
        }

        if (wasButtonHover != g_ButtonHover || wasButton3Hover != g_Button3Hover ||
            wasButton4Hover != g_Button4Hover || wasButton5Hover != g_Button5Hover ||
            wasXHover != g_XButtonHover || wasCheckboxHover != g_CheckboxHover) {
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };

        // Check if click is on checkbox (page 2 only)
        if (g_CurrentPage == 2) {
            const float BOX_WIDTH = 560.0f;
            const float BOX_HEIGHT = 201.0f;
            const float BOX_X = (600.0f - BOX_WIDTH) / 2.0f;
            const float BOX_Y = 120.0f;
            const float CHECKBOX_SIZE = 20.0f;
            const float CHECKBOX_X = BOX_X + 10.0f;
            const float CHECKBOX_Y = BOX_Y + BOX_HEIGHT + 30.0f;

            // Add some padding for easier clicking
            if (pt.x >= CHECKBOX_X - 2.0f && pt.x <= CHECKBOX_X + CHECKBOX_SIZE + 2.0f &&
                pt.y >= CHECKBOX_Y - 2.0f && pt.y <= CHECKBOX_Y + CHECKBOX_SIZE + 2.0f) {
                g_CheckboxChecked = !g_CheckboxChecked;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
        }

        // Check if click is on scrollbar (page 2 only)
        if (g_CurrentPage == 2) {
            const float BOX_WIDTH = 560.0f;
            const float BOX_HEIGHT = 201.0f;
            const float BOX_X = (600.0f - BOX_WIDTH) / 2.0f;
            const float BOX_Y = 120.0f;
            const float SCROLLBAR_WIDTH = 4.0f;
            const float SCROLLBAR_X = BOX_X + BOX_WIDTH - 10.0f;
            const float SCROLLBAR_Y = BOX_Y + 5.0f;
            const float SCROLLBAR_HEIGHT = BOX_HEIGHT - 10.0f;

            if (pt.x >= SCROLLBAR_X - 5.0f && pt.x <= SCROLLBAR_X + SCROLLBAR_WIDTH + 5.0f &&
                pt.y >= SCROLLBAR_Y && pt.y <= SCROLLBAR_Y + SCROLLBAR_HEIGHT) {
                g_ScrollbarDragging = true;
                SetCapture(hwnd);
                return 0;
            }
        }

        // Check if click is on X button
        if (pt.x >= X_BUTTON_X && pt.x <= X_BUTTON_X + X_BUTTON_SIZE &&
            pt.y >= X_BUTTON_Y && pt.y <= X_BUTTON_Y + X_BUTTON_SIZE) {
            DestroyWindow(hwnd);
            return 0;
        }

        // Check if click is on button
        if (g_CurrentPage == 1 &&
            pt.x >= BUTTON_X && pt.x <= BUTTON_X + BUTTON_WIDTH &&
            pt.y >= BUTTON_Y && pt.y <= BUTTON_Y + BUTTON_HEIGHT) {
            // Switch to page 2
            g_CurrentPage = 2;
            g_TermsScrollOffset = 0.0f; // Reset scroll
            InvalidateRect(hwnd, nullptr, FALSE);
        } else if (g_CurrentPage == 2 && g_CheckboxChecked &&
                   pt.x >= BUTTON2_X && pt.x <= BUTTON2_X + BUTTON2_WIDTH &&
                   pt.y >= BUTTON2_Y && pt.y <= BUTTON2_Y + BUTTON2_HEIGHT) {
            wchar_t appdataPath[MAX_PATH];
            bool installDirExists = false;
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appdataPath))) {
                fs::path installDir = fs::path(appdataPath) / L"PistachioCreamMenu";
                installDirExists = fs::exists(installDir) && fs::is_directory(installDir);
            }
            g_IsUpdateMode = installDirExists;
            g_IsInstalling = false;
            g_InstallComplete = false;
            g_InstallProgress = 0.0f;
            g_CurrentPage = 3;
            InvalidateRect(hwnd, nullptr, FALSE);
        } else if (g_CurrentPage == 2 &&
                   pt.x >= BUTTON3_X && pt.x <= BUTTON3_X + BUTTON3_WIDTH &&
                   pt.y >= BUTTON3_Y && pt.y <= BUTTON3_Y + BUTTON3_HEIGHT) {
            // Left button - Switch back to page 1
            g_CurrentPage = 1;
            InvalidateRect(hwnd, nullptr, FALSE);
        } else if (g_CurrentPage == 3 &&
                   pt.x >= BUTTON4_X && pt.x <= BUTTON4_X + BUTTON4_WIDTH &&
                   pt.y >= BUTTON4_Y && pt.y <= BUTTON4_Y + BUTTON4_HEIGHT) {
            if (g_InstallComplete.load()) {
                DestroyWindow(hwnd);
            } else if (!g_IsInstalling.load()) {
                CreateThread(nullptr, 0, InstallThread, hwnd, 0, nullptr);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        } else if (g_CurrentPage == 3 &&
                   pt.x >= BUTTON5_X && pt.x <= BUTTON5_X + BUTTON5_WIDTH &&
                   pt.y >= BUTTON5_Y && pt.y <= BUTTON5_Y + BUTTON5_HEIGHT) {
            if (!g_IsInstalling.load()) {
                g_CurrentPage = 2;
                g_TermsScrollOffset = 0.0f;
                g_InstallProgress = 0.0f;
                g_InstallComplete = false;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        if (g_ScrollbarDragging) {
            g_ScrollbarDragging = false;
            ReleaseCapture();
        }
        return 0;
    }

    case WM_SETCURSOR:
    {
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);

            // Check if cursor is over interactive elements
            bool overButton = false;
            if (g_CurrentPage == 1) {
                overButton = (pt.x >= BUTTON_X && pt.x <= BUTTON_X + BUTTON_WIDTH &&
                             pt.y >= BUTTON_Y && pt.y <= BUTTON_Y + BUTTON_HEIGHT);
            } else if (g_CurrentPage == 2) {
                // Check if over Next button (only clickable if checkbox is checked)
                bool overNextButton = (pt.x >= BUTTON2_X && pt.x <= BUTTON2_X + BUTTON2_WIDTH &&
                                       pt.y >= BUTTON2_Y && pt.y <= BUTTON2_Y + BUTTON2_HEIGHT);
                // Check if over Back button
                bool overBackButton = (pt.x >= BUTTON3_X && pt.x <= BUTTON3_X + BUTTON3_WIDTH &&
                                       pt.y >= BUTTON3_Y && pt.y <= BUTTON3_Y + BUTTON3_HEIGHT);

                overButton = (overNextButton && g_CheckboxChecked) || overBackButton;
            } else if (g_CurrentPage == 3) {
                bool overUpdateButton = (pt.x >= BUTTON4_X && pt.x <= BUTTON4_X + BUTTON4_WIDTH &&
                                         pt.y >= BUTTON4_Y && pt.y <= BUTTON4_Y + BUTTON4_HEIGHT);
                bool overBackButton3 = !g_IsInstalling.load() && (pt.x >= BUTTON5_X && pt.x <= BUTTON5_X + BUTTON5_WIDTH &&
                                        pt.y >= BUTTON5_Y && pt.y <= BUTTON5_Y + BUTTON5_HEIGHT);
                overButton = overUpdateButton || overBackButton3;
            }

            bool overX = (pt.x >= X_BUTTON_X && pt.x <= X_BUTTON_X + X_BUTTON_SIZE &&
                         pt.y >= X_BUTTON_Y && pt.y <= X_BUTTON_Y + X_BUTTON_SIZE);

            // Check if over checkbox (page 2 only)
            bool overCheckbox = false;
            if (g_CurrentPage == 2) {
                const float BOX_WIDTH = 560.0f;
                const float BOX_HEIGHT = 201.0f;
                const float BOX_X = (600.0f - BOX_WIDTH) / 2.0f;
                const float BOX_Y = 120.0f;
                const float CHECKBOX_SIZE = 20.0f;
                const float CHECKBOX_X = BOX_X + 10.0f;
                const float CHECKBOX_Y = BOX_Y + BOX_HEIGHT + 30.0f;

                overCheckbox = (pt.x >= CHECKBOX_X - 2.0f && pt.x <= CHECKBOX_X + CHECKBOX_SIZE + 2.0f &&
                               pt.y >= CHECKBOX_Y - 2.0f && pt.y <= CHECKBOX_Y + CHECKBOX_SIZE + 2.0f);
            }

            if (overButton || overX || overCheckbox) {
                SetCursor(LoadCursor(nullptr, IDC_HAND));
                return TRUE;
            }
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProc(hwnd, uMsg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ScreenToClient(hwnd, &pt);

            // Make top 20 pixels draggable, except for X button area
            if (pt.y < 20) {
                // Don't make X button area draggable
                if (pt.x >= X_BUTTON_X && pt.x <= X_BUTTON_X + X_BUTTON_SIZE) {
                    return HTCLIENT;
                }
                return HTCAPTION;
            }
        }
        return hit;
    }

    case WM_DESTROY:
        CleanupDeviceResources();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    // Initialize COM
    CoInitialize(nullptr);

    // Create D2D factory
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
    if (FAILED(hr)) {
        CoUninitialize();
        return 0;
    }

    // Create DirectWrite factory
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&g_pDWriteFactory)
    );
    if (FAILED(hr)) {
        if (g_pD2DFactory) g_pD2DFactory->Release();
        CoUninitialize();
        return 0;
    }

    // Create text format for button text (centered, semi-bold)
    hr = g_pDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        25.0f,
        L"en-us",
        &g_pTextFormat
    );
    if (FAILED(hr)) {
        if (g_pDWriteFactory) g_pDWriteFactory->Release();
        if (g_pD2DFactory) g_pD2DFactory->Release();
        CoUninitialize();
        return 0;
    }

    // Center the text horizontally and vertically
    g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create text format for welcome text (centered, normal weight)
    hr = g_pDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        25.0f,
        L"en-us",
        &g_pWelcomeTextFormat
    );
    if (FAILED(hr)) {
        if (g_pTextFormat) g_pTextFormat->Release();
        if (g_pDWriteFactory) g_pDWriteFactory->Release();
        if (g_pD2DFactory) g_pD2DFactory->Release();
        CoUninitialize();
        return 0;
    }

    // Center the welcome text horizontally
    g_pWelcomeTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    g_pWelcomeTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    // Create text format for small button text (centered, normal weight)
    hr = g_pDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.0f,
        L"en-us",
        &g_pButtonTextFormat
    );
    if (FAILED(hr)) {
        if (g_pWelcomeTextFormat) g_pWelcomeTextFormat->Release();
        if (g_pTextFormat) g_pTextFormat->Release();
        if (g_pDWriteFactory) g_pDWriteFactory->Release();
        if (g_pD2DFactory) g_pD2DFactory->Release();
        CoUninitialize();
        return 0;
    }

    // Center the button text horizontally and vertically
    g_pButtonTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    g_pButtonTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create text format for header text (left-aligned, bold)
    hr = g_pDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.0f,
        L"en-us",
        &g_pHeaderTextFormat
    );
    if (FAILED(hr)) {
        if (g_pButtonTextFormat) g_pButtonTextFormat->Release();
        if (g_pWelcomeTextFormat) g_pWelcomeTextFormat->Release();
        if (g_pTextFormat) g_pTextFormat->Release();
        if (g_pDWriteFactory) g_pDWriteFactory->Release();
        if (g_pD2DFactory) g_pD2DFactory->Release();
        CoUninitialize();
        return 0;
    }

    // Left-align the header text
    g_pHeaderTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    g_pHeaderTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    // Create text format for body text (left-aligned, normal weight)
    hr = g_pDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.0f,
        L"en-us",
        &g_pBodyTextFormat
    );
    if (FAILED(hr)) {
        if (g_pHeaderTextFormat) g_pHeaderTextFormat->Release();
        if (g_pButtonTextFormat) g_pButtonTextFormat->Release();
        if (g_pWelcomeTextFormat) g_pWelcomeTextFormat->Release();
        if (g_pTextFormat) g_pTextFormat->Release();
        if (g_pDWriteFactory) g_pDWriteFactory->Release();
        if (g_pD2DFactory) g_pD2DFactory->Release();
        CoUninitialize();
        return 0;
    }

    // Left-align the body text
    g_pBodyTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    g_pBodyTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    // Register window class
    const wchar_t CLASS_NAME[] = L"InstallerWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;

    RegisterClass(&wc);

    int windowWidth = 600;
    int windowHeight = 450;
    int x = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Installer",
        WS_POPUP,
        x, y,
        windowWidth, windowHeight,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    g_MainHwnd = hwnd;

    if (hwnd == nullptr) {
        if (g_pBodyTextFormat) g_pBodyTextFormat->Release();
        if (g_pHeaderTextFormat) g_pHeaderTextFormat->Release();
        if (g_pButtonTextFormat) g_pButtonTextFormat->Release();
        if (g_pWelcomeTextFormat) g_pWelcomeTextFormat->Release();
        if (g_pTextFormat) g_pTextFormat->Release();
        if (g_pDWriteFactory) g_pDWriteFactory->Release();
        if (g_pD2DFactory) g_pD2DFactory->Release();
        CoUninitialize();
        return 0;
    }

    // Set window corner preference for smooth rounded corners
    const DWORD cornerPref = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, 33, &cornerPref, sizeof(cornerPref)); // 33 = DWMWA_WINDOW_CORNER_PREFERENCE

    ShowWindow(hwnd, nCmdShow);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    if (g_pBodyTextFormat) {
        g_pBodyTextFormat->Release();
    }
    if (g_pHeaderTextFormat) {
        g_pHeaderTextFormat->Release();
    }
    if (g_pButtonTextFormat) {
        g_pButtonTextFormat->Release();
    }
    if (g_pWelcomeTextFormat) {
        g_pWelcomeTextFormat->Release();
    }
    if (g_pTextFormat) {
        g_pTextFormat->Release();
    }
    if (g_pDWriteFactory) {
        g_pDWriteFactory->Release();
    }
    if (g_pD2DFactory) {
        g_pD2DFactory->Release();
    }
    CoUninitialize();

    return 0;
}
