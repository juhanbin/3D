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
        m_Objects.push_back(obj);
    }
    m_Selected = -1;

    for (const auto& obj : m_Objects) {
        OutputDebugStringA("로드된 오브젝트 fbxPath: ");
        OutputDebugStringA(obj.fbxPath);
        OutputDebugStringA("\n");
    }

    return true;
}

// 애니메이션 BIN 내보내기
void CMainApp::ExportModelToBin_Anim(const MapObject& obj, const char* binPath)
{
    OutputDebugStringA("[BIN Export/Anim] fbxPath: ");
    OutputDebugStringA(obj.fbxPath);
    OutputDebugStringA("\n[BIN Export/Anim] binPath: ");
    OutputDebugStringA(binPath); OutputDebugStringA("\n");

    if (FILE* fp = fopen(obj.fbxPath, "rb")) fclose(fp);
    else { OutputDebugStringA("[BIN Export/Anim] 실패: FBX 파일이 존재하지 않음!\n"); return; }

    _uint iFlag = aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast;
    m_pAIScene = m_Importer.ReadFile(obj.fbxPath, iFlag);

    if (!m_pAIScene || !m_pAIScene->HasMeshes()) {
        OutputDebugStringA("[BIN Export/Anim] 실패: FBX 로드 실패!\n"); return;
    }

    std::ofstream ofs(binPath, std::ios::binary);
    if (!ofs) { OutputDebugStringA("[BIN Export/Anim] 실패: BIN 파일 저장 불가!\n"); return; }

    // ---- 1. 메시 저장 ----
    {
        char buf[256];
        uint32_t meshCount = m_pAIScene->mNumMeshes;
        sprintf(buf, "[BIN/Anim] Mesh Count: %u\n", meshCount);
        OutputDebugStringA(buf);
        ofs.write((char*)&meshCount, sizeof(meshCount));
        for (uint32_t m = 0; m < meshCount; ++m) {
            aiMesh* mesh = m_pAIScene->mMeshes[m];
            sprintf(buf, "  Mesh[%u] Name: %s  Vertex: %u  Index: %u  BoneCount: %u\n",
                m, mesh->mName.C_Str(), mesh->mNumVertices, mesh->mNumFaces * 3, mesh->mNumBones);
            OutputDebugStringA(buf);
            // --- 기존 vertex, bone 가중치, 인덱스 저장 코드 ---
            uint32_t vtxCount = mesh->mNumVertices;
            uint32_t idxCount = mesh->mNumFaces * 3;
            ofs.write((char*)&vtxCount, sizeof(vtxCount));
            ofs.write((char*)&idxCount, sizeof(idxCount));
            std::vector<SimpleVertex> vertices(vtxCount);
            std::vector<std::vector<std::pair<int, float>>> vtxBones(vtxCount);
            for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
                aiBone* bone = mesh->mBones[b];
                for (uint32_t w = 0; w < bone->mNumWeights; ++w) {
                    int vIdx = bone->mWeights[w].mVertexId;
                    float weight = bone->mWeights[w].mWeight;
                    vtxBones[vIdx].emplace_back(b, weight);
                }
            }
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
                for (int b = 0; b < 4; ++b) {
                    if (b < vtxBones[i].size()) {
                        vertices[i].blendIndex[b] = vtxBones[i][b].first;
                        vertices[i].blendWeight[b] = vtxBones[i][b].second;
                    }
                    else {
                        vertices[i].blendIndex[b] = 0;
                        vertices[i].blendWeight[b] = 0.f;
                    }
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
    }

    // ---- 2. 머티리얼 저장 ----
    {
        char buf[256];
        uint32_t matCount = m_pAIScene->mNumMaterials;
        sprintf(buf, "[BIN/Anim] Material Count: %u\n", matCount);
        OutputDebugStringA(buf);
        ofs.write((char*)&matCount, sizeof(matCount));
        for (uint32_t i = 0; i < matCount; ++i) {
            aiMaterial* material = m_pAIScene->mMaterials[i];
            aiString path;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &path);
            sprintf(buf, "  Material[%u] Diffuse: %s\n", i, path.C_Str());
            OutputDebugStringA(buf);
            MaterialInfo matInfo{};
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
                strcpy_s(matInfo.basecolor, path.C_Str());
            if (material->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS)
                strcpy_s(matInfo.normal, path.C_Str());
            for (int j = 0; j < material->GetTextureCount(aiTextureType_UNKNOWN); ++j) {
                if (material->GetTexture(aiTextureType_UNKNOWN, j, &path) == AI_SUCCESS) {
                    if (strstr(path.C_Str(), "ARM") || strstr(path.C_Str(), "arm")) {
                        strcpy_s(matInfo.arm, path.C_Str());
                        break;
                    }
                }
            }
            ofs.write((char*)&matInfo, sizeof(MaterialInfo));
        }
    }

    // ---- 3. 본 저장 ----
    std::vector<BoneInfo> bones;
    std::function<void(const aiNode*, int)> gatherBones = [&](const aiNode* node, int parentIdx) {
        BoneInfo bi{};
        strncpy(bi.name, node->mName.C_Str(), 63); bi.name[63] = 0;
        bi.parentIdx = parentIdx;
        memcpy(bi.transform, &node->mTransformation, sizeof(float) * 16);
        bool foundOffset = false;
        uint32_t meshCount = m_pAIScene->mNumMeshes;
        for (uint32_t m = 0; m < meshCount; ++m) {
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
    {
        char buf[256];
        sprintf(buf, "[BIN/Anim] Bone Count: %u\n", (uint32_t)bones.size());
        OutputDebugStringA(buf);
        for (uint32_t i = 0; i < bones.size(); ++i) {
            sprintf(buf, "  Bone[%u] Name: %s, Parent: %d\n", i, bones[i].name, bones[i].parentIdx);
            OutputDebugStringA(buf);
        }
    }
    uint32_t totalBoneCount = (uint32_t)bones.size();
    ofs.write((char*)&totalBoneCount, sizeof(totalBoneCount));
    ofs.write((char*)bones.data(), sizeof(BoneInfo) * bones.size());

    // ---- 4. 애니메이션+채널 저장 ----
    {
        char buf[256];
        uint32_t animCount = m_pAIScene->mNumAnimations;
        sprintf(buf, "[BIN/Anim] Animation Count: %u\n", animCount);
        OutputDebugStringA(buf);
        ofs.write((char*)&animCount, sizeof(animCount));
        for (uint32_t a = 0; a < animCount; ++a) {
            aiAnimation* anim = m_pAIScene->mAnimations[a];
            sprintf(buf, "  Anim[%u] Name: %s, Duration: %.3f, Channels: %u\n", a, anim->mName.C_Str(), anim->mDuration, anim->mNumChannels);
            OutputDebugStringA(buf);
            AnimInfo ainfo{};
            strncpy(ainfo.name, anim->mName.C_Str(), 63); ainfo.name[63] = 0;
            ainfo.duration = anim->mDuration;
            ainfo.ticksPerSecond = anim->mTicksPerSecond;
            ainfo.channelCount = anim->mNumChannels;
            ofs.write((char*)&ainfo, sizeof(AnimInfo));
            for (uint32_t c = 0; c < anim->mNumChannels; ++c) {
                aiNodeAnim* chan = anim->mChannels[c];
                sprintf(buf, "    Channel[%u] Bone: %s, Pos:%u Rot:%u Scale:%u\n", c, chan->mNodeName.C_Str(),
                    chan->mNumPositionKeys, chan->mNumRotationKeys, chan->mNumScalingKeys);
                OutputDebugStringA(buf);
                ChannelInfo cinfo{};
                strncpy(cinfo.boneName, chan->mNodeName.C_Str(), 63); cinfo.boneName[63] = 0;
                uint32_t n1 = chan->mNumPositionKeys;
                uint32_t n2 = chan->mNumRotationKeys;
                uint32_t n3 = chan->mNumScalingKeys;
                cinfo.keyframeCount = max3(n1, n2, n3);
                ofs.write((char*)&cinfo, sizeof(ChannelInfo));
                for (uint32_t k = 0; k < cinfo.keyframeCount; ++k) {
                    KeyFrame kf{};
                    if (k < chan->mNumScalingKeys) {
                        kf.time = chan->mScalingKeys[k].mTime;
                        kf.scale[0] = chan->mScalingKeys[k].mValue.x;
                        kf.scale[1] = chan->mScalingKeys[k].mValue.y;
                        kf.scale[2] = chan->mScalingKeys[k].mValue.z;
                    }
                    if (k < chan->mNumRotationKeys) {
                        kf.time = chan->mRotationKeys[k].mTime;
                        kf.rotation[0] = chan->mRotationKeys[k].mValue.x;
                        kf.rotation[1] = chan->mRotationKeys[k].mValue.y;
                        kf.rotation[2] = chan->mRotationKeys[k].mValue.z;
                        kf.rotation[3] = chan->mRotationKeys[k].mValue.w;
                    }
                    if (k < chan->mNumPositionKeys) {
                        kf.time = chan->mPositionKeys[k].mTime;
                        kf.translation[0] = chan->mPositionKeys[k].mValue.x;
                        kf.translation[1] = chan->mPositionKeys[k].mValue.y;
                        kf.translation[2] = chan->mPositionKeys[k].mValue.z;
                    }
                    ofs.write((char*)&kf, sizeof(KeyFrame));
                }
            }
        }
    }

    ofs.close();
    OutputDebugStringA("애니메이션 BIN 저장 완료!\n");
}




void CMainApp::ExportModelToBin_NonAnim(const MapObject& obj, const char* binPath)
{
    char curdir[512] = { 0 };
    _getcwd(curdir, 511);
    OutputDebugStringA("[BIN Export] 현재 작업 디렉토리: ");
    OutputDebugStringA(curdir);
    OutputDebugStringA("\n");

    OutputDebugStringA("[BIN Export] fbxPath: ");
    OutputDebugStringA(obj.fbxPath);
    OutputDebugStringA("\n");
    OutputDebugStringA("[BIN Export] binPath: ");
    OutputDebugStringA(binPath);
    OutputDebugStringA("\n");

    if (FILE* fp = fopen(obj.fbxPath, "rb")) {
        fclose(fp);
    }
    else {
        OutputDebugStringA("[BIN Export] 실패: FBX 파일이 존재하지 않음!\n");
        return;
    }

    _uint iFlag = { aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast | aiProcess_PreTransformVertices };
    m_pAIScene = m_Importer.ReadFile(obj.fbxPath, iFlag);

    if (!m_pAIScene || !m_pAIScene->HasMeshes()) {
        OutputDebugStringA("[BIN Export] 실패: FBX 로드 실패!\n");
        return;
    }

    std::ofstream ofs(binPath, std::ios::binary);
    if (!ofs) {
        OutputDebugStringA("[BIN Export] 실패: BIN 파일 저장 불가!\n");
        return;
    }

    // ---- 1. 메시 저장 ----
    {
        char buf[256];
        uint32_t meshCount = m_pAIScene->mNumMeshes;
        sprintf(buf, "[BIN/NonAnim] Mesh Count: %u\n", meshCount);
        OutputDebugStringA(buf);
        ofs.write((char*)&meshCount, sizeof(meshCount));
        for (uint32_t m = 0; m < meshCount; ++m) {
            aiMesh* mesh = m_pAIScene->mMeshes[m];
            sprintf(buf, "  Mesh[%u] Name: %s  Vertex: %u  Index: %u\n",
                m, mesh->mName.C_Str(), mesh->mNumVertices, mesh->mNumFaces * 3);
            OutputDebugStringA(buf);
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
    }

    // ---- 2. 머테리얼 저장 ----
    {
        char buf[256];
        uint32_t matCount = m_pAIScene->mNumMaterials;
        sprintf(buf, "[BIN/NonAnim] Material Count: %u\n", matCount);
        OutputDebugStringA(buf);
        ofs.write((char*)&matCount, sizeof(matCount));
        for (uint32_t i = 0; i < matCount; ++i) {
            aiMaterial* material = m_pAIScene->mMaterials[i];
            aiString path;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &path);
            sprintf(buf, "  Material[%u] Diffuse: %s\n", i, path.C_Str());
            OutputDebugStringA(buf);
            MaterialInfo matInfo{};
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
                strcpy_s(matInfo.basecolor, path.C_Str());
            if (material->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS)
                strcpy_s(matInfo.normal, path.C_Str());
            for (int j = 0; j < material->GetTextureCount(aiTextureType_UNKNOWN); ++j) {
                if (material->GetTexture(aiTextureType_UNKNOWN, j, &path) == AI_SUCCESS) {
                    if (strstr(path.C_Str(), "ARM") || strstr(path.C_Str(), "arm")) {
                        strcpy_s(matInfo.arm, path.C_Str());
                        break;
                    }
                }
            }
            ofs.write((char*)&matInfo, sizeof(MaterialInfo));
        }
    }

    ofs.close();
    OutputDebugStringA("NonAnim BIN 저장 완료!\n");
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