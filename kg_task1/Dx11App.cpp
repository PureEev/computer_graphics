#include "Dx11App.h"
#include <dxgi.h>
#include <chrono>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using namespace DirectX;

bool Dx11App::Init(HWND hwnd, int width, int height)
{
    m_width = width;
    m_height = height;

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, m_swapChain.GetAddressOf(),
        m_device.GetAddressOf(), nullptr,
        m_context.GetAddressOf())))
        return false;

    CreateRenderTarget();
    OnResize(width, height);

    if (!InitCube())
        return false;

    return true;
}

void Dx11App::CreateRenderTarget()
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_rtv.GetAddressOf());
}

void Dx11App::ReleaseRenderTarget()
{
    m_rtv.Reset();
    m_dsv.Reset();
    m_depth.Reset();
}

void Dx11App::OnResize(int width, int height)
{
    if (!m_swapChain || width == 0 || height == 0)
        return;

    m_width = width;
    m_height = height;

    ReleaseRenderTarget();
    m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    m_device->CreateTexture2D(&depthDesc, nullptr, m_depth.GetAddressOf());
    m_device->CreateDepthStencilView(m_depth.Get(), nullptr, m_dsv.GetAddressOf());

    m_viewport = { 0,0,(float)width,(float)height,0,1 };
    m_context->RSSetViewports(1, &m_viewport);
}

void Dx11App::Render()
{
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    m_time = std::chrono::duration<float>(now - start).count();

    UpdateMatrices(m_time);

    float clear[4] = { 0.1f,0.1f,0.2f,1 };

    m_context->OMSetRenderTargets(1, m_rtv.GetAddressOf(), m_dsv.Get());
    m_context->ClearRenderTargetView(m_rtv.Get(), clear);
    m_context->ClearDepthStencilView(m_dsv.Get(), D3D11_CLEAR_DEPTH, 1, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    m_context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetInputLayout(m_inputLayout.Get());
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11Buffer* vsCB[] = { m_geomBuffer.Get(), m_sceneBuffer.Get() };
    m_context->VSSetConstantBuffers(0, 2, vsCB);

    m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    m_context->DrawIndexed(36, 0, 0);

    m_swapChain->Present(1, 0);
}

void Dx11App::OnMouseMove(int dx, int dy)
{
    m_camYaw += dx * 0.005f;
    m_camPitch += dy * 0.005f;
}

bool Dx11App::InitCube()
{
    Vertex v[] =
    {
        {-1,-1,-1,1,0,0,1},{1,-1,-1,0,1,0,1},{1,1,-1,0,0,1,1},{-1,1,-1,1,1,0,1},
        {-1,-1,1,1,0,1,1},{1,-1,1,0,1,1,1},{1,1,1,1,1,1,1},{-1,1,1,0,0,0,1}
    };

    USHORT i[] =
    {
        0,1,2,0,2,3,4,6,5,4,7,6,
        4,5,1,4,1,0,3,2,6,3,6,7,
        1,5,6,1,6,2,4,0,3,4,3,7
    };

    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = sizeof(v);
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = v;
    m_device->CreateBuffer(&bd, &sd, m_vertexBuffer.GetAddressOf());

    bd.ByteWidth = sizeof(i);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    sd.pSysMem = i;
    m_device->CreateBuffer(&bd, &sd, m_indexBuffer.GetAddressOf());

    Microsoft::WRL::ComPtr<ID3DBlob> vs, ps;

    D3DCompileFromFile(L"cube.vs.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, vs.GetAddressOf(), nullptr);
    D3DCompileFromFile(L"cube.ps.hlsl", nullptr, nullptr, "ps", "ps_5_0", 0, 0, ps.GetAddressOf(), nullptr);

    m_device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
    m_device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}
    };

    m_device->CreateInputLayout(layout, 2, vs->GetBufferPointer(), vs->GetBufferSize(), m_inputLayout.GetAddressOf());

    bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.ByteWidth = sizeof(GeomBuffer);
    m_device->CreateBuffer(&bd, nullptr, m_geomBuffer.GetAddressOf());

    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth = sizeof(SceneBuffer);
    m_device->CreateBuffer(&bd, nullptr, m_sceneBuffer.GetAddressOf());

    return true;
}

void Dx11App::UpdateMatrices(float t)
{
    XMMATRIX m = XMMatrixRotationY(t);
    XMMATRIX camRot = XMMatrixRotationRollPitchYaw(m_camPitch, m_camYaw, 0);
    XMVECTOR eye = XMVector3TransformCoord(XMVectorSet(0, 0, -5, 1), camRot);
    XMMATRIX v = XMMatrixLookAtLH(eye, XMVectorZero(), XMVectorSet(0, 1, 0, 0));
    XMMATRIX p = XMMatrixPerspectiveFovLH(XM_PI / 3.0f, (float)m_width / m_height, 0.1f, 100);

    GeomBuffer gb{ XMMatrixTranspose(m) };
    m_context->UpdateSubresource(m_geomBuffer.Get(), 0, nullptr, &gb, 0, 0);

    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_sceneBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    SceneBuffer* sb = (SceneBuffer*)mapped.pData;
    sb->vp = XMMatrixTranspose(v * p);
    m_context->Unmap(m_sceneBuffer.Get(), 0);
}

void Dx11App::Cleanup()
{
    m_rtv.Reset();
    m_dsv.Reset();
    m_depth.Reset();
    m_swapChain.Reset();
    m_context.Reset();
    m_device.Reset();
}