#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>
#include <string>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

struct TextureDesc {
    UINT32 pitch = 0;
    UINT32 mipmapsCount = 0;
    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
    UINT32 width = 0;
    UINT32 height = 0;
    void* pData = nullptr;
};

struct Vertex {
    float x, y, z;
    float u, v;
    float nx, ny, nz;
    float tx, ty, tz;
};

struct SkyboxVertex {
    float x, y, z;
};

struct Light {
    DirectX::XMFLOAT4 pos;
    DirectX::XMFLOAT4 color;
};

// Буфер для Скайбокса
struct GeomBuffer {
    DirectX::XMMATRIX m;
    DirectX::XMFLOAT4 size;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT4 shine;
};

struct InstanceData {
    DirectX::XMMATRIX m;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT4 shine;
};

struct GeomBufferInst {
    InstanceData instances[100];
};

struct GeomBufferInstVis {
    DirectX::XMINT4 ids[100];
};

struct SceneBuffer {
    DirectX::XMMATRIX vp;
    DirectX::XMFLOAT4 cameraPos;
    DirectX::XMINT4 lightCount;
    DirectX::XMFLOAT4 ambientColor;
    Light lights[10];
    DirectX::XMFLOAT4 frustum[6]; 
};

// Структура параметров для Compute Shader
struct CullParams {
    DirectX::XMUINT4 numShapes;
    DirectX::XMFLOAT4 bbMin[100];
    DirectX::XMFLOAT4 bbMax[100];
};

class Dx11App
{
public:
    bool Init(HWND hwnd, int width, int height);
    void Cleanup();
    void Render();
    void ReadQueries(); // Метод для чтения статистики из GPU
    void OnResize(int width, int height);
    void OnMouseMove(int dx, int dy);

private:
    void CreateRenderTarget();
    void ReleaseRenderTarget();
    bool InitCube();
    bool InitSkybox();
    bool LoadTextures();
    void UpdateMatrices(float elapsed);
    void GenerateSphere(int latLines, int longLines, std::vector<SkyboxVertex>& vertices, std::vector<USHORT>& indices);

    bool LoadDDS(const wchar_t* filename, TextureDesc& desc, bool isCubemap = false);
    UINT32 DivUp(UINT32 a, UINT32 b);
    UINT32 GetBytesPerBlock(DXGI_FORMAT fmt);
    bool IsBoxInside(const DirectX::XMFLOAT4 frustum[6], const DirectX::XMFLOAT3& bbMin, const DirectX::XMFLOAT3& bbMax);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_rtv;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dsv;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depth;

    // Постпроцессинг
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_postProcessTex;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_postProcessRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_postProcessSRV;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_postVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_postPS;

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_skyboxVB;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_skyboxIB;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_skyboxVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_skyboxPS;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_skyboxLayout;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_skyboxDSState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_skyboxRSState;
    int m_skyboxIndexCount = 0;

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_geomBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_geomBufferInst;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_geomBufferInstVis;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_sceneBuffer;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubeTextureView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normalMapTextureView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_skyboxTextureView;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_opaqueDSState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_transparentDSState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_alphaBlendState;

    // --- Добавления для GPU Culling и Queries ---
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_cullCS;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_cullParamsCB;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indirectArgsSrc;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_indirectArgsUAV;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indirectArgs;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_geomBufferInstVisGPU;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_geomBufferInstVisGPU_UAV;

    Microsoft::WRL::ComPtr<ID3D11Query> m_queries[10];
    uint64_t m_curFrame = 0;
    uint64_t m_lastCompletedFrame = 0;
    int m_gpuVisibleInstances = 0;
    bool m_computeCull = true; // Флаг включения GPU Culling

    D3D11_VIEWPORT m_viewport{};
    float m_time = 0;
    float m_camYaw = 0;
    float m_camPitch = 0;
    int m_width = 0;
    int m_height = 0;

    DirectX::XMVECTOR m_cameraPos;
};