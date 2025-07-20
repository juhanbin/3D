#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <vector>
#include <cstdio>
#include <cstring>
#include <fstream>

// Object types
const char* g_ObjectTypes[] = { "Cube", "Sphere", "Cylinder", "Capsule", "Monster" };
const int g_NumObjectTypes = sizeof(g_ObjectTypes) / sizeof(g_ObjectTypes[0]);

struct MapObject
{
    int   id;
    int   type; // 0=Cube, 1=Sphere, ...
    float pos[3];
    float size[3];
    float rot[3];
};

std::vector<MapObject> g_Objects;
std::vector<std::vector<MapObject>> g_UndoStack;
int g_Selected = -1;

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// simple save/load helpers
static void SaveScene(const char* filename)
{
    std::ofstream ofs(filename);
    for (const MapObject& o : g_Objects)
    {
        ofs << o.id << ' ' << o.type << ' '
            << o.size[0] << ' ' << o.size[1] << ' ' << o.size[2] << ' '
            << o.rot[0] << ' ' << o.rot[1] << ' ' << o.rot[2] << ' '
            << o.pos[0] << ' ' << o.pos[1] << ' ' << o.pos[2] << '\n';
    }
}

static bool LoadScene(const char* filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
        return false;
    g_Objects.clear();
    MapObject o;
    while (ifs >> o.id >> o.type
        >> o.size[0] >> o.size[1] >> o.size[2]
        >> o.rot[0] >> o.rot[1] >> o.rot[2]
        >> o.pos[0] >> o.pos[1] >> o.pos[2])
    {
        g_Objects.push_back(o);
    }
    g_Selected = -1;
    return true;
}

static void PushUndo()
{
    g_UndoStack.push_back(g_Objects);
    if (g_UndoStack.size() > 20)
        g_UndoStack.erase(g_UndoStack.begin());
}

int main(int, char**)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0,0 }, MONITOR_DEFAULTTOPRIMARY));
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"ImGui MapTool", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    static float temp_size[3] = { 1,1,1 };
    static float temp_rot[3] = { 0,0,0 };
    static float temp_pos[3] = { 0,0,0 };

    ImVec4 clear_color = ImVec4(0.2f, 0.25f, 0.3f, 1.0f);
    bool done = false;

    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("edit option");
        if (ImGui::Button("create"))
            ImGui::OpenPopup("CreateObjectPopup");
        ImGui::SameLine();
        if (ImGui::Button("undo") && !g_UndoStack.empty())
        {
            g_Objects = g_UndoStack.back();
            g_UndoStack.pop_back();
            g_Selected = -1;
        }
        ImGui::SameLine();
        if (ImGui::Button("load"))
            LoadScene("../Mapdata/scene.txt");
        ImGui::SameLine();
        if (ImGui::Button("save"))
            SaveScene("../Mapdata/scene.txt");

        if (ImGui::BeginPopup("CreateObjectPopup"))
        {
            ImGui::Text("Select object type");
            ImGui::Separator();
            for (int i = 0; i < g_NumObjectTypes; ++i)
            {
                if (ImGui::Selectable(g_ObjectTypes[i]))
                {
                    PushUndo();
                    MapObject obj{};
                    obj.id = (int)g_Objects.size();
                    obj.type = i;
                    obj.size[0] = obj.size[1] = obj.size[2] = 1.0f;
                    obj.rot[0] = obj.rot[1] = obj.rot[2] = 0.0f;
                    obj.pos[0] = obj.pos[1] = obj.pos[2] = 0.0f;
                    g_Objects.push_back(obj);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        for (int i = 0; i < (int)g_Objects.size(); ++i)
        {
            char label[64];
            std::snprintf(label, sizeof(label), "[%s] Object %d", g_ObjectTypes[g_Objects[i].type], g_Objects[i].id);
            if (ImGui::Selectable(label, g_Selected == i))
            {
                g_Selected = i;
                // 크기->회전->위치 순서로 복사
                std::memcpy(temp_size, g_Objects[i].size, sizeof(float) * 3);
                std::memcpy(temp_rot, g_Objects[i].rot, sizeof(float) * 3);
                std::memcpy(temp_pos, g_Objects[i].pos, sizeof(float) * 3);
            }
        }
        ImGui::End();

        ImGui::Begin("object option");
        if (g_Selected != -1)
        {
            // 크기 -> 회전 -> 위치 순서
            ImGui::Text("size");
            ImGui::DragFloat3("x/y/z##size", temp_size, 0.1f);

            ImGui::Text("rotation");
            ImGui::DragFloat3("x/y/z##rot", temp_rot, 0.1f);

            ImGui::Text("position");
            ImGui::DragFloat3("x/y/z##pos", temp_pos, 0.1f);

            if (ImGui::Button("delete"))
            {
                PushUndo();
                g_Objects.erase(g_Objects.begin() + g_Selected);
                g_Selected = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button("apply"))
            {
                PushUndo();
                // 크기 -> 회전 -> 위치 순서로 적용
                std::memcpy(g_Objects[g_Selected].size, temp_size, sizeof(float) * 3);
                std::memcpy(g_Objects[g_Selected].rot, temp_rot, sizeof(float) * 3);
                std::memcpy(g_Objects[g_Selected].pos, temp_pos, sizeof(float) * 3);
            }
        }
        ImGui::End();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
