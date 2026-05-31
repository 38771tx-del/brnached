#ifndef BLUR_LIBRARY_H
#define BLUR_LIBRARY_H

#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <atomic>
#include <algorithm>
#include <mutex>
#include <thread>
#include <chrono>
#include <cmath>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace ParryBar {
}

namespace WinRTCapture {
    bool InitializeCapture(ID3D11Device* device, ID3D11DeviceContext* context);
    bool IsCaptureInitialized();
    void ReleaseCapture();
    bool AcquireFrame(ID3D11Texture2D** outTexture, int* outWidth, int* outHeight, bool requireFresh);
    bool CopyFrameToTexture(ID3D11Texture2D* destTexture);
    int GetCaptureWidth();
    int GetCaptureHeight();
    HWND FindRobloxWindow();
}

struct BlurApp {
    std::atomic<float> targetIntensity{ 0.0f };
    std::atomic<float> currentIntensity{ 0.0f };
};

BlurApp g_blurApp;
bool g_animatingOut = false;
bool g_robloxFound = false;
bool g_isRobloxFullscreen = false;
float g_blurIterations = 5.0f;
float g_targetBlurIterations = 5.0f;
float g_currentBlurIterations = 5.0f;

bool IsRobloxFound() {
    g_robloxFound = WinRTCapture::FindRobloxWindow() != nullptr;
    return g_robloxFound;
}

void CleanupOverlay();
void StartBlurRendering();
void StopBlurRendering();
void HideOverlayWindow();

struct BlurConstants {
	UINT textureWidth;
	UINT textureHeight;
	float blurRadius;
	float padding;
};

const char* kQuadVS = R"(
struct VS_INPUT {
   float3 pos : POSITION;
   float2 uv : TEXCOORD;
};

struct VS_OUTPUT {
   float4 pos : SV_POSITION;
   float2 uv : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT o;
    o.pos = float4(input.pos, 1.f);
    o.uv = input.uv;
    return o;
}
)";

const char* kQuadPS = R"(
Texture2D sceneTexture : register(t0);
SamplerState sceneSampler : register(s0);

struct PS_INPUT {
   float4 pos : SV_POSITION;
   float2 uv : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET {
    return sceneTexture.Sample(sceneSampler, input.uv);
}
)";

const char* kHorizontalCS = R"(
cbuffer BlurConstants : register(b0) {
			uint textureWidth;
			uint textureHeight;
			float blurRadius;
			float padding;
		};

		Texture2D<float4> InputTexture : register(t0);
		RWTexture2D<float4> OutputTexture : register(u0);

		[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    if (id.x >= textureWidth || id.y >= textureHeight) return;

    int radius = (int)blurRadius;
    float sigma = max(radius / 3.0f, 1.0f);
			float4 color = float4(0, 0, 0, 0);
    float totalWeight = 0.0f;

    for (int offset = -radius; offset <= radius; ++offset) {
        int sampleX = clamp(int(id.x) + offset, 0, int(textureWidth) - 1);
        float weight = exp(-(offset * offset) / (2.0f * sigma * sigma));
        color += InputTexture[int2(sampleX, id.y)] * weight;
			totalWeight += weight;
		}

    OutputTexture[int2(id.x, id.y)] = (totalWeight > 0.0f) ? color / totalWeight : InputTexture[int2(id.x, id.y)];
}
)";

const char* kVerticalCS = R"(
cbuffer BlurConstants : register(b0) {
			uint textureWidth;
			uint textureHeight;
			float blurRadius;
			float padding;
		};

		Texture2D<float4> InputTexture : register(t0);
		RWTexture2D<float4> OutputTexture : register(u0);

		[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    if (id.x >= textureWidth || id.y >= textureHeight) return;

    int radius = (int)blurRadius;
    float sigma = max(radius / 3.0f, 1.0f);
    float4 color = float4(0, 0, 0, 0);
    float totalWeight = 0.0f;

    for (int offset = -radius; offset <= radius; ++offset) {
        int sampleY = clamp(int(id.y) + offset, 0, int(textureHeight) - 1);
        float weight = exp(-(offset * offset) / (2.0f * sigma * sigma));
        color += InputTexture[int2(id.x, sampleY)] * weight;
        totalWeight += weight;
    }

    OutputTexture[int2(id.x, id.y)] = (totalWeight > 0.0f) ? color / totalWeight : InputTexture[int2(id.x, id.y)];
}
)";

struct BlurResources {
    HWND overlayWindow = nullptr;
    HWND targetWindow = nullptr;

    UINT frameWidth = 0;
    UINT frameHeight = 0;

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11RenderTargetView> renderTarget;

    ComPtr<ID3D11Texture2D> captureTexture;
    ComPtr<ID3D11ShaderResourceView> captureSRV;

    ComPtr<ID3D11Texture2D> blurTexture;
    ComPtr<ID3D11Texture2D> tempTexture;
    ComPtr<ID3D11UnorderedAccessView> blurUAV;
    ComPtr<ID3D11UnorderedAccessView> tempUAV;
    ComPtr<ID3D11ShaderResourceView> blurSRV;
    ComPtr<ID3D11ShaderResourceView> tempSRV;

    ComPtr<ID3D11Texture2D> maskTexture;
    ComPtr<ID3D11ShaderResourceView> maskSRV;
    ComPtr<ID3D11RenderTargetView> maskRTV;

    ComPtr<ID3D11Buffer> constantBuffer;
    ComPtr<ID3D11Buffer> quadBuffer;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
    ComPtr<ID3D11ComputeShader> horizontalShader;
    ComPtr<ID3D11ComputeShader> verticalShader;
    ComPtr<ID3D11SamplerState> sampler;


    std::recursive_mutex mutex;
    std::atomic<bool> visible{ false };
    std::atomic<bool> waitingFirstFrame{ false };
    std::atomic<bool> windowVisible{ false };
    std::atomic<bool> hasValidFrame{ false };
};

struct QuadVertex {
    float x, y, z;
    float u, v;
};


BlurResources g_resources;
std::atomic<bool> g_running{ false };
std::thread g_renderThread;
std::atomic<bool> g_blurHidePending{ false };
std::atomic<bool> g_forceFreshBlurFrame{ true };

void ReleaseCaptureResources();

// Match old behavior: do not activate on click, treat as client area
static LRESULT CALLBACK BlurWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_MOUSEACTIVATE) {
		return MA_NOACTIVATE;
	}
	if (msg == WM_NCHITTEST) {
		return HTCLIENT;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool CreateOverlayWindow(HWND targetWindow) {
    g_resources.targetWindow = targetWindow;

    WNDCLASSW wc{};
	wc.lpfnWndProc = BlurWindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"BlurVisual";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    static bool registered = false;
    if (!registered) {
        if (!RegisterClassW(&wc)) {
        return false;
        }
        registered = true;
    }

    g_resources.overlayWindow = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"Blur",
		WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    if (!g_resources.overlayWindow) {
        return false;
    }

    SetLayeredWindowAttributes(g_resources.overlayWindow, 0, 255, LWA_ALPHA);
    ShowWindow(g_resources.overlayWindow, SW_HIDE);
    return true;
}

bool CreateDevice() {
    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL obtained = D3D_FEATURE_LEVEL_11_0;
	HRESULT hr = D3D11CreateDevice(
								   nullptr,
								   D3D_DRIVER_TYPE_HARDWARE,
								   nullptr,
								   flags,
								   featureLevels,
								   ARRAYSIZE(featureLevels),
								   D3D11_SDK_VERSION,
        &device,
        &obtained,
        &context);
    if (FAILED(hr)) {
		return false;
    }

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Width = GetSystemMetrics(SM_CXSCREEN);
    desc.BufferDesc.Height = GetSystemMetrics(SM_CYSCREEN);
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = g_resources.overlayWindow;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    ComPtr<IDXGIDevice> dxgiDevice;
    device.As(&dxgiDevice);

    ComPtr<IDXGIAdapter> adapter;
    dxgiDevice->GetAdapter(&adapter);

    ComPtr<IDXGIFactory> factory;
    adapter->GetParent(IID_PPV_ARGS(&factory));

    ComPtr<IDXGISwapChain> swapChain;
    hr = factory->CreateSwapChain(device.Get(), &desc, &swapChain);
    if (FAILED(hr)) {
		return false;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));

    ComPtr<ID3D11RenderTargetView> rtv;
    hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &rtv);
    if (FAILED(hr)) {
		return false;
    }

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<FLOAT>(desc.BufferDesc.Width);
    viewport.Height = static_cast<FLOAT>(desc.BufferDesc.Height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    g_resources.device = device;
    g_resources.context = context;
    g_resources.swapChain = swapChain;
    g_resources.renderTarget = rtv;

    g_resources.frameWidth = desc.BufferDesc.Width;
    g_resources.frameHeight = desc.BufferDesc.Height;

	return true;
}

bool CompileShaders() {
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompile(
        kQuadVS,
        strlen(kQuadVS),
        nullptr,
        nullptr,
        nullptr,
        "main",
        "vs_5_0",
        0,
        0,
        &vsBlob,
        &errorBlob);
    if (FAILED(hr)) {
        return false;
    }

    hr = g_resources.device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_resources.vertexShader);
    if (FAILED(hr)) {
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = g_resources.device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_resources.inputLayout);
    if (FAILED(hr)) {
        return false;
    }

    hr = D3DCompile(
        kQuadPS,
        strlen(kQuadPS),
        nullptr,
        nullptr,
        nullptr,
        "main",
        "ps_5_0",
        0,
        0,
        &psBlob,
        &errorBlob);
    if (FAILED(hr)) {
        return false;
    }
    
    hr = g_resources.device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_resources.pixelShader);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<ID3DBlob> csBlob;
    hr = D3DCompile(
        kHorizontalCS,
        strlen(kHorizontalCS),
        nullptr,
        nullptr,
        nullptr,
        "main",
        "cs_5_0",
        0,
        0,
        &csBlob,
        &errorBlob);
    if (FAILED(hr)) {
            return false;
        }
    hr = g_resources.device->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &g_resources.horizontalShader);
    if (FAILED(hr)) {
            return false;
        }
        
    hr = D3DCompile(
        kVerticalCS,
        strlen(kVerticalCS),
        nullptr,
        nullptr,
        nullptr,
        "main",
        "cs_5_0",
        0,
        0,
        &csBlob,
        &errorBlob);
    if (FAILED(hr)) {
        return false;
    }
    hr = g_resources.device->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &g_resources.verticalShader);
    if (FAILED(hr)) {
        return false;
}

	QuadVertex vertices[] = {
        { -1.f, -1.f, 0.f, 0.f, 1.f },
        { -1.f,  1.f, 0.f, 0.f, 0.f },
        {  1.f, -1.f, 0.f, 1.f, 1.f },
        {  1.f,  1.f, 0.f, 1.f, 0.f }
    };

    D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(vertices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = vertices;

    hr = g_resources.device->CreateBuffer(&bufferDesc, &init, &g_resources.quadBuffer);
	if (FAILED(hr)) {
		return false;
	}

    D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = g_resources.device->CreateSamplerState(&samplerDesc, &g_resources.sampler);
    if (FAILED(hr)) {
        return false;
    }

    D3D11_BUFFER_DESC constantDesc{};
    constantDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantDesc.ByteWidth = sizeof(BlurConstants);
    constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = g_resources.device->CreateBuffer(&constantDesc, nullptr, &g_resources.constantBuffer);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

bool CreateFrameTargets(UINT width, UINT height) {
    width = (std::max)(width, 64u);
    height = (std::max)(height, 64u);

    if (width == g_resources.frameWidth && height == g_resources.frameHeight &&
        g_resources.captureTexture) {
        return true;
    }

    g_resources.captureTexture.Reset();
    g_resources.captureSRV.Reset();
    g_resources.blurTexture.Reset();
    g_resources.tempTexture.Reset();
    g_resources.blurUAV.Reset();
    g_resources.tempUAV.Reset();
    g_resources.blurSRV.Reset();
    g_resources.tempSRV.Reset();
    g_resources.maskTexture.Reset();
    g_resources.maskSRV.Reset();
    g_resources.maskRTV.Reset();

    D3D11_TEXTURE2D_DESC captureDesc{};
    captureDesc.Width = width;
    captureDesc.Height = height;
    captureDesc.MipLevels = 1;
    captureDesc.ArraySize = 1;
    captureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    captureDesc.SampleDesc.Count = 1;
    captureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    HRESULT hr = g_resources.device->CreateTexture2D(&captureDesc, nullptr, &g_resources.captureTexture);
    if (FAILED(hr)) {
        return false;
    }

    hr = g_resources.device->CreateShaderResourceView(g_resources.captureTexture.Get(), nullptr, &g_resources.captureSRV);
    if (FAILED(hr)) {
        return false;
    }

    captureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;

    hr = g_resources.device->CreateTexture2D(&captureDesc, nullptr, &g_resources.blurTexture);
    if (FAILED(hr)) {
        return false;
    }

    hr = g_resources.device->CreateTexture2D(&captureDesc, nullptr, &g_resources.tempTexture);
    if (FAILED(hr)) {
        return false;
    }

    hr = g_resources.device->CreateUnorderedAccessView(g_resources.blurTexture.Get(), nullptr, &g_resources.blurUAV);
    if (FAILED(hr)) {
        return false;
    }

    hr = g_resources.device->CreateUnorderedAccessView(g_resources.tempTexture.Get(), nullptr, &g_resources.tempUAV);
    if (FAILED(hr)) {
        return false;
    }

    hr = g_resources.device->CreateShaderResourceView(g_resources.blurTexture.Get(), nullptr, &g_resources.blurSRV);
    if (FAILED(hr)) {
        return false;
    }

    hr = g_resources.device->CreateShaderResourceView(g_resources.tempTexture.Get(), nullptr, &g_resources.tempSRV);
    if (FAILED(hr)) {
        return false;
    }

    D3D11_TEXTURE2D_DESC maskDesc = captureDesc;
    maskDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    hr = g_resources.device->CreateTexture2D(&maskDesc, nullptr, &g_resources.maskTexture);
    if (FAILED(hr)) {
            return false;
        }

    hr = g_resources.device->CreateRenderTargetView(g_resources.maskTexture.Get(), nullptr, &g_resources.maskRTV);
    if (FAILED(hr)) {
        return false;
    }

    hr = g_resources.device->CreateShaderResourceView(g_resources.maskTexture.Get(), nullptr, &g_resources.maskSRV);
    if (FAILED(hr)) {
        return false;
    }

    const FLOAT maskClear[4] = { 1.f, 1.f, 1.f, 1.f };
    g_resources.context->ClearRenderTargetView(g_resources.maskRTV.Get(), maskClear);

    g_resources.frameWidth = width;
    g_resources.frameHeight = height;
    return true;
}

void CleanupOverlay() {
    HideOverlayWindow();
    ReleaseCaptureResources();

    g_resources.maskSRV.Reset();
    g_resources.maskRTV.Reset();
    g_resources.maskTexture.Reset();

    g_resources.blurSRV.Reset();
    g_resources.tempSRV.Reset();
    g_resources.blurUAV.Reset();
    g_resources.tempUAV.Reset();
    g_resources.blurTexture.Reset();
    g_resources.tempTexture.Reset();

    g_resources.captureSRV.Reset();
    g_resources.captureTexture.Reset();

    g_resources.constantBuffer.Reset();
    g_resources.quadBuffer.Reset();
    g_resources.inputLayout.Reset();
    g_resources.vertexShader.Reset();
    g_resources.pixelShader.Reset();
    g_resources.horizontalShader.Reset();
    g_resources.verticalShader.Reset();
    g_resources.sampler.Reset();

    g_resources.renderTarget.Reset();
    g_resources.swapChain.Reset();
    g_resources.context.Reset();
    g_resources.device.Reset();

    if (g_resources.overlayWindow) {
        DestroyWindow(g_resources.overlayWindow);
        g_resources.overlayWindow = nullptr;
    }
    g_resources.windowVisible.store(false);
}

void ReleaseCaptureResources() {
    g_resources.hasValidFrame.store(false);
}

bool EnsureCapture() {
    std::lock_guard<std::recursive_mutex> lock(g_resources.mutex);
    if (!g_resources.visible.load(std::memory_order_relaxed)) {
        return false;
    }
    if (!g_isRobloxFullscreen) {
        return false;
    }
    if (WinRTCapture::IsCaptureInitialized()) {
        return true;
    }

    if (!WinRTCapture::InitializeCapture(g_resources.device.Get(), g_resources.context.Get())) {
        return false;
    }

    int captureWidth = WinRTCapture::GetCaptureWidth();
    int captureHeight = WinRTCapture::GetCaptureHeight();
    UINT width = (std::max)(static_cast<UINT>(captureWidth), 64u);
    UINT height = (std::max)(static_cast<UINT>(captureHeight), 64u);

    if (!CreateFrameTargets(width, height)) {
        return false;
    }

    g_resources.waitingFirstFrame.store(true);
    g_forceFreshBlurFrame.store(true);
    g_resources.hasValidFrame.store(false);

    return true;
}

bool AcquireFrame(bool requireFresh) {
    if (!WinRTCapture::IsCaptureInitialized()) {
        return false;
    }

    int width = WinRTCapture::GetCaptureWidth();
    int height = WinRTCapture::GetCaptureHeight();
    if (width <= 0 || height <= 0) {
        g_resources.hasValidFrame.store(false);
        return false;
    }
    
    bool updated = false;
        if (width != g_resources.frameWidth || height != g_resources.frameHeight || !g_resources.captureTexture) {
            if (!CreateFrameTargets(width, height)) {
            return false;
            }
            updated = true;
        }

    bool received = WinRTCapture::CopyFrameToTexture(g_resources.captureTexture.Get());

    if (received) {
        g_resources.hasValidFrame.store(true);
        g_forceFreshBlurFrame.store(false);
        if (updated) {
            const FLOAT maskClear[4] = { 1.f, 1.f, 1.f, 1.f };
            g_resources.context->ClearRenderTargetView(g_resources.maskRTV.Get(), maskClear);
        }
        return true;
    }

    if (requireFresh) {
        g_resources.hasValidFrame.store(false);
        return false;
    }

    return g_resources.hasValidFrame.load();
}

void ApplyBlur(float blurIntensity) {
    if (!g_resources.captureTexture || !g_resources.blurTexture || !g_resources.tempTexture) {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    HRESULT hr = g_resources.context->Map(g_resources.constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr)) {
        auto* constants = reinterpret_cast<BlurConstants*>(mapped.pData);
        constants->textureWidth = g_resources.frameWidth;
        constants->textureHeight = g_resources.frameHeight;
        constants->blurRadius = blurIntensity * 60.0f;
        constants->padding = 0.0f;
        g_resources.context->Unmap(g_resources.constantBuffer.Get(), 0);
    }

    ID3D11ShaderResourceView* horizontalSRV[] = { g_resources.captureSRV.Get() };
    g_resources.context->CSSetShader(g_resources.horizontalShader.Get(), nullptr, 0);
    g_resources.context->CSSetShaderResources(0, 1, horizontalSRV);
    g_resources.context->CSSetUnorderedAccessViews(0, 1, g_resources.tempUAV.GetAddressOf(), nullptr);
    g_resources.context->CSSetConstantBuffers(0, 1, g_resources.constantBuffer.GetAddressOf());

    UINT groupX = (g_resources.frameWidth + 7) / 8;
    UINT groupY = (g_resources.frameHeight + 7) / 8;
    g_resources.context->Dispatch(groupX, groupY, 1);

    ID3D11UnorderedAccessView* nullUAV = nullptr;
    ID3D11ShaderResourceView* nullSRV = nullptr;
    g_resources.context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
    g_resources.context->CSSetShaderResources(0, 1, &nullSRV);
    g_resources.context->CSSetShader(nullptr, nullptr, 0);

    ID3D11ShaderResourceView* verticalSRV[] = { g_resources.tempSRV.Get() };
    g_resources.context->CSSetShader(g_resources.verticalShader.Get(), nullptr, 0);
    g_resources.context->CSSetShaderResources(0, 1, verticalSRV);
    g_resources.context->CSSetUnorderedAccessViews(0, 1, g_resources.blurUAV.GetAddressOf(), nullptr);
    g_resources.context->CSSetConstantBuffers(0, 1, g_resources.constantBuffer.GetAddressOf());
    g_resources.context->Dispatch(groupX, groupY, 1);
    g_resources.context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
    g_resources.context->CSSetShaderResources(0, 1, &nullSRV);
    g_resources.context->CSSetShader(nullptr, nullptr, 0);
}

void RenderBlurredQuad() {
    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    ID3D11Buffer* vertexBuffers[] = { g_resources.quadBuffer.Get() };
    g_resources.context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
    g_resources.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    g_resources.context->IASetInputLayout(g_resources.inputLayout.Get());
    g_resources.context->VSSetShader(g_resources.vertexShader.Get(), nullptr, 0);
    g_resources.context->PSSetShader(g_resources.pixelShader.Get(), nullptr, 0);
    g_resources.context->PSSetSamplers(0, 1, g_resources.sampler.GetAddressOf());
    ID3D11ShaderResourceView* srvs[] = { g_resources.blurSRV.Get() };
    g_resources.context->PSSetShaderResources(0, 1, srvs);
    g_resources.context->Draw(4, 0);
    ID3D11ShaderResourceView* nullSRV = nullptr;
    g_resources.context->PSSetShaderResources(0, 1, &nullSRV);
}

void ShowOverlayWindow() {
    if (!g_resources.overlayWindow || !g_resources.targetWindow) {
        return;
    }
    SetWindowPos(g_resources.overlayWindow, g_resources.targetWindow, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(g_resources.overlayWindow, SW_SHOW);
    SetWindowPos(g_resources.targetWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    g_resources.windowVisible.store(true);
}

void HideOverlayWindow() {
    if (!g_resources.overlayWindow) {
        return;
    }
    ShowWindow(g_resources.overlayWindow, SW_HIDE);
    g_resources.windowVisible.store(false);
}

void ClearFrameTextures() {
    std::lock_guard<std::recursive_mutex> lock(g_resources.mutex);
    if (!g_resources.context || !g_resources.device) return;

    auto clearTexture = [&](ID3D11Texture2D* texture) {
        if (!texture) return;
        ComPtr<ID3D11RenderTargetView> rtv;
        if (SUCCEEDED(g_resources.device->CreateRenderTargetView(texture, nullptr, &rtv))) {
            const FLOAT clear[4] = { 0, 0, 0, 0 };
            g_resources.context->ClearRenderTargetView(rtv.Get(), clear);
        } else {
            ComPtr<ID3D11UnorderedAccessView> uav;
            if (SUCCEEDED(g_resources.device->CreateUnorderedAccessView(texture, nullptr, &uav))) {
                const FLOAT clear[4] = { 0, 0, 0, 0 };
                g_resources.context->ClearUnorderedAccessViewFloat(uav.Get(), clear);
            }
        }
    };

    clearTexture(g_resources.captureTexture.Get());
    clearTexture(g_resources.blurTexture.Get());
    clearTexture(g_resources.tempTexture.Get());
    g_resources.hasValidFrame.store(false);
}

void RenderFrame(bool requireFresh) {
    std::lock_guard<std::recursive_mutex> lock(g_resources.mutex);
    if (!g_resources.visible.load()) {
        return;
    }
    if (!EnsureCapture()) {
        g_resources.hasValidFrame.store(false);
        g_resources.waitingFirstFrame.store(true);
        g_forceFreshBlurFrame.store(true);
        HideOverlayWindow();
        ClearFrameTextures();
        return;
    }

    bool haveFrame = AcquireFrame(requireFresh);
    if (!haveFrame) {
        if (g_resources.waitingFirstFrame.load()) {
            g_resources.hasValidFrame.store(false);
            HideOverlayWindow();
            ClearFrameTextures();
        }
        return;
    }

    if (g_resources.waitingFirstFrame.exchange(false)) {
        ShowOverlayWindow();
    }

    if (!g_resources.hasValidFrame.load()) {
                return;
    }

    FLOAT clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
    g_resources.context->ClearRenderTargetView(g_resources.renderTarget.Get(), clearColor);
    g_resources.context->ClearRenderTargetView(g_resources.maskRTV.Get(), clearColor);

    float blurIntensity = g_blurApp.currentIntensity.load();
    ApplyBlur(blurIntensity);
    g_resources.context->OMSetRenderTargets(1, g_resources.renderTarget.GetAddressOf(), nullptr);
    RenderBlurredQuad();
    g_resources.swapChain->Present(1, 0);
}

void BlurThreadProc() {
    MSG msg{};
    auto lastTime = std::chrono::steady_clock::now();

    while (g_running.load()) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        dt = std::clamp(dt, 0.0f, 0.033f);
                lastTime = now;

        float current = g_blurApp.currentIntensity.load();
        float target = g_blurApp.targetIntensity.load();
        if (target <= 0.0f && current <= 0.001f) {
            target = 0.001f;
        }

        bool waitingFirst = g_resources.waitingFirstFrame.load();
        bool windowVisible = g_resources.windowVisible.load();

        if (!(target > current && (waitingFirst || !windowVisible))) {
            float tauIn = 0.10f;
            float tauOut = 0.06f;
            float tau = (target > current) ? tauIn : tauOut;
            float alpha = 1.0f - expf(-dt / tau);
            current += (target - current) * alpha;
            if (fabsf(current - target) < 0.001f) current = target;
            g_blurApp.currentIntensity.store(current);
        }
                
                float currentIter = g_currentBlurIterations;
                float targetIter = g_targetBlurIterations;
        float iterTau = (targetIter > currentIter) ? 0.14f : 0.08f;
        float iterAlpha = 1.0f - expf(-dt / iterTau);
                currentIter += (targetIter - currentIter) * iterAlpha;
        if (fabsf(currentIter - targetIter) < 0.001f) currentIter = targetIter;
                g_currentBlurIterations = currentIter;
                g_blurIterations = currentIter;
                
        bool requireFresh = g_forceFreshBlurFrame.load();
        if (g_resources.visible.load()) {
            RenderFrame(requireFresh);
        } else {
            HideOverlayWindow();
            ReleaseCaptureResources();
        }

        if (g_blurHidePending.load() && g_resources.windowVisible.load() && g_blurApp.currentIntensity.load() <= 0.01f) {
            HideOverlayWindow();
            g_blurHidePending.store(false);
            CleanupOverlay();
            g_resources.visible.store(false);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

void InitializeBlurLibrary(HWND targetWindow) {
    g_resources.targetWindow = targetWindow;
}

void SetBlurIntensity(float intensity) {
    intensity = std::clamp(intensity, 0.0f, 1.0f);
    g_blurApp.targetIntensity.store(intensity);
}

void SetBlurIterations(float iterations) {
    iterations = std::clamp(iterations, 0.0f, 10.0f);
    g_targetBlurIterations = iterations;
}

void SetBlurVisible(bool visible) {
    if (visible) {
        g_blurHidePending.store(false);
        if (!g_resources.overlayWindow) {
            if (!CreateOverlayWindow(g_resources.targetWindow)) return;
            if (!CreateDevice()) { CleanupOverlay(); return; }
            if (!CompileShaders()) { CleanupOverlay(); return; }
            CreateFrameTargets(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        }
        g_resources.visible.store(true);
        g_resources.waitingFirstFrame.store(true);
        g_forceFreshBlurFrame.store(true);
        ClearFrameTextures();
        if (!g_running.load()) {
            StartBlurRendering();
        }
    } else {
        g_blurHidePending.store(true);
        g_blurApp.targetIntensity.store(0.0f);
    }
}

void SetRobloxFullscreen(bool isFullscreen) {
    g_isRobloxFullscreen = isFullscreen;
    if (!isFullscreen) {
        WinRTCapture::ReleaseCapture();
        SetBlurVisible(false);
    }
}

void StartBlurRendering() {
    if (g_running.load()) {
        return;
    }
    g_running.store(true);
    g_renderThread = std::thread(BlurThreadProc);
}

void StopBlurRendering() {
    g_running.store(false);
    if (g_renderThread.joinable()) {
        g_renderThread.join();
    }
    HideOverlayWindow();
    ReleaseCaptureResources();
}

#endif // BLUR_LIBRARY_H

