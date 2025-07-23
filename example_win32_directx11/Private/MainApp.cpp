#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "MapObject.h"
#include "Edit_Defines.h"
#include "Graphic_Device.h"
#include <fstream>

using namespace DirectX;
using namespace Edit;

CMainApp::CMainApp()
    : m_pGameInstance{ Engine::CGameInstance::GetInstance() }
{
    Safe_AddRef(m_pGameInstance);
}

HRESULT CMainApp::Initialize()
{
    ENGINE_DESC EngineDesc{};

    EngineDesc.hInst = g_hInst;
    EngineDesc.hWnd = g_hWnd;
    EngineDesc.eWinMode = WINMODE::WIN;
    EngineDesc.iWinSizeX = g_iWinSizeX;
    EngineDesc.iWinSizeY = g_iWinSizeY;
    EngineDesc.iNumLevels = ENUM_CLASS(LEVEL::END);

    if (FAILED(m_pGameInstance->Initialize_Engine(EngineDesc, &m_pDevice, &m_pContext)))
        return E_FAIL;

    if (FAILED(Ready_Prototype_ForStatic()))
        return E_FAIL;

    if (FAILED(Start_Level(LEVEL::EDIT)))
        return E_FAIL;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(m_pDevice, m_pContext);

    CGraphic_Device* pGraphicDevice = m_pGameInstance->GetGraphicDevice();
    m_pBackBufferRTV = pGraphicDevice->GetBackBufferRTV();
    m_pDepthStencilView = pGraphicDevice->GetDepthStencilView();

    return S_OK;
}

void CMainApp::Update(_float fTimeDelta)
{
    m_pGameInstance->Update_Engine(fTimeDelta);
}

HRESULT CMainApp::Render()
{
    _float4 vClearColor = _float4(0.f, 0.f, 1.f, 1.f);

    m_pGameInstance->Render_Begin(&vClearColor);
    m_pGameInstance->Draw();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    Render_ImGuiPanel();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    m_pGameInstance->Render_End();
    return S_OK;
}

HRESULT CMainApp::Ready_Prototype_ForStatic()
{
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex"),
        CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements))))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_BG"),
        CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex_BG.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements))))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_Logo"),
        CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex_Logo.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements))))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_Fade"),
        CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex_Fade.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements))))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        CVIBuffer_Rect::Create(m_pDevice, m_pContext))))
        return E_FAIL;

    return S_OK;
}

HRESULT CMainApp::Start_Level(LEVEL eStartLevelID)
{
    if (FAILED(m_pGameInstance->Open_Level(static_cast<_uint>(LEVEL::LOADING), CLevel_Loading::Create(m_pDevice, m_pContext, eStartLevelID))))
        return E_FAIL;
    return S_OK;
}

// ----------- MapTool 관련 함수 ------------

Ray CMainApp::CreatePickingRay(int mx, int my, int w, int h, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj)
{
    float px = (2.0f * mx / w - 1.0f);
    float py = (1.0f - 2.0f * my / h);
    XMVECTOR rayClip = XMVectorSet(px, py, 1.0f, 1.0f);

    XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
    XMVECTOR rayEye = XMVector3TransformCoord(rayClip, invProj);
    rayEye = XMVectorSetW(rayEye, 0.0f);

    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMVECTOR rayDir = XMVector3TransformNormal(rayEye, invView);
    rayDir = XMVector3Normalize(rayDir);
    XMVECTOR rayOrigin = XMVector3TransformCoord(XMVectorZero(), invView);

    Ray ray;
    XMStoreFloat3(&ray.origin, rayOrigin);
    XMStoreFloat3(&ray.dir, rayDir);
    return ray;
}

bool CMainApp::RayIntersectsAABB(const Ray& ray, const DirectX::BoundingBox& box, float* outDist)
{
    float dist = 0.0f;
    bool hit = box.Intersects(XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.dir), dist);
    if (hit && outDist) *outDist = dist;
    return hit;
}

void CMainApp::Render_ImGuiPanel()
{
    ImGui::Begin("Map Tool Panel");

    if (ImGui::Button("Create Object")) {
        ImGui::OpenPopup("CreateObjectPopup");
    }
    ImGui::SameLine();
    if (ImGui::Button("Undo") && !m_UndoStack.empty()) {
        m_Objects = m_UndoStack.back();
        m_UndoStack.pop_back();
        m_Selected = -1;
        RefreshScene();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        LoadScene("../../Mapdata/scene.txt");
        RefreshScene();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        SaveScene("../../Mapdata/scene.txt");
    }

    // 오브젝트 타입 선택(enum)
    if (ImGui::BeginPopup("CreateObjectPopup")) {
        ImGui::Text("Select Object Type");
        ImGui::Separator();
        for (int i = 0; i < NumObjectTypes; ++i) {
            EObjectType type = static_cast<EObjectType>(i);
            if (ImGui::Selectable(ToObjectTypeString(type))) {
                PushUndo();
                MapObject obj{};
                obj.id = (int)m_Objects.size();
                obj.type = type;
                obj.size[0] = obj.size[1] = obj.size[2] = 1.0f;
                obj.rot[0] = obj.rot[1] = obj.rot[2] = 0.0f;
                obj.pos[0] = obj.pos[1] = obj.pos[2] = 0.0f;
                m_Objects.push_back(obj);
                RefreshScene();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::Text("피킹: 오브젝트 선택 (마우스 클릭)");

    // ImGui 패널이 열려있는 상태에서만 피킹 처리
    if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) // 왼쪽 클릭 시
    {
        // 마우스 좌표 구하기
        ImVec2 pos = ImGui::GetMousePos();
        int mouseX = (int)pos.x;
        int mouseY = (int)pos.y;

        char buf[128];
        sprintf(buf, "Mouse (Screen): %.2f %.2f  (Client): %d %d\n", pos.x, pos.y, mouseX, mouseY);
        OutputDebugStringA(buf);

        // 뷰/프로젝션 행렬 구하기
        XMMATRIX view = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Float4x4(Engine::D3DTS::VIEW));
        XMMATRIX proj = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Float4x4(Engine::D3DTS::PROJ));

        // 레이 생성
        int winW = g_iWinSizeX;
        int winH = g_iWinSizeY;
        Ray ray = CreatePickingRay(mouseX, mouseY, winW, winH, view, proj);

        sprintf(buf, "Ray O: %.2f %.2f %.2f D: %.2f %.2f %.2f\n", ray.origin.x, ray.origin.y, ray.origin.z, ray.dir.x, ray.dir.y, ray.dir.z);
        OutputDebugStringA(buf);

        int pickIdx = -1;
        float minDist = FLT_MAX;
        for (int i = 0; i < (int)m_Objects.size(); ++i) {
            BoundingBox box;
            // 오브젝트별 계산
            XMFLOAT3 center = XMFLOAT3(m_Objects[i].pos[0], m_Objects[i].pos[1], m_Objects[i].pos[2]);
            XMFLOAT3 extents = XMFLOAT3(
                m_Objects[i].size[0] * 0.5f,
                m_Objects[i].size[1] * 0.5f,
                m_Objects[i].size[2] * 0.5f
            );
            box.Center = center;
            box.Extents = extents;

            float hitDist;
            if (RayIntersectsAABB(ray, box, &hitDist)) {
                if (hitDist < minDist) {
                    minDist = hitDist;
                    pickIdx = i;
                }
            }
        }
        if (pickIdx >= 0) {
            m_Selected = pickIdx;
            memcpy(m_TempSize, m_Objects[m_Selected].size, sizeof(float) * 3);
            memcpy(m_TempRot, m_Objects[m_Selected].rot, sizeof(float) * 3);
            memcpy(m_TempPos, m_Objects[m_Selected].pos, sizeof(float) * 3);
        }
    }

    ImGui::Separator();

    for (int i = 0; i < (int)m_Objects.size(); ++i) {
        char label[64];
        std::snprintf(label, sizeof(label), "[%s] Object %d", ToObjectTypeString(m_Objects[i].type), m_Objects[i].id);
        if (ImGui::Selectable(label, m_Selected == i)) {
            m_Selected = i;
            std::memcpy(m_TempSize, m_Objects[i].size, sizeof(float) * 3);
            std::memcpy(m_TempRot, m_Objects[i].rot, sizeof(float) * 3);
            std::memcpy(m_TempPos, m_Objects[i].pos, sizeof(float) * 3);
        }
    }

    ImGui::End();

    ImGui::Begin("Object Properties");
    if (m_Selected != -1) {
        ImGui::Text("Size");
        ImGui::DragFloat3("x/y/z##size", m_TempSize, 0.1f);
        ImGui::Text("Rotation");
        ImGui::DragFloat3("x/y/z##rot", m_TempRot, 0.1f);
        ImGui::Text("Position");
        ImGui::DragFloat3("x/y/z##pos", m_TempPos, 0.1f);

        if (ImGui::Button("Delete")) {
            PushUndo();
            m_Objects.erase(m_Objects.begin() + m_Selected);
            m_Selected = -1;
            RefreshScene();
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            PushUndo();
            std::memcpy(m_Objects[m_Selected].size, m_TempSize, sizeof(float) * 3);
            std::memcpy(m_Objects[m_Selected].rot, m_TempRot, sizeof(float) * 3);
            std::memcpy(m_Objects[m_Selected].pos, m_TempPos, sizeof(float) * 3);
            RefreshScene();
        }
    }
    ImGui::End();
}

void CMainApp::SaveScene(const char* filename)
{
    std::ofstream ofs(filename);
    for (const MapObject& o : m_Objects)
    {
        ofs << o.id << ' ' << static_cast<int>(o.type) << ' '
            << o.size[0] << ' ' << o.size[1] << ' ' << o.size[2] << ' '
            << o.rot[0] << ' ' << o.rot[1] << ' ' << o.rot[2] << ' '
            << o.pos[0] << ' ' << o.pos[1] << ' ' << o.pos[2] << '\n';
    }
}

bool CMainApp::LoadScene(const char* filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
        return false;
    m_Objects.clear();
    MapObject o;
    int typeInt;
    while (ifs >> o.id >> typeInt
        >> o.size[0] >> o.size[1] >> o.size[2]
        >> o.rot[0] >> o.rot[1] >> o.rot[2]
        >> o.pos[0] >> o.pos[1] >> o.pos[2])
    {
        o.type = static_cast<EObjectType>(typeInt);
        m_Objects.push_back(o);
    }
    m_Selected = -1;
    return true;
}

void CMainApp::PushUndo()
{
    m_UndoStack.push_back(m_Objects);
    if (m_UndoStack.size() > 20)
        m_UndoStack.erase(m_UndoStack.begin());
}

void CMainApp::RefreshScene()
{
    m_pGameInstance->Clear_Layer(ENUM_CLASS(LEVEL::EDIT), L"Layer_MapObject");

    for (const MapObject& o : m_Objects)
    {
        CMapObject::MAPOBJECT_DESC desc{};
        desc.type = o.type;
        desc.vScale = _float3(o.size[0], o.size[1], o.size[2]);
        desc.vRot = _float3(o.rot[0], o.rot[1], o.rot[2]);
        desc.vPos = _float3(o.pos[0], o.pos[1], o.pos[2]);

        m_pGameInstance->Add_GameObject_ToLayer(
            ENUM_CLASS(LEVEL::EDIT), L"Layer_MapObject",
            ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_GameObject_MapObject"), &desc);
    }
}

CMainApp* CMainApp::Create()
{
    CMainApp* pInstance = new CMainApp();

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX(TEXT("Failed to Created : CMainApp"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CMainApp::Free()
{
    __super::Free();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);

    m_pGameInstance->Release_Engine();

    Safe_Release(m_pGameInstance);
}