#include "Dx11App.h"
#include <dxgi.h>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <cstdint>

#define DDS_MAGIC 0x20534444

using namespace DirectX;

struct DDS_PIXELFORMAT {
    uint32_t dwSize; uint32_t dwFlags; uint32_t dwFourCC; uint32_t dwRGBBitCount;
    uint32_t dwRBitMask; uint32_t dwGBitMask; uint32_t dwBBitMask; uint32_t dwABitMask;
};

struct DDS_HEADER {
    uint32_t dwSize; uint32_t dwFlags; uint32_t dwHeight; uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize; uint32_t dwDepth; uint32_t dwMipMapCount;
    uint32_t dwReserved1[11]; DDS_PIXELFORMAT ddspf;
    uint32_t dwCaps; uint32_t dwCaps2; uint32_t dwCaps3; uint32_t dwCaps4; uint32_t dwReserved2;
};

UINT32 Dx11App::DivUp(UINT32 a, UINT32 b) { return (a + b - 1) / b; }

UINT32 Dx11App::GetBytesPerBlock(DXGI_FORMAT fmt) {
    if (fmt == DXGI_FORMAT_BC1_UNORM) return 8;
    if (fmt == DXGI_FORMAT_BC2_UNORM || fmt == DXGI_FORMAT_BC3_UNORM) return 16;
    return 4;
}

bool Dx11App::LoadDDS(const wchar_t* filename, TextureDesc& desc, bool isCubemap) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false; // Файл не найден

    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != DDS_MAGIC) return false;

    DDS_HEADER header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    desc.width = header.dwWidth;
    desc.height = header.dwHeight;
    desc.mipmapsCount = header.dwMipMapCount == 0 ? 1 : header.dwMipMapCount;

    if (header.ddspf.dwFourCC == 0x31545844) desc.fmt = DXGI_FORMAT_BC1_UNORM;
    else if (header.ddspf.dwFourCC == 0x33545844) desc.fmt = DXGI_FORMAT_BC2_UNORM;
    else if (header.ddspf.dwFourCC == 0x35545844) desc.fmt = DXGI_FORMAT_BC3_UNORM;
    else desc.fmt = DXGI_FORMAT_R8G8B8A8_UNORM;

    size_t dataSize = fileSize - sizeof(magic) - sizeof(header);
    char* data = new char[dataSize];
    file.read(data, dataSize);
    desc.pData = data;

    return true;
}

bool Dx11App::Init(HWND hwnd, int width, int height) {
    m_width = width; m_height = height;
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2; sd.BufferDesc.Width = width; sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, m_swapChain.GetAddressOf(), m_device.GetAddressOf(), nullptr, m_context.GetAddressOf())))
        return false;

    CreateRenderTarget(); OnResize(width, height);

    if (!InitCube()) return false;
    if (!InitSkybox()) return false;
    if (!LoadTextures()) return false;

    return true;
}

void Dx11App::CreateRenderTarget() {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_rtv.GetAddressOf());
}

void Dx11App::ReleaseRenderTarget() {
    m_rtv.Reset(); m_dsv.Reset(); m_depth.Reset();
}

void Dx11App::OnResize(int width, int height) {
    if (!m_swapChain || width == 0 || height == 0) return;
    m_width = width; m_height = height;
    ReleaseRenderTarget();
    m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = width; depthDesc.Height = height; depthDesc.MipLevels = 1; depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; depthDesc.SampleDesc.Count = 1; depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    m_device->CreateTexture2D(&depthDesc, nullptr, m_depth.GetAddressOf());
    m_device->CreateDepthStencilView(m_depth.Get(), nullptr, m_dsv.GetAddressOf());

    m_viewport = { 0,0,(float)width,(float)height,0,1 };
    m_context->RSSetViewports(1, &m_viewport);
}

void Dx11App::Render() {
    static auto start = std::chrono::high_resolution_clock::now();
    m_time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count();
    UpdateMatrices(m_time);

    float clear[4] = { 0.1f, 0.1f, 0.2f, 1 };
    m_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), m_dsv.Get());
    m_context->ClearRenderTargetView(m_rtv.Get(), clear);
    m_context->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH, 1, 0);

    ID3D11Buffer* cb[] = { m_geomBuffer.Get(), m_sceneBuffer.Get() };
    m_context->VSSetConstantBuffers(0, 2, cb);
    ID3D11SamplerState* samplers[] = { m_samplerState.Get() };
    m_context->PSSetSamplers(0, 1, samplers);

    UINT strideSkybox = sizeof(SkyboxVertex), offset = 0;
    m_context->RSSetState(m_skyboxRSState.Get());
    m_context->OMSetDepthStencilState(m_skyboxDSState.Get(), 0);
    m_context->IASetVertexBuffers(0, 1, m_skyboxVB.GetAddressOf(), &strideSkybox, &offset);
    m_context->IASetIndexBuffer(m_skyboxIB.Get(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetInputLayout(m_skyboxLayout.Get());
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_skyboxVS.Get(), nullptr, 0);
    m_context->PSSetShader(m_skyboxPS.Get(), nullptr, 0);
    ID3D11ShaderResourceView* skyboxRes[] = { m_skyboxTextureView.Get() };
    m_context->PSSetShaderResources(0, 1, skyboxRes);
    m_context->DrawIndexed(m_skyboxIndexCount, 0, 0);

    m_context->RSSetState(nullptr);
    m_context->OMSetDepthStencilState(nullptr, 0);

    UINT strideCube = sizeof(Vertex);
    m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &strideCube, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetInputLayout(m_inputLayout.Get());
    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    ID3D11ShaderResourceView* cubeRes[] = { m_cubeTextureView.Get() };
    m_context->PSSetShaderResources(0, 1, cubeRes);
    m_context->DrawIndexed(36, 0, 0);

    m_swapChain->Present(1, 0);
}

void Dx11App::OnMouseMove(int dx, int dy) {
    m_camYaw += dx * 0.005f; m_camPitch += dy * 0.005f;
}

bool Dx11App::InitCube() {
    Vertex v[] = {
        {-1.0f, -1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, -1.0f, -1.0f, 1.0f, 1.0f}, {1.0f,  1.0f, -1.0f, 1.0f, 0.0f}, {-1.0f,  1.0f, -1.0f, 0.0f, 0.0f},
        {-1.0f, -1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, -1.0f, 1.0f, 0.0f, 1.0f}, {1.0f,  1.0f, 1.0f, 0.0f, 0.0f}, {-1.0f,  1.0f, 1.0f, 1.0f, 0.0f},
        {-1.0f, 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 1.0f, 0.0f, 0.0f},
        {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f}, {1.0f, -1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, -1.0f, 1.0f, 0.0f, 0.0f}, {-1.0f, -1.0f, 1.0f, 1.0f, 0.0f},
        {-1.0f, -1.0f, 1.0f, 0.0f, 1.0f}, {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f}, {-1.0f,  1.0f, -1.0f, 1.0f, 0.0f}, {-1.0f,  1.0f, 1.0f, 0.0f, 0.0f},
        {1.0f, -1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, -1.0f, 1.0f, 1.0f, 1.0f}, {1.0f,  1.0f, 1.0f, 1.0f, 0.0f}, {1.0f,  1.0f, -1.0f, 0.0f, 0.0f},
    };
    USHORT i[] = { 0,1,2, 0,2,3, 4,6,5, 4,7,6, 8,9,10, 8,10,11, 12,14,13, 12,15,14, 16,17,18, 16,18,19, 20,21,22, 20,22,23 };

    D3D11_BUFFER_DESC bd{}; bd.Usage = D3D11_USAGE_IMMUTABLE; bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.ByteWidth = sizeof(v);
    D3D11_SUBRESOURCE_DATA sd{}; sd.pSysMem = v; m_device->CreateBuffer(&bd, &sd, m_vertexBuffer.GetAddressOf());

    bd.ByteWidth = sizeof(i); bd.BindFlags = D3D11_BIND_INDEX_BUFFER; sd.pSysMem = i; m_device->CreateBuffer(&bd, &sd, m_indexBuffer.GetAddressOf());

    Microsoft::WRL::ComPtr<ID3DBlob> vs, ps, err;
    if (FAILED(D3DCompileFromFile(L"cube.vs.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, vs.GetAddressOf(), err.GetAddressOf()))) {
        MessageBoxW(nullptr, L"Не удалось скомпилировать cube.vs.hlsl! Проверьте, что файл лежит рядом с .vcxproj", L"Ошибка", MB_OK); return false;
    }
    if (FAILED(D3DCompileFromFile(L"cube.ps.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, ps.GetAddressOf(), err.GetAddressOf()))) {
        MessageBoxW(nullptr, L"Не удалось скомпилировать cube.ps.hlsl!", L"Ошибка", MB_OK); return false;
    }

    m_device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
    m_device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    m_device->CreateInputLayout(layout, 2, vs->GetBufferPointer(), vs->GetBufferSize(), m_inputLayout.GetAddressOf());

    bd = {}; bd.Usage = D3D11_USAGE_DEFAULT; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; bd.ByteWidth = sizeof(GeomBuffer); m_device->CreateBuffer(&bd, nullptr, m_geomBuffer.GetAddressOf());
    bd.Usage = D3D11_USAGE_DYNAMIC; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; bd.ByteWidth = sizeof(SceneBuffer); m_device->CreateBuffer(&bd, nullptr, m_sceneBuffer.GetAddressOf());

    return true;
}

void Dx11App::GenerateSphere(int latLines, int longLines, std::vector<SkyboxVertex>& vertices, std::vector<USHORT>& indices) {
    for (int i = 0; i <= latLines; i++) {
        float spherePitch = XM_PI / 2.0f - XM_PI * ((float)i / (float)latLines);
        for (int j = 0; j <= longLines; j++) {
            float sphereYaw = XM_2PI * ((float)j / (float)longLines);
            vertices.push_back({ cosf(spherePitch) * cosf(sphereYaw), sinf(spherePitch), cosf(spherePitch) * sinf(sphereYaw) });
        }
    }
    for (int i = 0; i < latLines; i++) {
        for (int j = 0; j < longLines; j++) {
            indices.push_back(i * (longLines + 1) + j); indices.push_back((i + 1) * (longLines + 1) + j); indices.push_back((i + 1) * (longLines + 1) + j + 1);
            indices.push_back(i * (longLines + 1) + j); indices.push_back((i + 1) * (longLines + 1) + j + 1); indices.push_back(i * (longLines + 1) + j + 1);
        }
    }
}

bool Dx11App::InitSkybox() {
    std::vector<SkyboxVertex> vertices; std::vector<USHORT> indices; GenerateSphere(20, 20, vertices, indices); m_skyboxIndexCount = indices.size();

    D3D11_BUFFER_DESC bd{}; bd.Usage = D3D11_USAGE_IMMUTABLE; bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.ByteWidth = sizeof(SkyboxVertex) * vertices.size();
    D3D11_SUBRESOURCE_DATA sd{}; sd.pSysMem = vertices.data(); m_device->CreateBuffer(&bd, &sd, m_skyboxVB.GetAddressOf());
    bd.ByteWidth = sizeof(USHORT) * indices.size(); bd.BindFlags = D3D11_BIND_INDEX_BUFFER; sd.pSysMem = indices.data(); m_device->CreateBuffer(&bd, &sd, m_skyboxIB.GetAddressOf());

    Microsoft::WRL::ComPtr<ID3DBlob> vs, ps, err;
    if (FAILED(D3DCompileFromFile(L"skybox.vs.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, vs.GetAddressOf(), err.GetAddressOf()))) {
        MessageBoxW(nullptr, L"Не удалось скомпилировать skybox.vs.hlsl!", L"Ошибка", MB_OK); return false;
    }
    if (FAILED(D3DCompileFromFile(L"skybox.ps.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, ps.GetAddressOf(), err.GetAddressOf()))) {
        MessageBoxW(nullptr, L"Не удалось скомпилировать skybox.ps.hlsl!", L"Ошибка", MB_OK); return false;
    }

    m_device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, m_skyboxVS.GetAddressOf());
    m_device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, m_skyboxPS.GetAddressOf());

    D3D11_INPUT_ELEMENT_DESC layout[] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0} };
    m_device->CreateInputLayout(layout, 1, vs->GetBufferPointer(), vs->GetBufferSize(), m_skyboxLayout.GetAddressOf());

    D3D11_DEPTH_STENCIL_DESC dssDesc{}; dssDesc.DepthEnable = true; dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    m_device->CreateDepthStencilState(&dssDesc, m_skyboxDSState.GetAddressOf());

    D3D11_RASTERIZER_DESC rsDesc = {}; rsDesc.CullMode = D3D11_CULL_NONE; rsDesc.FillMode = D3D11_FILL_SOLID;
    m_device->CreateRasterizerState(&rsDesc, m_skyboxRSState.GetAddressOf());

    return true;
}

bool Dx11App::LoadTextures() {
    // --- КУБ ---
    TextureDesc texDesc;
    if (!LoadDDS(L"Pug.dds", texDesc, false)) {
        MessageBoxW(nullptr, L"Файл Pug.dds не найден в папке проекта!", L"Ошибка текстуры", MB_OK | MB_ICONERROR);
        return false;
    }
    D3D11_TEXTURE2D_DESC desc = {}; desc.Format = texDesc.fmt; desc.ArraySize = 1; desc.MipLevels = texDesc.mipmapsCount; desc.Usage = D3D11_USAGE_IMMUTABLE; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; desc.SampleDesc.Count = 1; desc.Height = texDesc.height; desc.Width = texDesc.width;
    UINT32 pitch = DivUp(desc.Width, 4u) * GetBytesPerBlock(desc.Format);

    std::vector<D3D11_SUBRESOURCE_DATA> data(desc.MipLevels); const char* pSrcData = (const char*)texDesc.pData;
    UINT32 bw = DivUp(desc.Width, 4u), bh = DivUp(desc.Height, 4u);
    for (UINT32 i = 0; i < desc.MipLevels; i++) {
        data[i].pSysMem = pSrcData; data[i].SysMemPitch = pitch; pSrcData += pitch * bh;
        bh = (std::max)(1u, bh / 2); bw = (std::max)(1u, bw / 2); pitch = bw * GetBytesPerBlock(desc.Format);
    }
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture; m_device->CreateTexture2D(&desc, data.data(), pTexture.GetAddressOf());
    delete[](char*)texDesc.pData;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {}; srvDesc.Format = texDesc.fmt; srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; srvDesc.Texture2D.MipLevels = desc.MipLevels;
    m_device->CreateShaderResourceView(pTexture.Get(), &srvDesc, m_cubeTextureView.GetAddressOf());

    // --- СКАЙБОКС ---
    const wchar_t* CubemapNames[6] = { 
        L"right.dds",   // +X
    L"left.dds",    // -X
    L"top.dds",     // +Y
    L"bottom.dds",  // -Y
    L"front.dds",   // +Z
    L"back.dds"     // -Z 
    };
    TextureDesc cubeDescs[6];
    for (int i = 0; i < 6; i++) {
        if (!LoadDDS(CubemapNames[i], cubeDescs[i], true)) {
            MessageBoxW(nullptr, L"Не найдены файлы скайбокса (posx.dds и т.д.)!", L"Ошибка текстуры", MB_OK | MB_ICONERROR);
            return false;
        }
    }
    desc.Format = cubeDescs[0].fmt; desc.ArraySize = 6; desc.MipLevels = 1; desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; desc.Height = cubeDescs[0].height; desc.Width = cubeDescs[0].width;
    pitch = DivUp(desc.Width, 4u) * GetBytesPerBlock(desc.Format);
    D3D11_SUBRESOURCE_DATA dataCube[6];
    for (int i = 0; i < 6; i++) { dataCube[i].pSysMem = cubeDescs[i].pData; dataCube[i].SysMemPitch = pitch; }
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pCubemapTex; m_device->CreateTexture2D(&desc, dataCube, pCubemapTex.GetAddressOf());
    for (int i = 0; i < 6; i++) delete[](char*)cubeDescs[i].pData;

    srvDesc.Format = desc.Format; srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; srvDesc.TextureCube.MipLevels = 1;
    m_device->CreateShaderResourceView(pCubemapTex.Get(), &srvDesc, m_skyboxTextureView.GetAddressOf());

    D3D11_SAMPLER_DESC sampDesc = {}; sampDesc.Filter = D3D11_FILTER_ANISOTROPIC; sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; sampDesc.MaxAnisotropy = 16; sampDesc.MaxLOD = FLT_MAX;
    m_device->CreateSamplerState(&sampDesc, m_samplerState.GetAddressOf());
    return true;
}

void Dx11App::UpdateMatrices(float t) {
    XMMATRIX m = XMMatrixRotationY(t);
    XMMATRIX camRot = XMMatrixRotationRollPitchYaw(m_camPitch, m_camYaw, 0);
    m_cameraPos = XMVector3TransformCoord(XMVectorSet(0, 0, -5, 1), camRot);
    XMMATRIX v = XMMatrixLookAtLH(m_cameraPos, XMVectorZero(), XMVectorSet(0, 1, 0, 0));
    XMMATRIX p = XMMatrixPerspectiveFovLH(XM_PI / 3.0f, (float)m_width / m_height, 0.1f, 100.0f);

    float fov = XM_PI / 3.0f, nearPlane = 0.1f;
    float w = tanf(fov / 2.0f) * nearPlane * 2.0f, h = ((float)m_height / m_width) * w;
    float R = sqrtf(nearPlane * nearPlane + (w / 2.0f) * (w / 2.0f) + (h / 2.0f) * (h / 2.0f)) * 1.1f;

    GeomBuffer gb; gb.m = XMMatrixTranspose(m); gb.size = { R, 0, 0, 0 };
    m_context->UpdateSubresource(m_geomBuffer.Get(), 0, nullptr, &gb, 0, 0);

    D3D11_MAPPED_SUBRESOURCE mapped; m_context->Map(m_sceneBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    SceneBuffer* sb = (SceneBuffer*)mapped.pData; sb->vp = XMMatrixTranspose(v * p); XMStoreFloat4(&sb->cameraPos, m_cameraPos); m_context->Unmap(m_sceneBuffer.Get(), 0);
}

void Dx11App::Cleanup() { m_rtv.Reset(); m_dsv.Reset(); m_depth.Reset(); m_swapChain.Reset(); m_context.Reset(); m_device.Reset(); }