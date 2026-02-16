#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};

class Dx11App
{
public:
    bool Init(HWND hwnd, int width, int height);
    void Cleanup();

    void Render();
    void OnResize(int width, int height);

private:
    void CreateRenderTarget();
    void ReleaseRenderTarget();
    bool InitTriangle();

private:
    Microsoft::WRL::ComPtr<ID3D11Device>           m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayout;


    D3D11_VIEWPORT m_viewport{};
};
