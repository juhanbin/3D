
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <vector>
#include <cstdio>
#include <cstring>
#include <fstream>

// EditorMain.cpp
#include "framework.h"
#include "MainApp.h"
#include "GameInstance.h"

#define MAX_LOADSTRING 100

HWND g_hWnd;
HINSTANCE g_hInst;

// **중복 선언 주의**
WCHAR szTitle[MAX_LOADSTRING] = L"MapTool";
WCHAR szWindowClass[MAX_LOADSTRING] = L"MapToolWndClass";

// 함수 선언
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CMainApp* pMainApp = nullptr;

    // 직접 할당
    wcscpy_s(szTitle, L"MapTool");
    wcscpy_s(szWindowClass, L"MapToolWndClass");
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    pMainApp = CMainApp::Create();
    if (nullptr == pMainApp)
        return FALSE;

    MSG msg;
    CGameInstance* pGameInstance = CGameInstance::GetInstance();
    Safe_AddRef(pGameInstance);

    if (FAILED(pGameInstance->Add_Timer(TEXT("Timer_Default"))))
        return FALSE;
    if (FAILED(pGameInstance->Add_Timer(TEXT("Timer_60"))))
        return FALSE;

    _float fTimeAcc = 0.f;

    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == msg.message)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        pGameInstance->Compute_TimeDelta(TEXT("Timer_Default"));
        fTimeAcc += pGameInstance->Get_TimeDelta(TEXT("Timer_Default"));

        if (fTimeAcc >= 1.f / 60.0f)
        {
            pGameInstance->Compute_TimeDelta(TEXT("Timer_60"));
            pMainApp->Update(pGameInstance->Get_TimeDelta(TEXT("Timer_60")));
            pMainApp->Render(); // ImGui도 여기서 호출
            fTimeAcc = 0.f;
        }
    }

    Safe_Release(pGameInstance);
    Safe_Release(pMainApp);

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL; // 아이콘 필요없으면 NULL
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL; // 메뉴 없음
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = NULL;
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    g_hInst = hInstance;
    RECT rcWindow = { 0, 0, g_iWinSizeX, g_iWinSizeY };
    AdjustWindowRect(&rcWindow, WS_OVERLAPPEDWINDOW, TRUE);

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;
    ShowWindow(hWnd, nCmdShow);
    ShowCursor(TRUE); // 에디터는 마우스 보이게!
    UpdateWindow(hWnd);

    g_hWnd = hWnd;
    return TRUE;
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ImGui Win32 메시지 처리
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}