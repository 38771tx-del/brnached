#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <algorithm>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <unknwn.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <tlhelp32.h>
#include <wrl.h>

// Resolve IUnknown ambiguity
typedef ::IUnknown IUnknownBase;

#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace WinRTCapture {
    bool InitializeCapture(ID3D11Device* device, ID3D11DeviceContext* context);
    bool IsCaptureInitialized();
    int GetCaptureWidth();
    int GetCaptureHeight();
    winrt::Windows::Graphics::Imaging::SoftwareBitmap CaptureScreenRegion(int x, int y, int width, int height);
    HWND FindRobloxWindow();
}

using namespace winrt;
using namespace Microsoft::WRL;

std::atomic<bool> chat_scanning(false);
std::atomic<int> chat_timer(0);

winrt::Windows::Media::Ocr::OcrEngine g_ocrEngine{ nullptr };
winrt::com_ptr<ID3D11Device> g_d3dDevice;
winrt::com_ptr<ID3D11DeviceContext> g_d3dContext;

bool InitializeDirect3D() {
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        g_d3dDevice.put(),
        nullptr,
        g_d3dContext.put()
    );

    return SUCCEEDED(hr);
}

bool InitializeCaptureSession() {
    if (!g_d3dDevice) {
        return false;
    }
    return WinRTCapture::InitializeCapture(g_d3dDevice.get(), g_d3dContext.get());
}

bool InitializeOCR() {
    try {
        winrt::Windows::Globalization::Language english(L"en");
        g_ocrEngine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(english);
        
        if (g_ocrEngine == nullptr) {
            g_ocrEngine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();
        }
        
        return g_ocrEngine != nullptr;
    } catch (...) {
        return false;
    }
}

winrt::Windows::Graphics::Imaging::SoftwareBitmap CaptureScreenRegion(int x, int y, int width, int height) {
    return WinRTCapture::CaptureScreenRegion(x, y, width, height);
}

void StartChatScanning() {
    if (chat_scanning.load()) {
        return;
    }
    
    if (!InitializeDirect3D()) {
        std::cout << "d3d_fail" << std::endl;
        return;
    }
    
    if (!InitializeCaptureSession()) {
        std::cout << "capture_fail" << std::endl;
        return;
    }
    
    Sleep(100);
    
    if (!WinRTCapture::IsCaptureInitialized()) {
        std::cout << "capture_not_ready" << std::endl;
        return;
    }
    
    if (!InitializeOCR()) {
        std::cout << "ocr_fail" << std::endl;
        return;
    }
    
    chat_scanning.store(true);
    chat_timer.store(1);
    std::cout << "ready" << std::endl;
}

void StopChatScanning() {
    if (!chat_scanning.load()) {
        return;
    }
    
    
    chat_scanning.store(false);
    chat_timer.store(0);
}

void PerformChatScan() {
    if (!chat_scanning.load()) {
        return;
    }
    
    if (!WinRTCapture::IsCaptureInitialized()) {
        return;
    }
    
    try {
        int captureWidth = WinRTCapture::GetCaptureWidth();
        int captureHeight = WinRTCapture::GetCaptureHeight();
        
        int x = 760;
        int y = 200;
        int w = 400;
        int h = 120;
        
        if (x < 0 || y < 0 || x >= captureWidth || y >= captureHeight) {
            return;
        }
        if (x + w > captureWidth) w = captureWidth - x;
        if (y + h > captureHeight) h = captureHeight - y;
        if (w <= 0 || h <= 0) return;
        
        winrt::Windows::Graphics::Imaging::SoftwareBitmap softwareBitmap = CaptureScreenRegion(x, y, w, h);
        if (softwareBitmap == nullptr) {
            std::cout << "ROI:[null]" << std::endl;
            return;
        }

        using namespace winrt::Windows::Graphics::Imaging;
        
        if (softwareBitmap.BitmapPixelFormat() != BitmapPixelFormat::Bgra8) {
            SoftwareBitmap converted = SoftwareBitmap::Convert(softwareBitmap, BitmapPixelFormat::Bgra8);
            softwareBitmap = converted;
        }

        winrt::Windows::Media::Ocr::OcrResult result = g_ocrEngine.RecognizeAsync(softwareBitmap).get();
        
        struct WordPos {
            std::wstring text;
            float x;
        };
        
        std::vector<WordPos> words;
        
        auto lines = result.Lines();
        for (uint32_t i = 0; i < lines.Size(); i++) {
            auto line = lines.GetAt(i);
            auto ocrWords = line.Words();
            for (uint32_t j = 0; j < ocrWords.Size(); j++) {
                auto word = ocrWords.GetAt(j);
                auto rect = word.BoundingRect();
                std::wstring wordText = word.Text().c_str();
                if (!wordText.empty()) {
                    words.push_back({ wordText, rect.X });
                }
            }
        }
        
        std::sort(words.begin(), words.end(), [](const WordPos& a, const WordPos& b) {
            return a.x < b.x;
        });

        std::wstring sortedText;
        for (size_t k = 0; k < words.size(); ++k) {
            if (!sortedText.empty()) sortedText += L" ";
            sortedText += words[k].text;
        }

        std::string text = winrt::to_string(sortedText);
        std::cout << "ROI:[" << text << "]" << std::endl;
        
    } catch (const std::exception& e) {
    } catch (...) {
    }
}

int main() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    
    StartChatScanning();
    
    if (chat_scanning.load()) {
        Sleep(200);
        for (;;) {
            PerformChatScan();
            Sleep(100);
        }
    }
    
    winrt::uninit_apartment();
    return 0;
}
