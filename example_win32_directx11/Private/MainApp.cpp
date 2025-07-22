#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "MapObject.h"
#include "Edit_Defines.h"
#include "Graphic_Device.h"

CMainApp::CMainApp()
    : m_pGameInstance{ CGameInstance::GetInstance() }
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

    // 팝업: 오브젝트 타입 선택(enum)
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
