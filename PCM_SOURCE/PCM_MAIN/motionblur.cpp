#pragma once
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <algorithm>
#include <d3dcompiler.h>
#include <thread>
#include <wrl/client.h>
#include <mutex>
#include <atomic>

namespace ParryBar {
    bool IsGateCheckActive();
}

namespace WinRTCapture {
    bool InitializeCapture(ID3D11Device* device, ID3D11DeviceContext* context);
    bool IsCaptureInitialized();
    void ReleaseCapture();
    bool AcquireFrame(ID3D11Texture2D** outTexture, int* outWidth, int* outHeight, bool requireFresh);
    bool CopyFrameToTexture(ID3D11Texture2D* destTexture);
    bool GetCurrentFrameTexture(ID3D11Texture2D** outTexture);
    int GetCaptureWidth();
    int GetCaptureHeight();
    HWND FindRobloxWindow();
}

namespace MotionBlur {

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using Microsoft::WRL::ComPtr;

struct Application {
    HWND hwnd;
    HINSTANCE hInstance;
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;
    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* renderTargetView;
    int windowWidth;
    int windowHeight;
    bool isRunning;
    bool accumInitialized;
    ID3D11Buffer* quadVertexBuffer;
    ID3D11VertexShader* quadVertexShader;
    ID3D11PixelShader* quadPixelShader;
    ID3D11InputLayout* quadInputLayout;
    ID3D11SamplerState* samplerState;
    ID3D11Texture2D* desktopTexture;
    ID3D11ShaderResourceView* desktopSRV;
    
    ID3D11Texture2D* currTex;
    ID3D11ShaderResourceView* currSRV;
    ID3D11RenderTargetView* currRTV;
    
    ID3D11Texture2D* prevSingleTex;
    ID3D11ShaderResourceView* prevSingleSRV;
    ID3D11RenderTargetView* prevSingleRTV;
    
    ID3D11Texture2D* prevTex;
    ID3D11ShaderResourceView* prevSRV;
    ID3D11RenderTargetView* prevRTV;
    ID3D11Texture2D* combineResultTex;
    ID3D11ShaderResourceView* combineResultSRV;
    ID3D11RenderTargetView* combineResultRTV;
    ID3D11PixelShader* motionBlurPixelShader;
    ID3D11PixelShader* prevColorShader;
    ID3D11Buffer* cbParams;
    
    std::atomic<bool> hasValidFrame{ false };
    std::atomic<bool> waitingFirstFrame{ false };
};

static Application g_Application = {};
static std::thread g_thread;
static bool g_running = false;
static bool g_classRegistered = false;
static float g_mbIntensity = 1.0f;
static bool g_prevShouldRender = false;
static bool g_forceFreshFrame = true;
static bool g_isRobloxFullscreen = false;


void EnsureOverlaysOnTop(HWND motionBlurHwnd) {
    const wchar_t* overlayClasses[] = { 
        L"Keystrokes", 
        L"CPS", 
        L"CrosshairOverlay",
        L"ZoomVisual",
        L"BlurVisual",
        L"ScrollBlockerOverlay",
        L"InputOverlay",
        L"TraytipNotification",
        L"VisualParryBarWnd"
    };
    for (const wchar_t* className : overlayClasses) {
        HWND overlayHwnd = FindWindowW(className, nullptr);
        if (overlayHwnd && IsWindow(overlayHwnd) && IsWindowVisible(overlayHwnd)) {
            SetWindowPos(overlayHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
}

struct QuadVertex {
    float x, y, z;
    float u, v;
};

const char* quadVertexShaderSource = R"(
struct VS_INPUT {
   float3 pos : POSITION;
   float2 uv : TEXCOORD;
};
struct VS_OUTPUT {
   float4 pos : SV_POSITION;
   float2 uv : TEXCOORD;
};
VS_OUTPUT main(VS_INPUT input) {
   VS_OUTPUT output;
   output.pos = float4(input.pos, 1.0f);
   output.uv = input.uv;
   return output;
}
)";

const char* motionBlurPixelShaderSource = R"(
Texture2D currentFrame : register(t0);
Texture2D prevSingleFrame : register(t1);
Texture2D prevBlurredFrame : register(t2);
SamplerState textureSampler : register(s0);

cbuffer Params : register(b0)
{
    float mbRecall;
    float mbSoftness;
    float texelSizeX;
    float texelSizeY;
    float mbIntensity;
    float padding1;
    float padding2;
    float padding3;
};

struct PS_INPUT {
   float4 pos : SV_POSITION;
   float2 uv : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET {
    float4 curr = currentFrame.Sample(textureSampler, input.uv);
    float4 prevSingle = prevSingleFrame.Sample(textureSampler, input.uv);
    
    float2 diff = curr.rg - prevSingle.rg;
    float diffMag = length(diff);
    
    if (diffMag < 0.008) {
        return curr;
    }
    
    float2 motionDir = normalize(diff);
    float motionLen = saturate(diffMag * mbRecall * 26.0);
    
    int radius = (int)(motionLen * mbSoftness * 8.0);
    radius = clamp(radius, 3, 10);
    float sigma = float(radius) / 2.4;
    
    float4 result = curr;
    float totalWeight = 1.0;
    float2 pixel = float2(texelSizeX, texelSizeY);
    
    for (int i = -radius; i <= radius; i++) {
        float offsetAmount = float(i) * 0.7;
        float2 offset = motionDir * offsetAmount * pixel;
        float weight = exp(-(i * i) / (2.0 * sigma * sigma));
        
        float2 sampleUV = clamp(input.uv + offset, pixel, float2(1.0 - pixel.x, 1.0 - pixel.y));
        result += currentFrame.Sample(textureSampler, sampleUV) * weight;
        totalWeight += weight;
    }
    
    result /= totalWeight;
    return lerp(curr, result, motionLen * 0.85 * mbIntensity);
}
)";

const char* quadPixelShaderSource = R"(
Texture2D desktopTexture : register(t0);
SamplerState textureSampler : register(s0);
struct PS_INPUT {
   float4 pos : SV_POSITION;
   float2 uv : TEXCOORD;
};
float4 main(PS_INPUT input) : SV_TARGET {
   return desktopTexture.Sample(textureSampler, input.uv);
}
)";

const char* prevColorShaderSource = R"(
Texture2D currTex : register(t0);
Texture2D motionBlurResult : register(t1);
SamplerState textureSampler : register(s0);
struct PS_INPUT {
   float4 pos : SV_POSITION;
   float2 uv : TEXCOORD;
};
struct PS_OUTPUT {
   float4 prevSingle : SV_Target0;
   float4 prev : SV_Target1;
};
PS_OUTPUT main(PS_INPUT input) {
    PS_OUTPUT output;
    output.prevSingle = currTex.Sample(textureSampler, input.uv);
    output.prev = motionBlurResult.Sample(textureSampler, input.uv);
    return output;
}
)";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool InitializeWindow(int width, int height) {
    g_Application.hInstance = GetModuleHandle(nullptr);
    g_Application.windowWidth = width;
    g_Application.windowHeight = height;
    g_Application.accumInitialized = false;
    
    if (!g_classRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = g_Application.hInstance;
        wc.lpszClassName = L"MotionBlurWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        
        if (!RegisterClassW(&wc)) return false;
        g_classRegistered = true;
    }
    
    g_Application.hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        L"MotionBlurWindow",
        L"Motion Blur",
        WS_POPUP,
        0, 0, width, height,
        nullptr, nullptr, g_Application.hInstance, nullptr);
    
    if (!g_Application.hwnd) return false;
    
    SetLayeredWindowAttributes(g_Application.hwnd, 0, 255, LWA_ALPHA);
    return true;
}

bool InitializeDirectX() {
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };
    
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        featureLevels, ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION, &g_Application.device, nullptr, &g_Application.deviceContext);
    
    if (FAILED(hr)) return false;
    
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = g_Application.windowWidth;
    swapChainDesc.BufferDesc.Height = g_Application.windowHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = g_Application.hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    
    IDXGIDevice* dxgiDevice = nullptr;
    g_Application.device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    
    IDXGIAdapter* dxgiAdapter = nullptr;
    dxgiDevice->GetAdapter(&dxgiAdapter);
    
    IDXGIFactory* dxgiFactory = nullptr;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
    
    hr = dxgiFactory->CreateSwapChain(g_Application.device, &swapChainDesc, &g_Application.swapChain);
    
    dxgiFactory->Release();
    dxgiAdapter->Release();
    dxgiDevice->Release();
    
    if (FAILED(hr)) return false;
    
    ID3D11Texture2D* backBuffer = nullptr;
    g_Application.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    hr = g_Application.device->CreateRenderTargetView(backBuffer, nullptr, &g_Application.renderTargetView);
    if (backBuffer) backBuffer->Release();
    if (FAILED(hr)) return false;
    
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)g_Application.windowWidth;
    viewport.Height = (float)g_Application.windowHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    g_Application.deviceContext->RSSetViewports(1, &viewport);
    
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = g_Application.windowWidth;
    texDesc.Height = g_Application.windowHeight;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    
    hr = g_Application.device->CreateTexture2D(&texDesc, nullptr, &g_Application.currTex);
    if (FAILED(hr)) return false;
    hr = g_Application.device->CreateShaderResourceView(g_Application.currTex, nullptr, &g_Application.currSRV);
    if (FAILED(hr)) return false;
    hr = g_Application.device->CreateRenderTargetView(g_Application.currTex, nullptr, &g_Application.currRTV);
    if (FAILED(hr)) return false;
    
    hr = g_Application.device->CreateTexture2D(&texDesc, nullptr, &g_Application.prevSingleTex);
    if (FAILED(hr)) return false;
    hr = g_Application.device->CreateShaderResourceView(g_Application.prevSingleTex, nullptr, &g_Application.prevSingleSRV);
    if (FAILED(hr)) return false;
    hr = g_Application.device->CreateRenderTargetView(g_Application.prevSingleTex, nullptr, &g_Application.prevSingleRTV);
    if (FAILED(hr)) return false;
    
    hr = g_Application.device->CreateTexture2D(&texDesc, nullptr, &g_Application.prevTex);
    if (FAILED(hr)) return false;
    hr = g_Application.device->CreateShaderResourceView(g_Application.prevTex, nullptr, &g_Application.prevSRV);
    if (FAILED(hr)) return false;
    hr = g_Application.device->CreateRenderTargetView(g_Application.prevTex, nullptr, &g_Application.prevRTV);
    if (FAILED(hr)) return false;
    
    hr = g_Application.device->CreateTexture2D(&texDesc, nullptr, &g_Application.combineResultTex);
    if (FAILED(hr)) return false;
    hr = g_Application.device->CreateShaderResourceView(g_Application.combineResultTex, nullptr, &g_Application.combineResultSRV);
    if (FAILED(hr)) return false;
    hr = g_Application.device->CreateRenderTargetView(g_Application.combineResultTex, nullptr, &g_Application.combineResultRTV);
    if (FAILED(hr)) return false;
    
    return true;
}

bool EnsureCapture() {
    if (!g_isRobloxFullscreen) {
        return false;
    }
    bool needsReset = false;
    if (!WinRTCapture::IsCaptureInitialized()) {
        if (!WinRTCapture::InitializeCapture(g_Application.device, g_Application.deviceContext)) {
            return false;
        }
        needsReset = true;
    }
    int width = WinRTCapture::GetCaptureWidth();
    int height = WinRTCapture::GetCaptureHeight();
    if (width <= 0 || height <= 0) {
        g_Application.hasValidFrame.store(false);
        g_Application.waitingFirstFrame.store(true);
        g_forceFreshFrame = true;
        return false;
    }

    if (width != g_Application.windowWidth || height != g_Application.windowHeight) {
        D3D11_TEXTURE2D_DESC desktopDesc = {};
        desktopDesc.Width = width;
        desktopDesc.Height = height;
        desktopDesc.MipLevels = 1;
        desktopDesc.ArraySize = 1;
        desktopDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desktopDesc.SampleDesc.Count = 1;
        desktopDesc.Usage = D3D11_USAGE_DEFAULT;
        desktopDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

        if (g_Application.desktopTexture) {
            g_Application.desktopTexture->Release();
            g_Application.desktopTexture = nullptr;
        }
        if (g_Application.desktopSRV) {
            g_Application.desktopSRV->Release();
            g_Application.desktopSRV = nullptr;
        }

        HRESULT hr = g_Application.device->CreateTexture2D(&desktopDesc, nullptr, &g_Application.desktopTexture);
        if (FAILED(hr)) {
            return false;
        }
        hr = g_Application.device->CreateShaderResourceView(g_Application.desktopTexture, nullptr, &g_Application.desktopSRV);
        if (FAILED(hr)) {
            return false;
        }

        g_Application.windowWidth = width;
        g_Application.windowHeight = height;
        needsReset = true;
    }

    if (needsReset) {
        g_Application.waitingFirstFrame.store(true);
        g_forceFreshFrame = true;
        g_Application.hasValidFrame.store(false);
        g_Application.accumInitialized = false;
    }

    return true;
}

void ReleaseCaptureResources() {
    g_Application.hasValidFrame.store(false);
}

bool InitializeQuad() {
    QuadVertex vertices[] = {
        {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
        {-1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f, 0.0f}
    };
    
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;
    
    HRESULT hr = g_Application.device->CreateBuffer(&bufferDesc, &initData, &g_Application.quadVertexBuffer);
    if (FAILED(hr)) return false;
    
    ID3DBlob* vsBlob = nullptr;
    hr = D3DCompile(quadVertexShaderSource, strlen(quadVertexShaderSource), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, nullptr);
    if (FAILED(hr)) return false;
    
    hr = g_Application.device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_Application.quadVertexShader);
    if (FAILED(hr)) {
        vsBlob->Release();
        return false;
    }
    
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    
    hr = g_Application.device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_Application.quadInputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;
    
    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(motionBlurPixelShaderSource, strlen(motionBlurPixelShaderSource), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, nullptr);
    if (FAILED(hr)) return false;
    
    hr = g_Application.device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_Application.motionBlurPixelShader);
    psBlob->Release();
    if (FAILED(hr)) return false;
    
    ID3DBlob* psBlobCopy = nullptr;
    hr = D3DCompile(quadPixelShaderSource, strlen(quadPixelShaderSource), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlobCopy, nullptr);
    if (FAILED(hr)) return false;
    
    hr = g_Application.device->CreatePixelShader(psBlobCopy->GetBufferPointer(), psBlobCopy->GetBufferSize(), nullptr, &g_Application.quadPixelShader);
    psBlobCopy->Release();
    if (FAILED(hr)) return false;
    
    ID3DBlob* psBlobPrevColor = nullptr;
    hr = D3DCompile(prevColorShaderSource, strlen(prevColorShaderSource), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlobPrevColor, nullptr);
    if (FAILED(hr)) return false;
    
    hr = g_Application.device->CreatePixelShader(psBlobPrevColor->GetBufferPointer(), psBlobPrevColor->GetBufferSize(), nullptr, &g_Application.prevColorShader);
    psBlobPrevColor->Release();
    if (FAILED(hr)) return false;
    
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    
    hr = g_Application.device->CreateSamplerState(&samplerDesc, &g_Application.samplerState);
    if (FAILED(hr)) return false;
    
    struct CBParams { float mbRecall; float mbSoftness; float texelSizeX; float texelSizeY; float mbIntensity; float padding1; float padding2; float padding3; };
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.ByteWidth = sizeof(CBParams);
    
    hr = g_Application.device->CreateBuffer(&cbDesc, nullptr, &g_Application.cbParams);
    if (FAILED(hr)) return false;
    
    return true;
}

bool AcquireFrame(bool requireFresh) {
    if (!WinRTCapture::IsCaptureInitialized()) {
        return false;
    }

    int width = WinRTCapture::GetCaptureWidth();
    int height = WinRTCapture::GetCaptureHeight();
    if (width <= 0 || height <= 0) {
        g_Application.hasValidFrame.store(false);
        return false;
    }

    if (width != g_Application.windowWidth || height != g_Application.windowHeight || !g_Application.desktopTexture) {
        D3D11_TEXTURE2D_DESC desktopDesc = {};
        desktopDesc.Width = width;
        desktopDesc.Height = height;
        desktopDesc.MipLevels = 1;
        desktopDesc.ArraySize = 1;
        desktopDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desktopDesc.SampleDesc.Count = 1;
        desktopDesc.Usage = D3D11_USAGE_DEFAULT;
        desktopDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

        if (g_Application.desktopTexture) {
            g_Application.desktopTexture->Release();
            g_Application.desktopTexture = nullptr;
        }
        if (g_Application.desktopSRV) {
            g_Application.desktopSRV->Release();
            g_Application.desktopSRV = nullptr;
        }

        HRESULT hr = g_Application.device->CreateTexture2D(&desktopDesc, nullptr, &g_Application.desktopTexture);
        if (FAILED(hr)) {
            return false;
        }
        hr = g_Application.device->CreateShaderResourceView(g_Application.desktopTexture, nullptr, &g_Application.desktopSRV);
        if (FAILED(hr)) {
            return false;
        }

        g_Application.windowWidth = width;
        g_Application.windowHeight = height;
    }

    bool received = WinRTCapture::CopyFrameToTexture(g_Application.desktopTexture);
    if (received) {
        g_Application.hasValidFrame.store(true);
        g_forceFreshFrame = false;
        return true;
    }

    if (requireFresh) {
        g_Application.hasValidFrame.store(false);
        return false;
    }

    return g_Application.hasValidFrame.load();
}

void RenderFullScreenQuad() {
    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    g_Application.deviceContext->IASetVertexBuffers(0, 1, &g_Application.quadVertexBuffer, &stride, &offset);
    g_Application.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    g_Application.deviceContext->IASetInputLayout(g_Application.quadInputLayout);
    g_Application.deviceContext->VSSetShader(g_Application.quadVertexShader, nullptr, 0);
    g_Application.deviceContext->Draw(4, 0);
}

void Render() {
    bool shouldRender = false;
    HWND robloxWindow = WinRTCapture::FindRobloxWindow();
    
    if (robloxWindow && IsWindow(robloxWindow)) {
        RECT clientRect;
        if (GetClientRect(robloxWindow, &clientRect)) {
            int clientWidth = clientRect.right - clientRect.left;
            int clientHeight = clientRect.bottom - clientRect.top;
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            bool isFullscreen = (clientWidth == screenWidth && clientHeight == screenHeight);
            
            HWND foregroundWindow = GetForegroundWindow();
            bool robloxInFocus = (foregroundWindow == robloxWindow) || (GetParent(foregroundWindow) == robloxWindow);
            bool isGateCheckActive = ParryBar::IsGateCheckActive();
            
            g_isRobloxFullscreen = isFullscreen;
            
            if (isFullscreen && robloxInFocus && isGateCheckActive) {
                shouldRender = true;
            }
        }
    }
    
    if (!shouldRender) {
        ShowWindow(g_Application.hwnd, SW_HIDE);
        g_prevShouldRender = false;
        g_forceFreshFrame = true;
        ReleaseCaptureResources();
        WinRTCapture::ReleaseCapture();
        Sleep(16);
        return;
    }
    
    bool justBecameVisible = !g_prevShouldRender && shouldRender;
    g_prevShouldRender = shouldRender;
    
    if (justBecameVisible) {
        g_Application.accumInitialized = false;
        g_forceFreshFrame = true;
    }
    
    if (!EnsureCapture()) {
        ShowWindow(g_Application.hwnd, SW_HIDE);
        ReleaseCaptureResources();
        WinRTCapture::ReleaseCapture();
        return;
    }
    
    ShowWindow(g_Application.hwnd, SW_SHOW);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    
    bool requireFreshFrame = g_forceFreshFrame;
    bool grabbed = AcquireFrame(requireFreshFrame);
    if (grabbed) {
        g_forceFreshFrame = false;
    }
    if (!grabbed) {
        if (g_Application.waitingFirstFrame.load()) {
            g_Application.hasValidFrame.store(false);
            ShowWindow(g_Application.hwnd, SW_HIDE);
        }
        return;
    }
    
    if (g_Application.waitingFirstFrame.exchange(false)) {
        ShowWindow(g_Application.hwnd, SW_SHOW);
        EnsureOverlaysOnTop(g_Application.hwnd);
    }
    
    if (!g_Application.hasValidFrame.load()) {
        return;
    }
    
    if (!g_Application.accumInitialized) {
        g_Application.deviceContext->OMSetRenderTargets(1, &g_Application.currRTV, nullptr);
        g_Application.deviceContext->ClearRenderTargetView(g_Application.currRTV, clearColor);
        
        g_Application.deviceContext->OMSetRenderTargets(1, &g_Application.prevRTV, nullptr);
        g_Application.deviceContext->ClearRenderTargetView(g_Application.prevRTV, clearColor);
        
        g_Application.deviceContext->OMSetRenderTargets(1, &g_Application.prevSingleRTV, nullptr);
        g_Application.deviceContext->ClearRenderTargetView(g_Application.prevSingleRTV, clearColor);
        
        g_Application.deviceContext->OMSetRenderTargets(1, &g_Application.currRTV, nullptr);
        g_Application.deviceContext->ClearRenderTargetView(g_Application.currRTV, clearColor);
        
        g_Application.deviceContext->PSSetShader(g_Application.quadPixelShader, nullptr, 0);
        ID3D11ShaderResourceView* srvsInit[1] = { g_Application.desktopSRV };
        g_Application.deviceContext->PSSetShaderResources(0, 1, srvsInit);
        g_Application.deviceContext->PSSetSamplers(0, 1, &g_Application.samplerState);
        RenderFullScreenQuad();
        
        g_Application.accumInitialized = true;
    }
    
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(g_Application.deviceContext->Map(g_Application.cbParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            struct CBParams { float mbRecall; float mbSoftness; float texelSizeX; float texelSizeY; float mbIntensity; float padding1; float padding2; float padding3; };
            CBParams* cb = (CBParams*)mapped.pData;
            cb->mbRecall = 10.0f;
            cb->mbSoftness = 12.0f;
            cb->texelSizeX = 1.0f / g_Application.windowWidth;
            cb->texelSizeY = 1.0f / g_Application.windowHeight;
            cb->mbIntensity = g_mbIntensity;
            cb->padding1 = 0.0f;
            cb->padding2 = 0.0f;
            cb->padding3 = 0.0f;
            g_Application.deviceContext->Unmap(g_Application.cbParams, 0);
        }
    }
    
    g_Application.deviceContext->OMSetRenderTargets(1, &g_Application.combineResultRTV, nullptr);
    g_Application.deviceContext->ClearRenderTargetView(g_Application.combineResultRTV, clearColor);
    
    g_Application.deviceContext->PSSetShader(g_Application.motionBlurPixelShader, nullptr, 0);
    ID3D11ShaderResourceView* srvsA[3] = { g_Application.desktopSRV, g_Application.prevSingleSRV, g_Application.prevSRV };
    g_Application.deviceContext->PSSetShaderResources(0, 3, srvsA);
    g_Application.deviceContext->PSSetSamplers(0, 1, &g_Application.samplerState);
    g_Application.deviceContext->PSSetConstantBuffers(0, 1, &g_Application.cbParams);
    
    RenderFullScreenQuad();
    
    ID3D11ShaderResourceView* nullSRVs[3] = { nullptr, nullptr, nullptr };
    g_Application.deviceContext->PSSetShaderResources(0, 3, nullSRVs);
    
    ID3D11RenderTargetView* prevRTVs[2] = { g_Application.prevSingleRTV, g_Application.prevRTV };
    g_Application.deviceContext->OMSetRenderTargets(2, prevRTVs, nullptr);
    
    g_Application.deviceContext->PSSetShader(g_Application.prevColorShader, nullptr, 0);
    ID3D11ShaderResourceView* srvsB[2] = { g_Application.desktopSRV, g_Application.combineResultSRV };
    g_Application.deviceContext->PSSetShaderResources(0, 2, srvsB);
    g_Application.deviceContext->PSSetSamplers(0, 1, &g_Application.samplerState);
    
    RenderFullScreenQuad();
    
    ID3D11ShaderResourceView* nullSRV1[2] = { nullptr, nullptr };
    g_Application.deviceContext->PSSetShaderResources(0, 2, nullSRV1);
    
    g_Application.deviceContext->OMSetRenderTargets(1, &g_Application.renderTargetView, nullptr);
    g_Application.deviceContext->ClearRenderTargetView(g_Application.renderTargetView, clearColor);
    
    g_Application.deviceContext->PSSetShader(g_Application.quadPixelShader, nullptr, 0);
    ID3D11ShaderResourceView* srvsC[1] = { g_Application.combineResultSRV };
    g_Application.deviceContext->PSSetShaderResources(0, 1, srvsC);
    g_Application.deviceContext->PSSetSamplers(0, 1, &g_Application.samplerState);
    
    RenderFullScreenQuad();
    
    ID3D11ShaderResourceView* nullSRV2[1] = { nullptr };
    g_Application.deviceContext->PSSetShaderResources(0, 1, nullSRV2);
    
    g_Application.swapChain->Present(1, 0);
}

void Cleanup() {
    ReleaseCaptureResources();
    if (g_Application.renderTargetView) g_Application.renderTargetView->Release();
    if (g_Application.swapChain) g_Application.swapChain->Release();
    if (g_Application.desktopSRV) g_Application.desktopSRV->Release();
    if (g_Application.desktopTexture) g_Application.desktopTexture->Release();
    if (g_Application.currSRV) g_Application.currSRV->Release();
    if (g_Application.currRTV) g_Application.currRTV->Release();
    if (g_Application.currTex) g_Application.currTex->Release();
    if (g_Application.prevSingleSRV) g_Application.prevSingleSRV->Release();
    if (g_Application.prevSingleRTV) g_Application.prevSingleRTV->Release();
    if (g_Application.prevSingleTex) g_Application.prevSingleTex->Release();
    if (g_Application.prevSRV) g_Application.prevSRV->Release();
    if (g_Application.prevRTV) g_Application.prevRTV->Release();
    if (g_Application.prevTex) g_Application.prevTex->Release();
    if (g_Application.combineResultSRV) g_Application.combineResultSRV->Release();
    if (g_Application.combineResultRTV) g_Application.combineResultRTV->Release();
    if (g_Application.combineResultTex) g_Application.combineResultTex->Release();
    if (g_Application.motionBlurPixelShader) g_Application.motionBlurPixelShader->Release();
    if (g_Application.prevColorShader) g_Application.prevColorShader->Release();
    if (g_Application.quadPixelShader) g_Application.quadPixelShader->Release();
    if (g_Application.quadVertexShader) g_Application.quadVertexShader->Release();
    if (g_Application.quadInputLayout) g_Application.quadInputLayout->Release();
    if (g_Application.quadVertexBuffer) g_Application.quadVertexBuffer->Release();
    if (g_Application.samplerState) g_Application.samplerState->Release();
    if (g_Application.cbParams) g_Application.cbParams->Release();
    if (g_Application.deviceContext) g_Application.deviceContext->Release();
    if (g_Application.device) g_Application.device->Release();
    
    memset(&g_Application, 0, sizeof(g_Application));
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        g_Application.isRunning = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void StartMotionBlur() {
    if (g_running) return;
    
    if (g_thread.joinable()) {
        g_running = false;
        g_thread.join();
    }
    
    g_running = true;
    
    g_thread = std::thread([]() {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        
        if (!InitializeWindow(screenW, screenH)) {
            g_running = false;
            return;
        }
        if (!InitializeDirectX()) {
            Cleanup();
            g_running = false;
            return;
        }
        if (!InitializeQuad()) {
            Cleanup();
            g_running = false;
            return;
        }
        
        g_Application.isRunning = true;
        ShowWindow(g_Application.hwnd, SW_SHOW);
        
        MSG msg = {};
        while (g_Application.isRunning && g_running) {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (g_Application.isRunning && g_running) Render();
        }
        
        Cleanup();
    });
}

void StopMotionBlur() {
    g_running = false;
    g_Application.isRunning = false;
    if (g_thread.joinable()) {
        g_thread.join();
        g_thread = std::thread();
    }
    Cleanup();
}

bool IsMotionBlurRunning() {
    return g_running;
}

void SetMotionBlurIntensity(float intensity) {
    g_mbIntensity = intensity;
}

}
