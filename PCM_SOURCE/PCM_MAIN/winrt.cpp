#ifndef WINRT_H
#define WINRT_H

#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <wrl/client.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <tlhelp32.h>
#include <memory>
#include <functional>

extern "C" {
    HRESULT CreateDirect3D11DeviceFromDXGIDevice(IDXGIDevice* dxgiDevice, IInspectable** device);
}

namespace WinRTCapture {

using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using Microsoft::WRL::ComPtr;

template<typename T>
winrt::com_ptr<T> GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& obj) {
    auto access = obj.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

// Frame data structure
struct CapturedFrame {
    std::vector<uint8_t> pixelData;
    int width;
    int height;
    uint64_t frameNumber;
    DXGI_FORMAT format;
};

// Frame consumer callback type
using FrameConsumerCallback = std::function<void(const CapturedFrame&)>;

// Centralized Frame Capture Manager (Singleton Pattern)
class FrameCaptureManager {
private:
    // WinRT objects
    Direct3D11CaptureFramePool framePool{ nullptr };
    GraphicsCaptureSession captureSession{ nullptr };
    GraphicsCaptureItem captureItem{ nullptr };
    IDirect3DDevice dxDevice{ nullptr };

    // D3D11 objects
    ID3D11Device* d3dDevice{ nullptr };
    ID3D11DeviceContext* d3dContext{ nullptr };

    // Shared frame storage
    ComPtr<ID3D11Texture2D> sharedGPUTexture{ nullptr };
    ComPtr<ID3D11Texture2D> stagingTextures[3]{};
    uint32_t stagingCopyIndex{ 0 };
    uint64_t stagingFrameNumber[3]{ 0, 0, 0 };
    bool stagingReady[3]{ false, false, false };
    CapturedFrame latestFrame;  
    std::atomic<uint64_t> cpuFrameCounter{ 0 };

    // Frame info
    int frameWidth{ 0 };
    int frameHeight{ 0 };
    DXGI_FORMAT frameFormat{ DXGI_FORMAT_UNKNOWN };
    winrt::Windows::Graphics::SizeInt32 lastContentSize{ 0, 0 };
    std::atomic<uint64_t> frameCounter{ 0 };
    std::atomic<bool> hasFrame{ false };

    // Window size tracking for auto-reinitialize
    HWND targetWindow{ nullptr };
    int lastWindowWidth{ 0 };
    int lastWindowHeight{ 0 };

    // Synchronization
    std::recursive_mutex captureMutex;
    std::atomic<bool> initialized{ false };
    std::atomic<bool> capturing{ false };

    // Frame consumers (registered callbacks)
    std::vector<FrameConsumerCallback> consumers;
    std::mutex consumersMutex;

    // Event handlers
    GraphicsCaptureItem::Closed_revoker closed;

    // Singleton instance
    static FrameCaptureManager* instance;

    FrameCaptureManager() = default;

    void OnClosed(GraphicsCaptureItem const&, winrt::Windows::Foundation::IInspectable const&) {
        std::lock_guard<std::recursive_mutex> lock(captureMutex);
        capturing = false;
        initialized = false;
        hasFrame = false;
        frameCounter = 0;
        cpuFrameCounter = 0;
        frameWidth = 0;
        frameHeight = 0;
        frameFormat = DXGI_FORMAT_UNKNOWN;
        lastContentSize = { 0, 0 };
        sharedGPUTexture.Reset();
        for (int i = 0; i < 3; i++) {
            stagingTextures[i].Reset();
            stagingReady[i] = false;
            stagingFrameNumber[i] = 0;
        }
        try { if (captureSession) captureSession.Close(); } catch (...) {}
        try { if (framePool) framePool.Close(); } catch (...) {}
        framePool = nullptr;
        captureSession = nullptr;
        captureItem = nullptr;
        dxDevice = nullptr;
    }

    bool CopyPixelsFromStaging(ID3D11Texture2D* tex, uint64_t texFrameNumber) {
        if (!tex) return false;
        if (frameWidth <= 0 || frameHeight <= 0) return false;
        if (latestFrame.pixelData.size() != static_cast<size_t>(frameWidth) * static_cast<size_t>(frameHeight) * 4)
            latestFrame.pixelData.resize(static_cast<size_t>(frameWidth) * static_cast<size_t>(frameHeight) * 4);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT hr = d3dContext->Map(tex, 0, D3D11_MAP_READ, 0, &mapped);
        if (hr == DXGI_ERROR_WAS_STILL_DRAWING) return false;
        if (FAILED(hr)) return false;

        uint8_t* srcData = static_cast<uint8_t*>(mapped.pData);
        for (int y = 0; y < frameHeight; y++) {
            memcpy(
                latestFrame.pixelData.data() + (static_cast<size_t>(y) * static_cast<size_t>(frameWidth) * 4),
                srcData + (static_cast<size_t>(y) * static_cast<size_t>(mapped.RowPitch)),
                static_cast<size_t>(frameWidth) * 4
            );
        }

        d3dContext->Unmap(tex, 0);
        cpuFrameCounter = texFrameNumber;
        latestFrame.width = frameWidth;
        latestFrame.height = frameHeight;
        latestFrame.format = frameFormat;
        latestFrame.frameNumber = texFrameNumber;
        return true;
    }

    void EnsureStagingTextures() {
        if (!d3dDevice || frameWidth <= 0 || frameHeight <= 0) return;
        if (frameFormat != DXGI_FORMAT_B8G8R8A8_UNORM && frameFormat != DXGI_FORMAT_R8G8B8A8_UNORM) return;

        bool need = false;
        for (int i = 0; i < 3; i++) {
            if (!stagingTextures[i]) { need = true; break; }
        }
        if (!need) {
            D3D11_TEXTURE2D_DESC d{};
            stagingTextures[0]->GetDesc(&d);
            if (d.Width != static_cast<UINT>(frameWidth) || d.Height != static_cast<UINT>(frameHeight) || d.Format != frameFormat)
                need = true;
        }
        if (!need) return;

        for (int i = 0; i < 3; i++) {
            stagingTextures[i].Reset();
            stagingReady[i] = false;
            stagingFrameNumber[i] = 0;
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(frameWidth);
        desc.Height = static_cast<UINT>(frameHeight);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = frameFormat;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        for (int i = 0; i < 3; i++) {
            d3dDevice->CreateTexture2D(&desc, nullptr, &stagingTextures[i]);
        }
    }

    void UpdateCpuFrameFromGpu() {
        if (!hasFrame.load()) return;
        if (!sharedGPUTexture) return;
        if (!d3dContext) return;
        if (frameWidth <= 0 || frameHeight <= 0) return;
        if (frameFormat != DXGI_FORMAT_B8G8R8A8_UNORM && frameFormat != DXGI_FORMAT_R8G8B8A8_UNORM) return;

        EnsureStagingTextures();
        if (!stagingTextures[0] || !stagingTextures[1] || !stagingTextures[2]) return;

        uint64_t curFrame = frameCounter.load();
        stagingCopyIndex = (stagingCopyIndex + 1) % 3;
        uint32_t copyIdx = stagingCopyIndex;

        d3dContext->CopyResource(stagingTextures[copyIdx].Get(), sharedGPUTexture.Get());
        d3dContext->Flush();
        stagingReady[copyIdx] = true;
        stagingFrameNumber[copyIdx] = curFrame;

        uint32_t mapIdx = (copyIdx + 2) % 3;
        if (!stagingReady[mapIdx]) mapIdx = copyIdx;

        for (int i = 0; i < 8; i++) {
            if (CopyPixelsFromStaging(stagingTextures[mapIdx].Get(), stagingFrameNumber[mapIdx])) return;
            mapIdx = (mapIdx + 2) % 3;
            Sleep(0);
        }
    }

    // Process frame and send to all consumers
    void ProcessAndDistributeFrame(Direct3D11CaptureFrame const& frame) {
        auto frameContentSize = frame.ContentSize();
        auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
        if (!frameSurface) return;

        D3D11_TEXTURE2D_DESC frameDesc;
        frameSurface->GetDesc(&frameDesc);

        if (frameDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM && frameDesc.Format != DXGI_FORMAT_R8G8B8A8_UNORM) {
            return;
        }

        if (frameContentSize.Width != lastContentSize.Width || frameContentSize.Height != lastContentSize.Height) {
            try {
                framePool.Recreate(dxDevice, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, frameContentSize);
            } catch (...) {}
            lastContentSize = frameContentSize;
        }

        if (!sharedGPUTexture ||
            frameDesc.Width != static_cast<UINT>(frameWidth) ||
            frameDesc.Height != static_cast<UINT>(frameHeight) ||
            frameDesc.Format != frameFormat) {

            sharedGPUTexture.Reset();
            frameWidth = static_cast<int>(frameDesc.Width);
            frameHeight = static_cast<int>(frameDesc.Height);
            frameFormat = frameDesc.Format;
            latestFrame.width = frameWidth;
            latestFrame.height = frameHeight;
            latestFrame.format = frameFormat;
            latestFrame.pixelData.clear();
            cpuFrameCounter = 0;
            for (int i = 0; i < 3; i++) {
                stagingTextures[i].Reset();
                stagingReady[i] = false;
                stagingFrameNumber[i] = 0;
            }

            D3D11_TEXTURE2D_DESC sharedDesc{};
            sharedDesc.Width = frameDesc.Width;
            sharedDesc.Height = frameDesc.Height;
            sharedDesc.MipLevels = 1;
            sharedDesc.ArraySize = 1;
            sharedDesc.Format = frameDesc.Format;
            sharedDesc.SampleDesc.Count = 1;
            sharedDesc.Usage = D3D11_USAGE_DEFAULT;
            sharedDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            sharedDesc.CPUAccessFlags = 0;
            sharedDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

            if (FAILED(d3dDevice->CreateTexture2D(&sharedDesc, nullptr, &sharedGPUTexture))) {
                return;
            }
        }

        d3dContext->CopyResource(sharedGPUTexture.Get(), frameSurface.get());
        d3dContext->Flush();

        frameCounter++;
        latestFrame.frameNumber = frameCounter.load();
        hasFrame = true;
    }

public:
    static FrameCaptureManager* GetInstance() {
        if (!instance) {
            instance = new FrameCaptureManager();
        }
        return instance;
    }

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND targetWindow) {
        std::lock_guard<std::recursive_mutex> lock(captureMutex);

        if (initialized.load()) return true;
        if (!device || !context) return false;

        this->targetWindow = targetWindow;
        if (targetWindow) {
            RECT clientRect;
            if (GetClientRect(targetWindow, &clientRect)) {
                lastWindowWidth = clientRect.right - clientRect.left;
                lastWindowHeight = clientRect.bottom - clientRect.top;
            }
        }

        d3dDevice = device;
        d3dContext = context;

        try {
            // Initialize WinRT
            static bool apartmentInitialized = false;
            if (!apartmentInitialized) {
                try {
                    winrt::init_apartment(winrt::apartment_type::multi_threaded);
                } catch (...) {}
                apartmentInitialized = true;
            }

            ComPtr<IDXGIDevice> dxgiDevice;
            if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) return false;

            winrt::com_ptr<IInspectable> inspectable;
            if (FAILED(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.Get(), inspectable.put()))) return false;
            dxDevice = inspectable.as<IDirect3DDevice>();

            // Create capture item for window
            auto activation_factory = get_activation_factory<GraphicsCaptureItem>();
            auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
            GraphicsCaptureItem item{ nullptr };
            const auto iid = winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>();

            if (FAILED(interop_factory->CreateForWindow(targetWindow, iid, reinterpret_cast<void**>(put_abi(item))))) {
                return false;
            }
            captureItem = item;

            auto itemSize = item.Size();
            frameWidth = itemSize.Width;
            frameHeight = itemSize.Height;

            try {
                GraphicsCaptureAccess::RequestAccessAsync(GraphicsCaptureAccessKind::Borderless).get();
            } catch (...) {}

            framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(
                dxDevice,
                DirectXPixelFormat::B8G8R8A8UIntNormalized,
                2,
                itemSize
            );

            captureSession = framePool.CreateCaptureSession(captureItem);

            try {
                captureSession.IsBorderRequired(false);
                captureSession.IsCursorCaptureEnabled(false);
            } catch (...) {}

            closed = captureItem.Closed(winrt::auto_revoke, {this, &FrameCaptureManager::OnClosed});

            initialized = true;
            return true;

        } catch (...) {
            return false;
        }
    }

    bool StartCapture() {
        std::lock_guard<std::recursive_mutex> lock(captureMutex);

        if (!initialized.load()) return false;
        if (capturing.load()) return true;

        try {
            captureSession.StartCapture();
            capturing = true;
            return true;
        } catch (...) {
            return false;
        }
    }

    void StopCapture() {
        std::lock_guard<std::recursive_mutex> lock(captureMutex);

        if (!capturing.load()) return;

        try {
            if (captureSession) {
                captureSession.Close();
            }
            capturing = false;
        } catch (...) {}
    }

    void Release() {
        std::lock_guard<std::recursive_mutex> lock(captureMutex);

        StopCapture();

        closed.revoke();

        try {
            if (framePool) framePool.Close();
        } catch (...) {}

        framePool = nullptr;
        captureSession = nullptr;
        captureItem = nullptr;
        dxDevice = nullptr;
        d3dDevice = nullptr;
        d3dContext = nullptr;
        sharedGPUTexture.Reset();
        for (int i = 0; i < 3; i++) stagingTextures[i].Reset();

        {
            std::lock_guard<std::mutex> consumerLock(consumersMutex);
            consumers.clear();
        }

        initialized = false;
        hasFrame = false;
        frameCounter = 0;
        cpuFrameCounter = 0;
        frameWidth = 0;
        frameHeight = 0;
        frameFormat = DXGI_FORMAT_UNKNOWN;
        lastContentSize = { 0, 0 };
        lastWindowWidth = 0;
        lastWindowHeight = 0;
    }

    // Register a frame consumer callback
    void RegisterConsumer(FrameConsumerCallback callback) {
        std::lock_guard<std::mutex> lock(consumersMutex);
        consumers.push_back(callback);
    }

    void ClearConsumers() {
        std::lock_guard<std::mutex> lock(consumersMutex);
        consumers.clear();
    }

    // Manual frame grab (for compatibility with existing code)
    bool GrabFrame() {
        std::lock_guard<std::recursive_mutex> lock(captureMutex);

        if (!initialized.load() || !capturing.load()) return false;
        if (!framePool) return false;

        if (targetWindow && IsWindow(targetWindow)) {
            RECT clientRect;
            if (GetClientRect(targetWindow, &clientRect)) {
                int currentWidth = clientRect.right - clientRect.left;
                int currentHeight = clientRect.bottom - clientRect.top;
                if ((lastWindowWidth > 0 && currentWidth != lastWindowWidth) || 
                    (lastWindowHeight > 0 && currentHeight != lastWindowHeight)) {
                    lastWindowWidth = currentWidth;
                    lastWindowHeight = currentHeight;
                    initialized = false;
                    capturing = false;
                    hasFrame = false;
                    try { if (captureSession) captureSession.Close(); } catch (...) {}
                    try { if (framePool) framePool.Close(); } catch (...) {}
                    closed.revoke();
                    framePool = nullptr;
                    captureSession = nullptr;
                    captureItem = nullptr;
                    dxDevice = nullptr;
                    sharedGPUTexture.Reset();
                    for (int i = 0; i < 3; i++) {
                        stagingTextures[i].Reset();
                        stagingReady[i] = false;
                        stagingFrameNumber[i] = 0;
                    }
                    frameWidth = 0;
                    frameHeight = 0;
                    frameFormat = DXGI_FORMAT_UNKNOWN;
                    lastContentSize = { 0, 0 };
                    if (d3dDevice && d3dContext) {
                        try {
                            Initialize(d3dDevice, d3dContext, targetWindow);
                            StartCapture();
                        } catch (...) {}
                    }
                    return false;
                }
            }
        }

        try {
            auto frame = framePool.TryGetNextFrame();
            if (frame) {
                ProcessAndDistributeFrame(frame);
                return true;
            }
        } catch (...) {}

        return false;
    }

    bool CopyCurrentFrameToTexture(ID3D11Texture2D* destTexture) {
        std::lock_guard<std::recursive_mutex> lock(captureMutex);

        if (!initialized.load() || !capturing.load() || !destTexture) return false;

        GrabFrame();

        if (!sharedGPUTexture) return false;

        ComPtr<ID3D11Device> destDevice;
        destTexture->GetDevice(&destDevice);
        if (!destDevice) return false;

        D3D11_TEXTURE2D_DESC srcDesc, destDesc;
        destTexture->GetDesc(&destDesc);

        if (d3dDevice && destDevice.Get() == d3dDevice) {
            if (!d3dContext) return false;
            sharedGPUTexture->GetDesc(&srcDesc);
            if (srcDesc.Width == destDesc.Width && srcDesc.Height == destDesc.Height && srcDesc.Format == destDesc.Format) {
                d3dContext->CopyResource(destTexture, sharedGPUTexture.Get());
            } else {
                D3D11_BOX srcBox = {};
                srcBox.right = (std::min)(srcDesc.Width, destDesc.Width);
                srcBox.bottom = (std::min)(srcDesc.Height, destDesc.Height);
                srcBox.back = 1;
                d3dContext->CopySubresourceRegion(destTexture, 0, 0, 0, 0, sharedGPUTexture.Get(), 0, &srcBox);
            }
            d3dContext->Flush();
            return true;
        }

        ComPtr<IDXGIResource> dxgiRes;
        if (FAILED(sharedGPUTexture.As(&dxgiRes)) || !dxgiRes) return false;

        HANDLE sharedHandle = nullptr;
        if (FAILED(dxgiRes->GetSharedHandle(&sharedHandle)) || !sharedHandle) return false;

        ComPtr<ID3D11Texture2D> sharedOnDest;
        HRESULT openHr = destDevice->OpenSharedResource(sharedHandle, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(sharedOnDest.GetAddressOf()));
        if (FAILED(openHr) || !sharedOnDest) return false;

        ComPtr<ID3D11DeviceContext> destCtx;
        destDevice->GetImmediateContext(&destCtx);
        if (!destCtx) return false;

        sharedOnDest->GetDesc(&srcDesc);
        if (srcDesc.Width == destDesc.Width && srcDesc.Height == destDesc.Height && srcDesc.Format == destDesc.Format) {
            destCtx->CopyResource(destTexture, sharedOnDest.Get());
        } else {
            D3D11_BOX srcBox = {};
            srcBox.right = (std::min)(srcDesc.Width, destDesc.Width);
            srcBox.bottom = (std::min)(srcDesc.Height, destDesc.Height);
            srcBox.back = 1;
            destCtx->CopySubresourceRegion(destTexture, 0, 0, 0, 0, sharedOnDest.Get(), 0, &srcBox);
        }

        destCtx->Flush();
        return true;
    }

    // Get latest frame data
    bool GetLatestFrame(CapturedFrame& outFrame) {
        std::lock_guard<std::recursive_mutex> lock(captureMutex);

        for (int i = 0; i < 10 && !hasFrame.load(); i++) {
            GrabFrame();
            Sleep(8);
        }
        if (!hasFrame.load()) return false;
        if (cpuFrameCounter.load() != frameCounter.load()) {
            UpdateCpuFrameFromGpu();
        }
        if (latestFrame.pixelData.empty()) return false;

        outFrame = latestFrame;
        return true;
    }

    // Get pixel color from latest frame
    uint32_t GetPixelColor(int x, int y) {
        std::lock_guard<std::recursive_mutex> lock(captureMutex);

        static uint64_t lastGrabMs = 0;
        uint64_t nowMs = GetTickCount64();
        if (nowMs != lastGrabMs) {
            lastGrabMs = nowMs;
            GrabFrame();
        }

        if (!hasFrame.load()) return 0;

        if (x < 0 || x >= frameWidth || y < 0 || y >= frameHeight) return 0;

        if (cpuFrameCounter.load() != frameCounter.load()) {
            UpdateCpuFrameFromGpu();
        }
        if (latestFrame.pixelData.empty()) return 0;

        int offset = (y * frameWidth + x) * 4;
        uint8_t c0 = latestFrame.pixelData[offset + 0];
        uint8_t c1 = latestFrame.pixelData[offset + 1];
        uint8_t c2 = latestFrame.pixelData[offset + 2];
        uint8_t a = latestFrame.pixelData[offset + 3];
        uint8_t r = 0, g = 0, b = 0;
        if (frameFormat == DXGI_FORMAT_R8G8B8A8_UNORM) {
            r = c0; g = c1; b = c2;
        } else {
            b = c0; g = c1; r = c2;
        }
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    // Getters
    bool IsInitialized() const { return initialized.load(); }
    bool IsCapturing() const { return capturing.load(); }
    bool IsReady() const { return initialized.load() && capturing.load() && framePool && captureSession && captureItem && dxDevice; }
    bool HasFrame() const { return hasFrame.load(); }
    int GetWidth() const { return frameWidth; }
    int GetHeight() const { return frameHeight; }
    uint64_t GetFrameCounter() const { return frameCounter.load(); }
    ID3D11Texture2D* GetGPUTexture() { return sharedGPUTexture.Get(); }
};

FrameCaptureManager* FrameCaptureManager::instance = nullptr;

// Global helper to find Roblox window
HWND FindRobloxWindow() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return nullptr;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(snapshot, &pe32)) {
        CloseHandle(snapshot);
        return nullptr;
    }

    do {
        if (_wcsicmp(pe32.szExeFile, L"RobloxPlayerBeta.exe") == 0) {
            DWORD pid = pe32.th32ProcessID;

            struct EnumData {
                DWORD processId;
                HWND hwnd;
            };
            EnumData data = { pid, nullptr };

            EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                EnumData* data = reinterpret_cast<EnumData*>(lParam);
                DWORD windowProcessId;
                GetWindowThreadProcessId(hwnd, &windowProcessId);
                if (windowProcessId == data->processId && IsWindowVisible(hwnd)) {
                    wchar_t className[256];
                    GetClassNameW(hwnd, className, 256);
                    if (wcsstr(className, L"WINDOWSCLIENT") != nullptr ||
                        wcsstr(className, L"Roblox") != nullptr) {
                        data->hwnd = hwnd;
                        return FALSE;
                    }
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(&data));

            CloseHandle(snapshot);

            if (data.hwnd) {
                return data.hwnd;
            }
        }
    } while (Process32NextW(snapshot, &pe32));

    CloseHandle(snapshot);
    return nullptr;
}

// ============================================================================
// PUBLIC API - Compatible with existing code
// ============================================================================

bool InitializeCapture(ID3D11Device* device, ID3D11DeviceContext* context) {
    auto manager = FrameCaptureManager::GetInstance();
    if (manager->IsReady()) return true;
    if (manager->IsInitialized() && !manager->IsCapturing()) manager->Release();

    HWND robloxWindow = FindRobloxWindow();
    if (!robloxWindow) return false;

    if (!manager->Initialize(device, context, robloxWindow)) {
        return false;
    }

    return manager->StartCapture();
}

void ReleaseCapture() {
    auto manager = FrameCaptureManager::GetInstance();
    manager->Release();
}

bool IsCaptureInitialized() {
    auto manager = FrameCaptureManager::GetInstance();
    return manager->IsReady();
}

uint32_t GetPixelColor(int x, int y) {
    auto manager = FrameCaptureManager::GetInstance();
    return manager->GetPixelColor(x, y);
}

int GetCaptureWidth() {
    auto manager = FrameCaptureManager::GetInstance();
    return manager->GetWidth();
}

int GetCaptureHeight() {
    auto manager = FrameCaptureManager::GetInstance();
    return manager->GetHeight();
}

uint64_t GetFrameCounter() {
    auto manager = FrameCaptureManager::GetInstance();
    return manager->GetFrameCounter();
}

// Get current frame texture (GPU texture)
bool GetCurrentFrameTexture(ID3D11Texture2D** outTexture) {
    auto manager = FrameCaptureManager::GetInstance();

    if (!manager->IsInitialized() || !outTexture) return false;

    manager->GrabFrame(); // Try to get fresh frame

    auto texture = manager->GetGPUTexture();
    if (!texture) return false;

    *outTexture = texture;
    (*outTexture)->AddRef();
    return true;
}

// Copy frame to texture
bool CopyFrameToTexture(ID3D11Texture2D* destTexture) {
    auto manager = FrameCaptureManager::GetInstance();
    if (!manager->IsInitialized() || !destTexture) return false;
    return manager->CopyCurrentFrameToTexture(destTexture);
}

// Legacy compatibility
bool AcquireFrame(ID3D11Texture2D** outTexture, int* outWidth, int* outHeight, bool requireFresh) {
    auto manager = FrameCaptureManager::GetInstance();

    if (!manager->IsInitialized()) return false;

    manager->GrabFrame();

    if (outTexture) {
        auto texture = manager->GetGPUTexture();
        if (!texture) return false;
        *outTexture = texture;
        (*outTexture)->AddRef();
    }

    if (outWidth) *outWidth = manager->GetWidth();
    if (outHeight) *outHeight = manager->GetHeight();

    return true;
}

// Register a callback to receive frames
void RegisterFrameConsumer(FrameConsumerCallback callback) {
    auto manager = FrameCaptureManager::GetInstance();
    manager->RegisterConsumer(callback);
}

// Capture screen region to bitmap
winrt::Windows::Graphics::Imaging::SoftwareBitmap CaptureScreenRegion(int x, int y, int width, int height) {
    auto manager = FrameCaptureManager::GetInstance();

    if (!manager->IsReady()) {
        return nullptr;
    }

    manager->GrabFrame();
    Sleep(16);

    ID3D11Texture2D* capturedTexture = manager->GetGPUTexture();
    if (!capturedTexture) {
        return nullptr;
    }

    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dContext;
    capturedTexture->GetDevice(&d3dDevice);
    if (!d3dDevice) {
        return nullptr;
    }
    d3dDevice->GetImmediateContext(&d3dContext);
    if (!d3dContext) {
        return nullptr;
    }

    try {
        D3D11_TEXTURE2D_DESC stagingDesc = {};
        stagingDesc.Width = width;
        stagingDesc.Height = height;
        stagingDesc.MipLevels = 1;
        stagingDesc.ArraySize = 1;
        stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.BindFlags = 0;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        ComPtr<ID3D11Texture2D> stagingTexture;
        HRESULT hr = d3dDevice->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
        if (FAILED(hr)) {
            return nullptr;
        }

        D3D11_BOX srcBox = {};
        srcBox.left = x;
        srcBox.top = y;
        srcBox.front = 0;
        srcBox.right = x + width;
        srcBox.bottom = y + height;
        srcBox.back = 1;

        d3dContext->CopySubresourceRegion(
            stagingTexture.Get(), 0, 0, 0, 0,
            capturedTexture, 0, &srcBox
        );

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = d3dContext->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);
        if (FAILED(hr)) {
            return nullptr;
        }

        BYTE* srcBytes = static_cast<BYTE*>(mappedResource.pData);
        std::vector<BYTE> pixelData(width * height * 4);

        for (int row = 0; row < height; row++) {
            memcpy(pixelData.data() + (row * width * 4), 
                   srcBytes + (row * mappedResource.RowPitch), 
                   width * 4);
        }

        d3dContext->Unmap(stagingTexture.Get(), 0);

        using namespace winrt::Windows::Graphics::Imaging;
        using namespace winrt::Windows::Storage::Streams;

        auto buffer = Buffer(width * height * 4);
        buffer.Length(width * height * 4);
        auto bufferData = buffer.data();
        memcpy(bufferData, pixelData.data(), width * height * 4);

        auto bitmap = SoftwareBitmap::CreateCopyFromBuffer(
            buffer,
            BitmapPixelFormat::Bgra8,
            width,
            height,
            BitmapAlphaMode::Ignore
        );

        return bitmap;
    } catch (...) {
        return nullptr;
    }
}

}

#endif
