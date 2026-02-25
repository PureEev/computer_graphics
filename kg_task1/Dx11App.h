#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#pragma comment(lib, "d3dcompiler.lib")

struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};

struct GeomBuffer
{
    DirectX::XMMATRIX m;
};

struct SceneBuffer
{
    DirectX::XMMATRIX vp;
};

class Dx11App
{
public:
    bool Init(HWND hwnd, int width, int height);
    void Cleanup();
    void Render();
    void OnResize(int width, int height);
    void OnMouseMove(int dx, int dy);

private:
    void CreateRenderTarget();
    void ReleaseRenderTarget();
    bool InitCube();
    void UpdateMatrices(float elapsed);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depth;

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_geomBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_sceneBuffer;

    D3D11_VIEWPORT m_viewport{};
    float m_time = 0;
    float m_camYaw = 0;
    float m_camPitch = 0;
    int m_width = 0;
    int m_height = 0;
};