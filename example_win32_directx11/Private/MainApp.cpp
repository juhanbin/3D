#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "MapObject.h"
#include "Edit_Defines.h"
#include "Graphic_Device.h"
#include <direct.h>
#include "BinType.h"
#include <vector>

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

void CMainApp::GatherBones(const aiNode* node, int parentIdx)
{
    BoneInfoBin bone;
    memset(&bone, 0, sizeof(BoneInfoBin));
    strncpy(bone.Name, node->mName.C_Str(), 63);
    bone.ParentIndex = parentIdx;

    const aiMatrix4x4& m = node->mTransformation;
    float mat[16] = {
        m.a1, m.a2, m.a3, m.a4,
        m.b1, m.b2, m.b3, m.b4,
        m.c1, m.c2, m.c3, m.c4,
        m.d1, m.d2, m.d3, m.d4
    };
    memcpy(bone.Transform, mat, sizeof(float) * 16);

    m_Bones.push_back(bone);
    int myIdx = (int)m_Bones.size() - 1;
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        GatherBones(node->mChildren[i], myIdx);
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
                    strcpy(obj.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster/Monster.fbx");

                    strcpy(obj.binPath, "../../Mapdata/Monster.bin");

                    break;
                case EObjectType::ROCK_AA:
                    // Rock_AA.fbx 예시
                    strcpy(obj.fbxPath, "../Bin/Resources/Blood_Spear/Model/Rock/Rock_AA.fbx");

                    strcpy(obj.binPath, "../../Mapdata/Rock_AA.bin");

                    break;
                case EObjectType::HERO:
                    // Rock_AA.fbx 예시
                    strcpy(obj.fbxPath, "../Bin/Resources/Blood_Spear/Model/Hero/Hero.fbx");

                    strcpy(obj.binPath, "../../Mapdata/Hero.bin");

                    break;
                case EObjectType::SPEAR:
                    // Rock_AA.fbx 예시
                    strcpy(obj.fbxPath, "../Bin/Resources/Blood_Spear/Model/Spear/Spear.fbx");

                    strcpy(obj.binPath, "../../Mapdata/Spear.bin");

                    break;
                case EObjectType::MONSTER_SPEAR:
                    // Rock_AA.fbx 예시
                    strcpy(obj.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster_Spear/Monster_Spear.fbx");

                    strcpy(obj.binPath, "../../Mapdata/Monster_Spear.bin");

                    break;
                case EObjectType::MONSTER_BOW:
                    // Rock_AA.fbx 예시
                    strcpy(obj.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster_Bow/Monster_Bow.fbx");

                    strcpy(obj.binPath, "../../Mapdata/Monster_Bow.bin");

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
    uint32_t count = m_Objects.size();
    ofs.write((const char*)&count, sizeof(count));
    for (const auto& obj : m_Objects)
        ofs.write((const char*)&obj, sizeof(MapObject));

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
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        obj.fbxPath,
        aiProcess_ConvertToLeftHanded |
        aiProcessPreset_TargetRealtime_Fast
    );
    if (!scene) {
        printf("FBX 로드 실패: %s\n", importer.GetErrorString());
        return;
    }

    FILE* fp = fopen(binPath, "wb");
    if (!fp) {
        printf("BIN 파일 열기 실패: %s\n", binPath);
        return;
    }

    // 1. 본
    GatherBones(scene->mRootNode, -1);

    uint32_t boneCount = static_cast<uint32_t>(m_Bones.size());
    fwrite(&boneCount, sizeof(uint32_t), 1, fp);                    //본 개수 저장
    fwrite(m_Bones.data(), sizeof(BoneInfoBin), boneCount, fp);

    // 2. 메시
    const uint32_t numMeshes = scene->mNumMeshes;
    fwrite(&numMeshes, sizeof(uint32_t), 1, fp);

    auto FindGlobalBoneIndex = [&](const char* name)->int {
        for (int gi = 0; gi < (int)m_Bones.size(); ++gi) {
            if (std::strncmp(m_Bones[gi].Name, name, sizeof(m_Bones[gi].Name)) == 0)
                return gi;
        }
        return -1;
        };

    for (uint32_t i = 0; i < numMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[i];

        // 1) MeshInfoBin
        MeshInfoBin info{};
        std::memset(info.Name, 0, sizeof(info.Name));
        std::strncpy(info.Name, mesh->mName.C_Str(), sizeof(info.Name) - 1);
        info.MaterialIndex = mesh->mMaterialIndex;
        info.NumVertices = mesh->mNumVertices;
        info.NumFaces = mesh->mNumFaces;
        info.NumIndices = info.NumFaces * 3;

        // 2) 정점 배열 (기본 속성)
        std::vector<VTXANIMMESH> verts(info.NumVertices);
        const bool hasNormal = mesh->HasNormals();
        const bool hasTB = mesh->HasTangentsAndBitangents();
        const bool hasUV0 = (mesh->HasTextureCoords(0) && mesh->mNumUVComponents[0] >= 2);

        for (uint32_t v = 0; v < info.NumVertices; ++v)
        {
            auto& out = verts[v];

            // Position
            out.vPosition = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);

            // Normal
            out.vNormal = hasNormal
                ? XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z)
                : XMFLOAT3(0, 0, 0);

            // Tangent / Bitangent
            if (hasTB) {
                out.vTangent = XMFLOAT3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
                out.vBinormal = XMFLOAT3(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z);
            }
            else {
                out.vTangent = XMFLOAT3(0, 0, 0);
                out.vBinormal = XMFLOAT3(0, 0, 0);
            }

            // UV (x,y만)
            out.vTexcoord = hasUV0
                ? XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y)
                : XMFLOAT2(0, 0);

            // Blend 초기화
            out.vBlendIndex = XMUINT4(0, 0, 0, 0);
            out.vBlendWeight = XMFLOAT4(0, 0, 0, 0);
        }

        // 3) 인덱스 배열 (삼각형 가정: aiProcess_Triangulate 사용 전제)
        std::vector<uint32_t> indices(info.NumIndices);
        {
            uint32_t w = 0;
            for (uint32_t f = 0; f < info.NumFaces; ++f) {
                const aiFace& face = mesh->mFaces[f];
                indices[w++] = face.mIndices[0];
                indices[w++] = face.mIndices[1];
                indices[w++] = face.mIndices[2];
            }
        }

        // 4) 메시 본 슬롯(이름 + 오프셋 + 전역본인덱스) 및 정점 가중치 수집
        std::vector<MeshBoneRaw> meshBones;
        meshBones.reserve(mesh->mNumBones);

        // 정점별 (전역본, 가중치) 임시 목록
        std::vector<std::vector<std::pair<uint16_t, float>>> vw(info.NumVertices);

        for (uint32_t b = 0; b < mesh->mNumBones; ++b)
        {
            aiBone* aiB = mesh->mBones[b];

            MeshBoneRaw raw{};
            std::memset(raw.Name, 0, sizeof(raw.Name));
            std::strncpy(raw.Name, aiB->mName.C_Str(), sizeof(raw.Name) - 1);

            // Offset(row-major) 그대로 저장 (BIN/엔진에서 Transpose 안 하는 규약)
            const aiMatrix4x4& M = aiB->mOffsetMatrix;
            const float mat[16] = {
                M.a1, M.a2, M.a3, M.a4,
                M.b1, M.b2, M.b3, M.b4,
                M.c1, M.c2, M.c3, M.c4,
                M.d1, M.d2, M.d3, M.d4
            };
            std::memcpy(raw.Offset, mat, sizeof(mat));

            // 전역 본 인덱스 찾기
            raw.GlobalIndex = FindGlobalBoneIndex(raw.Name); // 없으면 -1
            meshBones.push_back(raw);

            // 정점 가중치: 전역 본 인덱스로 수집
            for (uint32_t w = 0; w < aiB->mNumWeights; ++w) {
                const uint32_t vertIdx = aiB->mWeights[w].mVertexId;
                const float    weight = aiB->mWeights[w].mWeight;

                if (vertIdx < vw.size() && weight > 0.f) {
                    vw[vertIdx].emplace_back(static_cast<uint16_t>(b), weight); // ★ 로컬 b 사용
                }
            }
        }

        // 5) 각 정점: Top-4 선별 + 정규화(+ 내림차순 정렬)
        for (uint32_t v = 0; v < info.NumVertices; ++v)
        {
            auto& list = vw[v];
            if (list.empty()) continue;

            // 같은 본 중복 들어온 경우 합치기(선택사항) ? 안전성↑
            {
                std::sort(list.begin(), list.end(),
                    [](auto& L, auto& R) { return L.first < R.first; });
                std::vector<std::pair<uint16_t, float>> merged;
                merged.reserve(list.size());
                for (auto& e : list) {
                    if (!merged.empty() && merged.back().first == e.first)
                        merged.back().second += e.second;
                    else
                        merged.push_back(e);
                }
                list.swap(merged);
            }

            // 가중치 내림차순 정렬
            std::sort(list.begin(), list.end(),
                [](auto& L, auto& R) { return L.second > R.second; });

            // Top-4
            const size_t n = std::min<size_t>(4, list.size());

            float sum = 0.f;
            for (size_t k = 0; k < n; ++k) sum += list[k].second;
            if (sum <= 0.f) continue;
            const float inv = 1.f / sum;

            // 정규화 후 기록(내림차순 유지)
            auto& out = verts[v];
            XMUINT4 bi = { 0,0,0,0 };
            XMFLOAT4 bw = { 0,0,0,0 };
            if (n > 0) { bi.x = list[0].first; bw.x = list[0].second * inv; }
            if (n > 1) { bi.y = list[1].first; bw.y = list[1].second * inv; }
            if (n > 2) { bi.z = list[2].first; bw.z = list[2].second * inv; }
            if (n > 3) { bi.w = list[3].first; bw.w = list[3].second * inv; }
            out.vBlendIndex = bi;
            out.vBlendWeight = bw;
        }

        // ===== 실제 파일 쓰기 =====
        fwrite(&info, sizeof(MeshInfoBin), 1, fp);  // MeshInfo
        if (info.NumVertices) fwrite(verts.data(), sizeof(VTXANIMMESH), info.NumVertices, fp);
        if (info.NumIndices)  fwrite(indices.data(), sizeof(uint32_t), info.NumIndices, fp);

        const uint32_t boneSlotCount = static_cast<uint32_t>(meshBones.size());
        fwrite(&boneSlotCount, sizeof(uint32_t), 1, fp); // per-mesh bone count
        if (boneSlotCount) fwrite(meshBones.data(), sizeof(MeshBoneRaw), boneSlotCount, fp);
    }


    // 3. 머티리얼 정보 저장 
    uint32_t numMaterials = scene->mNumMaterials;
    fwrite(&numMaterials, sizeof(uint32_t), 1, fp);

    vector<MaterialInfoBin2> matInfos(numMaterials);
    for (uint32_t i = 0; i < numMaterials; ++i) {
        aiMaterial* material = scene->mMaterials[i];
        MaterialInfoBin2& matInfo = matInfos[i];
        memset(&matInfo, 0, sizeof(MaterialInfoBin2));
        matInfo.numTextures = 0;

        // Diffuse
        aiString path;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
            matInfo.textures[matInfo.numTextures].type = (int)TextureType::DIFFUSE;
            strncpy(matInfo.textures[matInfo.numTextures].path, path.C_Str(), 259);
            matInfo.textures[matInfo.numTextures].path[259] = 0;
            matInfo.numTextures++;
        }
        // Normal
        if (material->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
            matInfo.textures[matInfo.numTextures].type = (int)TextureType::NORMAL;
            strncpy(matInfo.textures[matInfo.numTextures].path, path.C_Str(), 259);
            matInfo.textures[matInfo.numTextures].path[259] = 0;
            matInfo.numTextures++;
        }
    }
    fwrite(matInfos.data(), sizeof(MaterialInfoBin2), numMaterials, fp);

    // 4. 애니메이션
    uint32_t animCount = scene->mNumAnimations;
    fwrite(&animCount, sizeof(animCount), 1, fp);

    for (uint32_t a = 0; a < animCount; ++a) {
        aiAnimation* anim = scene->mAnimations[a];

        // AnimInfoBin
        AnimInfoBin ainfo{};
        strncpy(ainfo.name, anim->mName.C_Str(), 63);
        ainfo.duration = anim->mDuration;
        ainfo.ticksPerSecond = anim->mTicksPerSecond;
        ainfo.channelCount = anim->mNumChannels;
        fwrite(&ainfo, sizeof(AnimInfoBin), 1, fp);

       // char buf[256];
       // sprintf_s(buf, "[Export] Animation %d: name='%s', duration=%.2f, tickPerSec=%.2f, channelCount=%d\n",
       //     a, ainfo.name, ainfo.duration, ainfo.ticksPerSecond, ainfo.channelCount);
       // OutputDebugStringA(buf);


        for (uint32_t c = 0; c < anim->mNumChannels; ++c) {
            aiNodeAnim* chan = anim->mChannels[c];


            // ChannelInfoBin
            ChannelInfoBin cinfo{};
            strncpy(cinfo.boneName, chan->mNodeName.C_Str(), 63);
            cinfo.boneName[63] = 0;
            uint32_t nScale = chan->mNumScalingKeys;
            uint32_t nRot = chan->mNumRotationKeys;
            uint32_t nPos = chan->mNumPositionKeys;

            uint32_t max12 = (nScale > nRot) ? nScale : nRot;
            cinfo.keyframeCount = (max12 > nPos) ? max12 : nPos;

            fwrite(&cinfo, sizeof(ChannelInfoBin), 1, fp);

           // sprintf_s(buf, "  [Export] Channel %d: bone='%s', keyframeCount=%d\n", c, cinfo.boneName, cinfo.keyframeCount);
           // OutputDebugStringA(buf);

            KEYFRAME kf = {};

            // KEYFRAME 저장
            for (uint32_t k = 0; k < cinfo.keyframeCount; ++k) {
                // 스케일
                if (k < nScale) {
                    kf.vScale.x = static_cast<float>(chan->mScalingKeys[k].mValue.x);
                    kf.vScale.y = static_cast<float>(chan->mScalingKeys[k].mValue.y);
                    kf.vScale.z = static_cast<float>(chan->mScalingKeys[k].mValue.z);
                    kf.fTrackPosition = static_cast<float>(chan->mScalingKeys[k].mTime);
                }
                // 회전
                if (k < nRot) {
                    kf.vRotation.x = static_cast<float>(chan->mRotationKeys[k].mValue.x);
                    kf.vRotation.y = static_cast<float>(chan->mRotationKeys[k].mValue.y);
                    kf.vRotation.z = static_cast<float>(chan->mRotationKeys[k].mValue.z);
                    kf.vRotation.w = static_cast<float>(chan->mRotationKeys[k].mValue.w);
                    kf.fTrackPosition = static_cast<float>(chan->mRotationKeys[k].mTime);
                }
                // 이동
                if (k < nPos) {
                    kf.vTranslation.x = static_cast<float>(chan->mPositionKeys[k].mValue.x);
                    kf.vTranslation.y = static_cast<float>(chan->mPositionKeys[k].mValue.y);
                    kf.vTranslation.z = static_cast<float>(chan->mPositionKeys[k].mValue.z);
                    kf.fTrackPosition = static_cast<float>(chan->mPositionKeys[k].mTime);
                }
                //printf("sizeof(ChannelInfoBin) = %zu\n", sizeof(ChannelInfoBin));
                char buf[256];
                sprintf_s(buf, "sizeof(ChannelInfoBin) = %zu\n", sizeof(ChannelInfoBin));
                OutputDebugStringA(buf);
               // sprintf_s(buf, "    [Export] KeyFrame %d: scale=(%.2f,%.2f,%.2f) rot=(%.2f,%.2f,%.2f,%.2f) trans=(%.2f,%.2f,%.2f) time=%.2f\n",
               //     k, kf.vScale.x, kf.vScale.y, kf.vScale.z, kf.vRotation.x, kf.vRotation.y, kf.vRotation.z, kf.vRotation.w,
               //     kf.vTranslation.x, kf.vTranslation.y, kf.vTranslation.z, kf.fTrackPosition);
               // OutputDebugStringA(buf);

                fwrite(&kf, sizeof(KEYFRAME), 1, fp);
            }
        }

    }
    fclose(fp);
    printf("BIN에 뼈 정보 저장 완료!\n");
}

void CMainApp::ExportModelToBin_NonAnim(const MapObject& obj, const char* binPath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        obj.fbxPath,
        aiProcess_ConvertToLeftHanded |
        aiProcessPreset_TargetRealtime_Fast |
        aiProcess_PreTransformVertices
    );

    if (!scene) {
        printf("FBX 로드 실패: %s\n", importer.GetErrorString());
        return;
    }

    uint32_t numMeshes = scene->mNumMeshes;

    FILE* fp = fopen(binPath, "wb");
    if (!fp) {
        printf("BIN 파일 열기 실패: %s\n", binPath);
        return;
    }

    // 1. 메시 개수 저장
    fwrite(&numMeshes, sizeof(uint32_t), 1, fp);

    // 2. 메시 정보, 버텍스/인덱스 데이터 준비
    vector<MeshInfoBin> meshInfos(numMeshes);
    vector<vector<VTXMESH>> allVertices(numMeshes);
    vector<vector<uint32_t>> allIndices(numMeshes);

    for (uint32_t i = 0; i < numMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        MeshInfoBin& info = meshInfos[i];
        memset(&info, 0, sizeof(MeshInfoBin));
        strncpy(info.Name, mesh->mName.C_Str(), 63);
        info.MaterialIndex = mesh->mMaterialIndex;
        info.NumVertices = mesh->mNumVertices;
        info.NumFaces = mesh->mNumFaces;
        info.NumIndices = mesh->mNumFaces * 3;

        // 버텍스 데이터 저장
        auto& vertices = allVertices[i];
        vertices.resize(info.NumVertices);

        for (uint32_t v = 0; v < info.NumVertices; ++v) {
            VTXMESH& vert = vertices[v];
            vert.vPosition.x = mesh->mVertices[v].x;
            vert.vPosition.y = mesh->mVertices[v].y;
            vert.vPosition.z = mesh->mVertices[v].z;

            // 노멀: 있으면 저장, 없으면 0
            if (mesh->HasNormals()) {
                vert.vNormal.x = mesh->mNormals[v].x;
                vert.vNormal.y = mesh->mNormals[v].y;
                vert.vNormal.z = mesh->mNormals[v].z;
            }
            else {
                vert.vNormal.x = vert.vNormal.y = vert.vNormal.z = 0.0f;
            }
            // 탄젠트/바이노멀: 있으면 저장, 없으면 0
            if (mesh->HasTangentsAndBitangents()) {
                vert.vTangent.x = mesh->mTangents[v].x;
                vert.vTangent.y = mesh->mTangents[v].y;
                vert.vTangent.z = mesh->mTangents[v].z;

                vert.vBinormal.x = mesh->mBitangents[v].x;
                vert.vBinormal.y = mesh->mBitangents[v].y;
                vert.vBinormal.z = mesh->mBitangents[v].z;
            }
            else {
                vert.vTangent.x = vert.vTangent.y = vert.vTangent.z = 0.0f;
                vert.vBinormal.x = vert.vBinormal.y = vert.vBinormal.z = 0.0f;
            }
            // UV: 있으면 저장, 없으면 0
            if (mesh->HasTextureCoords(0)) {
                vert.vTexcoord.x = mesh->mTextureCoords[0][v].x;
                vert.vTexcoord.y = mesh->mTextureCoords[0][v].y;
            }
            else {
                vert.vTexcoord.x = vert.vTexcoord.y = 0.0f;
            }
        }

        // 인덱스 데이터 저장
        auto& indices = allIndices[i];
        indices.resize(info.NumIndices);
        uint32_t idx = 0;
        for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
            aiFace& face = mesh->mFaces[f];
            indices[idx++] = face.mIndices[0];
            indices[idx++] = face.mIndices[1];
            indices[idx++] = face.mIndices[2];
        }
    }

    // 3. 메시 정보 배열 저장
    for (uint32_t i = 0; i < numMeshes; ++i)
        fwrite(&meshInfos[i], sizeof(MeshInfoBin), 1, fp);

    // 4. 각 메시의 버텍스 배열 저장
    for (uint32_t i = 0; i < numMeshes; ++i)
        fwrite(allVertices[i].data(), sizeof(VTXMESH), meshInfos[i].NumVertices, fp);

    // 5. 각 메시의 인덱스 배열 저장
    for (uint32_t i = 0; i < numMeshes; ++i)
        fwrite(allIndices[i].data(), sizeof(uint32_t), meshInfos[i].NumIndices, fp);

    // ---- 6. 머티리얼 정보 저장 ----
    uint32_t numMaterials = scene->mNumMaterials;
    fwrite(&numMaterials, sizeof(uint32_t), 1, fp);

    vector<MaterialInfoBin2> matInfos(numMaterials);
    for (uint32_t i = 0; i < numMaterials; ++i) {
        aiMaterial* material = scene->mMaterials[i];
        MaterialInfoBin2& matInfo = matInfos[i];
        memset(&matInfo, 0, sizeof(MaterialInfoBin2));
        matInfo.numTextures = 0;

        // Diffuse
        aiString path;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
            matInfo.textures[matInfo.numTextures].type = (int)TextureType::DIFFUSE;
            strncpy(matInfo.textures[matInfo.numTextures].path, path.C_Str(), 259);
            matInfo.textures[matInfo.numTextures].path[259] = 0;
            matInfo.numTextures++;
        }
        // Normal
        if (material->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
            matInfo.textures[matInfo.numTextures].type = (int)TextureType::NORMAL;
            strncpy(matInfo.textures[matInfo.numTextures].path, path.C_Str(), 259);
            matInfo.textures[matInfo.numTextures].path[259] = 0;
            matInfo.numTextures++;
        }
    }
    fwrite(matInfos.data(), sizeof(MaterialInfoBin2), numMaterials, fp);

    fclose(fp);
    printf("BIN 파일로 저장 완료: %s\n", binPath);
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