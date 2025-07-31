#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "MapObject.h"
#include "Edit_Defines.h"
#include "Graphic_Device.h"
#include <direct.h>


using namespace DirectX;
using namespace Edit;

NS_BEGIN(Edit)
// 생성자
CMainApp::CMainApp()
    : m_pGameInstance{ Engine::CGameInstance::GetInstance() }
{
    Safe_AddRef(m_pGameInstance);
}

// 엔진 초기화
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

// ---------- MapTool 함수 ----------

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

void CMainApp::SafeDebugOutput(const char* prefix, const char* str)
{
    char safeBuf[512];
    size_t len = strlen(str);
    if (len > 500) len = 500;

    for (size_t i = 0; i < len; ++i) {
        // ASCII만 통과, 나머지는 '.' 대체
        safeBuf[i] = (str[i] >= 32 && str[i] <= 126) ? str[i] : '.';
    }
    safeBuf[len] = 0;

    OutputDebugStringA(prefix);
    OutputDebugStringA(safeBuf);
    OutputDebugStringA("\n");
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
        LoadScene("../../Mapdata/scene.bin");
        RefreshScene();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        SaveScene("../../Mapdata/scene.bin");
    }

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
                switch (type) {
                case EObjectType::MONSTER:
                    // Rock_AA.fbx 예시
                    strcpy(obj.fbxPath, "../Bin/Resources/Blood_Spear/Model/Hero/Hero.fbx");

                    strcpy(obj.binPath, "../../Mapdata/Hero.bin");

                    break;
                case EObjectType::ROCK_AA:
                    // Rock_AA.fbx 예시
                    strcpy(obj.fbxPath, "../Bin/Resources/Blood_Spear/Model/Rock/Rock_AA.fbx");

                    strcpy(obj.binPath, "../../Mapdata/Rock_AA.bin");

                    break;
                default:
                    obj.fbxPath[0] = 0;
                    obj.binPath[0] = 0;
                    break;
                }
                m_Objects.push_back(obj);
                RefreshScene();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    // 피킹 (마우스 클릭)
    if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
        ImVec2 pos = ImGui::GetMousePos();
        int mouseX = (int)pos.x;
        int mouseY = (int)pos.y;

        XMMATRIX view = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Float4x4(Engine::D3DTS::VIEW));
        XMMATRIX proj = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Float4x4(Engine::D3DTS::PROJ));
        int winW = g_iWinSizeX;
        int winH = g_iWinSizeY;
        Ray ray = CreatePickingRay(mouseX, mouseY, winW, winH, view, proj);

        int pickIdx = -1;
        float minDist = FLT_MAX;
        for (int i = 0; i < (int)m_Objects.size(); ++i) {
            BoundingBox box;
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
        ImGui::InputText("BIN Path", m_Objects[m_Selected].binPath, 260);

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
        // BIN 내보내기 (FBX → BIN)
        if (ImGui::Button("Export_NonAnim")) {
            ExportModelToBin_NonAnim(m_Objects[m_Selected], m_Objects[m_Selected].binPath);
        }
        if (ImGui::Button("Export_Anim")) {
            ExportModelToBin_Anim(m_Objects[m_Selected], m_Objects[m_Selected].binPath);
        }
    }
    ImGui::End();
}

// 씬 저장 (MapObject만)
void CMainApp::SaveScene(const char* filename)
{
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return;
    size_t count = m_Objects.size();
    ofs.write((const char*)&count, sizeof(count));
    for (const auto& obj : m_Objects) {
        ofs.write((const char*)&obj, sizeof(MapObject));
    }
}

// 씬 불러오기
bool CMainApp::LoadScene(const char* filename)
{
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return false;
    size_t count = 0;
    ifs.read((char*)&count, sizeof(count));
    if (!ifs) return false;
    m_Objects.clear();
    for (size_t i = 0; i < count; ++i) {
        MapObject obj{};
        ifs.read((char*)&obj, sizeof(MapObject));

        // [추가] 모든 char 배열 null 보장
        obj.fbxPath[sizeof(obj.fbxPath) - 1] = 0;
        obj.binPath[sizeof(obj.binPath) - 1] = 0;

        // 구조체 내부에 char 배열 더 있으면 여기도 처리!
        // 예: obj.name[sizeof(obj.name)-1] = 0;

        m_Objects.push_back(obj);
    }
    m_Selected = -1;

    for (const auto& obj : m_Objects) {
        //OutputDebugStringA("로드된 오브젝트 fbxPath: ");
        //OutputDebugStringA(obj.fbxPath);
        //OutputDebugStringA("\n");
    }

    return true;
}


// 애니메이션 BIN 내보내기
void CMainApp::ExportModelToBin_Anim(const MapObject& obj, const char* binPath)
{
    if (FILE* fp = fopen(obj.fbxPath, "rb")) fclose(fp);
    else return;

    _uint iFlag = { aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast };
    m_pAIScene = m_Importer.ReadFile(obj.fbxPath, iFlag);
    if (!m_pAIScene || !m_pAIScene->HasMeshes()) return;

    std::ofstream ofs(binPath, std::ios::binary);
    if (!ofs) return;

    {
        char dbg[128];
        sprintf_s(dbg, "[SAVE-DEBUG] sizeof AnimInfo=%zu ChannelInfo=%zu KeyFrame=%zu\n",
            sizeof(AnimInfo), sizeof(ChannelInfo), sizeof(KeyFrame));
        OutputDebugStringA(dbg);
    }

    // ---- 1. 메시 저장 ----
    uint32_t meshCount = m_pAIScene->mNumMeshes;
    ofs.write((char*)&meshCount, sizeof(meshCount));
    for (uint32_t m = 0; m < meshCount; ++m) {
        aiMesh* mesh = m_pAIScene->mMeshes[m];

        uint32_t vtxCount = mesh->mNumVertices;
        uint32_t idxCount = mesh->mNumFaces * 3;
        ofs.write((char*)&vtxCount, sizeof(vtxCount));
        ofs.write((char*)&idxCount, sizeof(idxCount));

        std::vector<SimpleVertex> vertices(vtxCount);
        for (uint32_t i = 0; i < vtxCount; ++i) {
            vertices[i].pos[0] = mesh->mVertices[i].x;
            vertices[i].pos[1] = mesh->mVertices[i].y;
            vertices[i].pos[2] = mesh->mVertices[i].z;

            vertices[i].normal[0] = mesh->HasNormals() ? mesh->mNormals[i].x : 0;
            vertices[i].normal[1] = mesh->HasNormals() ? mesh->mNormals[i].y : 0;
            vertices[i].normal[2] = mesh->HasNormals() ? mesh->mNormals[i].z : 0;

            if (mesh->HasTextureCoords(0)) {
                vertices[i].uv[0] = mesh->mTextureCoords[0][i].x;
                vertices[i].uv[1] = mesh->mTextureCoords[0][i].y;
            }
            else {
                vertices[i].uv[0] = vertices[i].uv[1] = 0.f;
            }
        }
        ofs.write((char*)vertices.data(), sizeof(SimpleVertex) * vtxCount);

        std::vector<uint32_t> indices(idxCount);
        uint32_t k = 0;
        for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
            aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (int j = 0; j < 3; ++j)
                indices[k++] = face.mIndices[j];
        }
        ofs.write((char*)indices.data(), sizeof(uint32_t) * idxCount);
    }

    // ---- 2. 머티리얼 저장 ----
    uint32_t matCount = m_pAIScene->mNumMaterials;
    ofs.write((char*)&matCount, sizeof(matCount));
    for (uint32_t i = 0; i < matCount; ++i) {
        aiMaterial* material = m_pAIScene->mMaterials[i];
        MaterialInfo matInfo{};
        memset(&matInfo, 0, sizeof(matInfo));
        ExtractTextureFilename(material, aiTextureType_DIFFUSE, matInfo.basecolor, sizeof(matInfo.basecolor), "Diffuse");
        ExtractTextureFilename(material, aiTextureType_NORMALS, matInfo.normal, sizeof(matInfo.normal), "Normal");
        ofs.write((char*)&matInfo, sizeof(MaterialInfo));
    }

    // ---- 3. 본 저장 ----
    std::vector<BoneInfo> bones;
    std::function<void(const aiNode*, int)> gatherBones = [&](const aiNode* node, int parentIdx) {
        BoneInfo bi{};
        memset(&bi, 0, sizeof(bi));
        strncpy(bi.name, node->mName.C_Str(), sizeof(bi.name) - 1);
        bi.parentIdx = parentIdx;
        memcpy(bi.transform, &node->mTransformation, sizeof(float) * 16);

        bool foundOffset = false;
        for (uint32_t m = 0; m < m_pAIScene->mNumMeshes; ++m) {
            aiMesh* mesh = m_pAIScene->mMeshes[m];
            for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
                aiBone* bone = mesh->mBones[b];
                if (bone->mName == node->mName) {
                    memcpy(bi.offset, &bone->mOffsetMatrix, sizeof(float) * 16);
                    foundOffset = true; break;
                }
            }
            if (foundOffset) break;
        }
        if (!foundOffset) {
            for (int i = 0; i < 16; ++i) bi.offset[i] = (i % 5 == 0) ? 1.f : 0.f;
        }

        int curIdx = (int)bones.size();
        bones.push_back(bi);
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
            gatherBones(node->mChildren[i], curIdx);
        };
    gatherBones(m_pAIScene->mRootNode, -1);

    uint32_t totalBoneCount = (uint32_t)bones.size();
    ofs.write((char*)&totalBoneCount, sizeof(totalBoneCount));
    ofs.write((char*)bones.data(), sizeof(BoneInfo) * bones.size());

    // ---- 4. 애니메이션 저장 ----
    // 디버그: 세이브 시점 크기 확인
    char dbg[128];
    sprintf_s(dbg, "[SAVE-DEBUG] sizeof AnimInfo=%zu ChannelInfo=%zu KeyFrame=%zu\n",
        sizeof(AnimInfo), sizeof(ChannelInfo), sizeof(KeyFrame));
    OutputDebugStringA(dbg);

    // [중략: Mesh/Material/Bone 저장 로직]

    uint32_t animCount = m_pAIScene->mNumAnimations;
    ofs.write((char*)&animCount, sizeof(animCount));
    for (uint32_t a = 0; a < animCount; ++a) {
        aiAnimation* anim = m_pAIScene->mAnimations[a];
        AnimInfo ainfo{};
        strncpy(ainfo.name, anim->mName.C_Str(), sizeof(ainfo.name) - 1);
        ainfo.duration = anim->mDuration;
        ainfo.ticksPerSecond = anim->mTicksPerSecond;
        ainfo.channelCount = anim->mNumChannels;
        ofs.write((char*)&ainfo, sizeof(AnimInfo));

        for (uint32_t c = 0; c < anim->mNumChannels; ++c) {
            aiNodeAnim* chan = anim->mChannels[c];
            ChannelInfo cinfo{};
            strncpy(cinfo.boneName, chan->mNodeName.C_Str(), sizeof(cinfo.boneName) - 1);
            cinfo.keyframeCount = max3(chan->mNumPositionKeys, chan->mNumRotationKeys, chan->mNumScalingKeys);
            ofs.write((char*)&cinfo, sizeof(ChannelInfo));

            char dbg2[256];
            sprintf_s(dbg2, "[SAVE] Channel[%u]: bone=%s keyframes=%u\n", c, cinfo.boneName, cinfo.keyframeCount);
            OutputDebugStringA(dbg2);

            // [KeyFrame 저장 로직 그대로]
        }
    }
    ofs.close();
}




void CMainApp::ExportModelToBin_NonAnim(const MapObject& obj, const char* binPath)
{
    if (FILE* fp = fopen(obj.fbxPath, "rb")) fclose(fp);
    else return;

    _uint iFlag = { aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast };
    m_pAIScene = m_Importer.ReadFile(obj.fbxPath, iFlag);
    if (!m_pAIScene || !m_pAIScene->HasMeshes()) return;

    std::ofstream ofs(binPath, std::ios::binary);
    if (!ofs) return;

    // ---- 메시 저장 ----
    uint32_t meshCount = m_pAIScene->mNumMeshes;
    ofs.write((char*)&meshCount, sizeof(meshCount));
    for (uint32_t m = 0; m < meshCount; ++m) {
        aiMesh* mesh = m_pAIScene->mMeshes[m];
        uint32_t vtxCount = mesh->mNumVertices;
        uint32_t idxCount = mesh->mNumFaces * 3;
        ofs.write((char*)&vtxCount, sizeof(vtxCount));
        ofs.write((char*)&idxCount, sizeof(idxCount));

        std::vector<SimpleVertex> vertices(vtxCount);
        for (uint32_t i = 0; i < vtxCount; ++i) {
            vertices[i].pos[0] = mesh->mVertices[i].x;
            vertices[i].pos[1] = mesh->mVertices[i].y;
            vertices[i].pos[2] = mesh->mVertices[i].z;
            vertices[i].normal[0] = mesh->HasNormals() ? mesh->mNormals[i].x : 0;
            vertices[i].normal[1] = mesh->HasNormals() ? mesh->mNormals[i].y : 0;
            vertices[i].normal[2] = mesh->HasNormals() ? mesh->mNormals[i].z : 0;
            if (mesh->HasTextureCoords(0)) {
                vertices[i].uv[0] = mesh->mTextureCoords[0][i].x;
                vertices[i].uv[1] = mesh->mTextureCoords[0][i].y;
            }
            else {
                vertices[i].uv[0] = vertices[i].uv[1] = 0.f;
            }
        }
        ofs.write((char*)vertices.data(), sizeof(SimpleVertex) * vtxCount);

        std::vector<uint32_t> indices(idxCount);
        uint32_t k = 0;
        for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
            aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (int j = 0; j < 3; ++j)
                indices[k++] = face.mIndices[j];
        }
        ofs.write((char*)indices.data(), sizeof(uint32_t) * idxCount);
    }

    // ---- 머티리얼 저장 ----
    uint32_t matCount = m_pAIScene->mNumMaterials;
    ofs.write((char*)&matCount, sizeof(matCount));
    for (uint32_t i = 0; i < matCount; ++i) {
        aiMaterial* material = m_pAIScene->mMaterials[i];
        MaterialInfo matInfo{};
        memset(&matInfo, 0, sizeof(matInfo));
        ExtractTextureFilename(material, aiTextureType_DIFFUSE, matInfo.basecolor, sizeof(matInfo.basecolor), "Diffuse");
        ExtractTextureFilename(material, aiTextureType_NORMALS, matInfo.normal, sizeof(matInfo.normal), "Normal");
        ofs.write((char*)&matInfo, sizeof(MaterialInfo));
    }

    ofs.close();
}



// Undo
void CMainApp::PushUndo()
{
    m_UndoStack.push_back(m_Objects);
    if (m_UndoStack.size() > 20)
        m_UndoStack.erase(m_UndoStack.begin());
}

// 씬 갱신
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

void CMainApp::ExtractTextureFilename(const aiMaterial* material, aiTextureType type, char* outBuf, size_t bufSize, const char* dbgName)
{
    aiString path;
    if (material->GetTexture(type, 0, &path) == AI_SUCCESS) {
        std::string texPath = path.C_Str();
        std::replace(texPath.begin(), texPath.end(), '#', '/');
        auto lastSlash = texPath.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            texPath = texPath.substr(lastSlash + 1);
        strncpy(outBuf, texPath.c_str(), bufSize - 1);
        outBuf[bufSize - 1] = 0;
        char buf[256];
        //sprintf(buf, "[%s]%s->%s\n", dbgName, path.C_Str(), texPath.c_str());
        //OutputDebugStringA(buf);
    }
    else {
        outBuf[0] = 0;
        char buf[128];
        //sprintf(buf, "[%s](none)\n", dbgName);
        //OutputDebugStringA(buf);
    }
}

// 인스턴스 생성
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

NS_END