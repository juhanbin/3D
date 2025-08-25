#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "MapObject.h"
#include "Edit_Defines.h"
#include "Graphic_Device.h"
#include <direct.h>
#include "BinType.h"
#include <vector>
#include "Picking.h"
#include <cfloat>

using namespace DirectX;
using namespace Edit;

NS_BEGIN(Edit)

CMainApp::CMainApp()
    : m_pGameInstance{ Engine::CGameInstance::GetInstance() } {
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

    if (FAILED(m_pGameInstance->Initialize_Engine(EngineDesc, &m_pDevice, &m_pContext))) return E_FAIL;
    if (FAILED(Ready_Prototype_ForStatic())) return E_FAIL;
    if (FAILED(Start_Level(LEVEL::EDIT)))    return E_FAIL;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(m_pDevice, m_pContext);

    CGraphic_Device* pGD = m_pGameInstance->GetGraphicDevice();
    m_pBackBufferRTV = pGD->GetBackBufferRTV();
    m_pDepthStencilView = pGD->GetDepthStencilView();
    return S_OK;
}

void CMainApp::Update(_float fTimeDelta) { m_pGameInstance->Update_Engine(fTimeDelta); }

HRESULT CMainApp::Render()
{
    _float4 clr = _float4(0.f, 0.f, 1.f, 1.f);
    m_pGameInstance->Render_Begin(&clr);
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
        CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements)))) return E_FAIL;
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_BG"),
        CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex_BG.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements)))) return E_FAIL;
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_Logo"),
        CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex_Logo.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements)))) return E_FAIL;
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_Fade"),
        CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex_Fade.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements)))) return E_FAIL;
    if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        CVIBuffer_Rect::Create(m_pDevice, m_pContext)))) return E_FAIL;

    return S_OK;
}

HRESULT CMainApp::Start_Level(LEVEL eStartLevelID)
{
    if (FAILED(m_pGameInstance->Open_Level(static_cast<_uint>(LEVEL::LOADING),
        CLevel_Loading::Create(m_pDevice, m_pContext, eStartLevelID)))) return E_FAIL;
    return S_OK;
}

// ===== bones (원본 유지)
void CMainApp::GatherBones(const aiNode* node, int parentIdx)
{
    if (!node) return;
    BoneInfoBin bone{}; std::memset(&bone, 0, sizeof(BoneInfoBin));
    std::strncpy(bone.Name, node->mName.C_Str(), sizeof(bone.Name) - 1);
    bone.ParentIndex = parentIdx;

    const aiMatrix4x4& m = node->mTransformation;
    const float mat[16] = { m.a1,m.a2,m.a3,m.a4, m.b1,m.b2,m.b3,m.b4, m.c1,m.c2,m.c3,m.c4, m.d1,m.d2,m.d3,m.d4 };
    std::memcpy(bone.Transform, mat, sizeof(mat));

    m_Bones.push_back(bone);
    int myIdx = (int)m_Bones.size() - 1;
    for (unsigned i = 0; i < node->mNumChildren; ++i) GatherBones(node->mChildren[i], myIdx);
}

XMMATRIX CMainApp::MakeWorld(const MapObject& o) const
{
    XMMATRIX S = XMMatrixScaling(o.size[0], o.size[1], o.size[2]);
    XMMATRIX R = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(o.rot[0]),
        XMConvertToRadians(o.rot[1]),
        XMConvertToRadians(o.rot[2]));
    XMMATRIX T = XMMatrixTranslation(o.pos[0], o.pos[1], o.pos[2]);
    return S * R * T;
}

bool CMainApp::RaycastObject_AABB(const XMVECTOR& rayPosW, const XMVECTOR& rayDirW,
    const MapObject& o, float& outWorldDist, XMFLOAT3& outHitW) const
{
    BoundingBox localBB;
    localBB.Center = XMFLOAT3(0, 0, 0);
    localBB.Extents = XMFLOAT3(o.size[0] * 0.5f, o.size[1] * 0.5f, o.size[2] * 0.5f);

    XMMATRIX W = MakeWorld(o);
    XMMATRIX iW = XMMatrixInverse(nullptr, W);
    XMVECTOR ro = XMVector3TransformCoord(rayPosW, iW);
    XMVECTOR rd = XMVector3Normalize(XMVector3TransformNormal(rayDirW, iW));

    float tLocal = 0.f;
    if (!localBB.Intersects(ro, rd, tLocal)) return false;

    XMVECTOR pW = XMVector3TransformCoord(ro + rd * tLocal, W);
    XMStoreFloat3(&outHitW, pW);
    outWorldDist = XMVectorGetX(XMVector3Length(pW - rayPosW));
    return true;
}

// ★ 가장 가까운 피킹 결과(CMapObject OBB → 실패 시 바닥)
bool CMainApp::PickPoint_OBB(DirectX::XMFLOAT3& outHitW, int& outObjIdx)
{
    using namespace DirectX;
    const _float3& rp = m_pGameInstance->Get_RayPos();
    const _float3& rd = m_pGameInstance->Get_RayDir();
    XMVECTOR rayPosW = XMLoadFloat3(&rp);
    XMVECTOR rayDirW = XMVector3Normalize(XMLoadFloat3(&rd));

    int   bestIdx = -1;
    float bestT = FLT_MAX;
    XMFLOAT3 bestHit{};

    // ===== 실제 씬의 오브젝트들 대상으로 검사 =====
    const auto& all = CMapObject::All();
    for (int i = 0; i < (int)all.size(); ++i)
    {
        float t; XMFLOAT3 hit{};
        if (all[i]->RaycastBounds(rayPosW, rayDirW, t, hit))
        {
            if (t > 0.f && t < bestT) { bestT = t; bestIdx = i; bestHit = hit; }
        }
    }

    if (bestIdx >= 0) { outHitW = bestHit; outObjIdx = bestIdx; return true; }

    // 오브젝트 미히트 → 바닥(y=0)
    XMFLOAT3 g{};
    if (RaycastGround(rayPosW, rayDirW, g)) { outHitW = g; outObjIdx = -1; return true; }

    return false;
}


bool CMainApp::RaycastGround(const XMVECTOR& rayPosW, const XMVECTOR& rayDirW, XMFLOAT3& outHitW) const
{
    const float yPlane = 0.f;
    float oy = XMVectorGetY(rayPosW);
    float dy = XMVectorGetY(rayDirW);
    if (fabsf(dy) < 1e-6f) return false;
    float t = (yPlane - oy) / dy;
    if (t <= 0.f) return false;
    XMStoreFloat3(&outHitW, rayPosW + rayDirW * t);
    return true;
}



/* ====================== ImGui / Input ====================== */
void CMainApp::Render_ImGuiPanel()
{
    ImGui::Begin("Map Tool Panel");

    if (ImGui::Button("Create Object")) ImGui::OpenPopup("CreateObjectPopup");
    ImGui::SameLine();
    if (ImGui::Button("Undo") && !m_UndoStack.empty()) {
        m_Objects = m_UndoStack.back(); m_UndoStack.pop_back(); m_Selected = -1; RefreshScene();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) { LoadScene("../../Mapdata/scene.bin"); RefreshScene(); }
    ImGui::SameLine();
    if (ImGui::Button("Save")) { SaveScene("../../Mapdata/scene.bin"); }

    if (ImGui::BeginPopup("CreateObjectPopup")) {
        ImGui::Text("Select Object Type"); ImGui::Separator();
        for (int i = 0; i < NumObjectTypes; ++i) {
            EObjectType type = (EObjectType)i;
            if (ImGui::Selectable(ToObjectTypeString(type))) {
                PushUndo();
                MapObject o{}; o.id = (int)m_Objects.size(); o.type = type;
                o.size[0] = o.size[1] = o.size[2] = 1.f; o.rot[0] = o.rot[1] = o.rot[2] = 0.f; o.pos[0] = o.pos[1] = o.pos[2] = 0.f;
                switch (type) {
                case EObjectType::MONSTER:      strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster/Monster.fbx");       strcpy(o.binPath, "../../Mapdata/Monster.bin");       break;
                case EObjectType::ROCK_AA:      strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Rock/Rock_AA.fbx");          strcpy(o.binPath, "../../Mapdata/Rock_AA.bin");       break;
                case EObjectType::HERO:         strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Hero/Hero.fbx");             strcpy(o.binPath, "../../Mapdata/Hero.bin");          break;
                case EObjectType::SPEAR:        strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Spear/Spear.fbx");           strcpy(o.binPath, "../../Mapdata/Spear.bin");         break;
                case EObjectType::MONSTER_SPEAR:strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster_Spear/Monster_Spear.fbx"); strcpy(o.binPath, "../../Mapdata/Monster_Spear.bin"); break;
                case EObjectType::MONSTER_BOW:  strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster_Bow/Monster_Bow.fbx");      strcpy(o.binPath, "../../Mapdata/Monster_Bow.bin");   break;
                case EObjectType::BRIDGE:       strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Bridge/Bridge.fbx");        strcpy(o.binPath, "../../Mapdata/Bridge.bin");        break;
                case EObjectType::CAVE:         strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Cave/Cave.fbx");            strcpy(o.binPath, "../../Mapdata/Cave.bin");          break;
                default: o.fbxPath[0] = o.binPath[0] = 0; break;
                }
                m_Objects.push_back(o); RefreshScene(); ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();
    ImGui::Checkbox("Enable Pick Debug", &m_PickDebugEnabled);
    if (m_LastPickValid) {
        ImGui::Text("Picked Pos: (%.3f, %.3f, %.3f)%s", m_LastPickPos.x, m_LastPickPos.y, m_LastPickPos.z,
            m_LastPickObj >= 0 ? "" : "  (ground)");
        if (ImGui::Button("Copy XYZ")) {
            char buf[128]; std::snprintf(buf, sizeof(buf), "%.6f %.6f %.6f", m_LastPickPos.x, m_LastPickPos.y, m_LastPickPos.z);
            ImGui::SetClipboardText(buf);
        }
    }
    else {
        ImGui::TextUnformatted("No pick yet.");
    }

    // ★ 클릭 → 레이-OBB 피킹
    if (m_PickDebugEnabled && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
        XMFLOAT3 hit; int idx;
        if (PickPoint_OBB(hit, idx)) {
            m_LastPickPos = hit; m_LastPickValid = true; m_LastPickObj = idx;
            char dbg[256];
            if (idx >= 0) std::snprintf(dbg, sizeof(dbg), "[Pick] obj=%d  pos=(%.3f, %.3f, %.3f)\n", idx, hit.x, hit.y, hit.z);
            else        std::snprintf(dbg, sizeof(dbg), "[Pick] ground  pos=(%.3f, %.3f, %.3f)\n", hit.x, hit.y, hit.z);
            OutputDebugStringA(dbg);
        }
        else {
            m_LastPickValid = false; m_LastPickObj = -1;
        }
    }

    ImGui::Separator();
    for (int i = 0; i < (int)m_Objects.size(); ++i) {
        char label[64]; std::snprintf(label, sizeof(label), "[%s] Object %d", ToObjectTypeString(m_Objects[i].type), m_Objects[i].id);
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
        ImGui::Text("Size");     ImGui::DragFloat3("x/y/z##size", m_TempSize, 0.1f);
        ImGui::Text("Rotation"); ImGui::DragFloat3("x/y/z##rot", m_TempRot, 0.1f);
        ImGui::Text("Position"); ImGui::DragFloat3("x/y/z##pos", m_TempPos, 0.1f);
        ImGui::InputText("BIN Path", m_Objects[m_Selected].binPath, 260);

        if (ImGui::Button("Delete")) { PushUndo(); m_Objects.erase(m_Objects.begin() + m_Selected); m_Selected = -1; RefreshScene(); }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            PushUndo(); std::memcpy(m_Objects[m_Selected].size, m_TempSize, sizeof(float) * 3);
            std::memcpy(m_Objects[m_Selected].rot, m_TempRot, sizeof(float) * 3);
            std::memcpy(m_Objects[m_Selected].pos, m_TempPos, sizeof(float) * 3); RefreshScene();
        }
        if (ImGui::Button("Export_NonAnim")) { ExportModelToBin_NonAnim(m_Objects[m_Selected], m_Objects[m_Selected].binPath); }
        if (ImGui::Button("Export_Anim")) { ExportModelToBin_Anim(m_Objects[m_Selected], m_Objects[m_Selected].binPath); }
    }
    ImGui::End();
}

/* ====================== 저장/로드 & 기타 (원본 유지) ====================== */
void CMainApp::SaveScene(const char* filename)
{
    std::ofstream ofs(filename, std::ios::binary);
    uint32_t count = (uint32_t)m_Objects.size();
    ofs.write((const char*)&count, sizeof(count));
    for (const auto& obj : m_Objects) ofs.write((const char*)&obj, sizeof(MapObject));
}

bool CMainApp::LoadScene(const char* filename)
{
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return false;
    size_t count = 0; ifs.read((char*)&count, sizeof(count)); if (!ifs) return false;
    m_Objects.clear();
    for (size_t i = 0; i < count; ++i) { MapObject obj{}; ifs.read((char*)&obj, sizeof(MapObject)); m_Objects.push_back(obj); }
    m_Selected = -1;
    return true;
}

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

    // 1) 본(스켈레톤) 저장
    GatherBones(scene->mRootNode, -1);

    uint32_t boneCount = static_cast<uint32_t>(m_Bones.size());
    fwrite(&boneCount, sizeof(uint32_t), 1, fp);
    fwrite(m_Bones.data(), sizeof(BoneInfoBin), boneCount, fp);

    // 2) 메시 저장
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

        // MeshInfo
        MeshInfoBin info{};
        std::memset(info.Name, 0, sizeof(info.Name));
        std::strncpy(info.Name, mesh->mName.C_Str(), sizeof(info.Name) - 1);
        info.MaterialIndex = mesh->mMaterialIndex;
        info.NumVertices = mesh->mNumVertices;
        info.NumFaces = mesh->mNumFaces;
        info.NumIndices = info.NumFaces * 3;

        // 정점 배열
        std::vector<VTXANIMMESH> verts(info.NumVertices);
        const bool hasNormal = mesh->HasNormals();
        const bool hasTB = mesh->HasTangentsAndBitangents();
        const bool hasUV0 = (mesh->HasTextureCoords(0) && mesh->mNumUVComponents[0] >= 2);

        for (uint32_t v = 0; v < info.NumVertices; ++v)
        {
            auto& out = verts[v];

            out.vPosition = XMFLOAT3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);

            out.vNormal = hasNormal
                ? XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z)
                : XMFLOAT3(0, 0, 0);

            if (hasTB) {
                out.vTangent = XMFLOAT3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
                out.vBinormal = XMFLOAT3(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z);
            }
            else {
                out.vTangent = XMFLOAT3(0, 0, 0);
                out.vBinormal = XMFLOAT3(0, 0, 0);
            }

            out.vTexcoord = hasUV0
                ? XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y)
                : XMFLOAT2(0, 0);

            out.vBlendIndex = XMUINT4(0, 0, 0, 0);
            out.vBlendWeight = XMFLOAT4(0, 0, 0, 0);
        }

        // 인덱스 배열
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

        // per-mesh bone 슬롯 및 정점 가중치
        std::vector<MeshBoneRaw> meshBones;
        meshBones.reserve(mesh->mNumBones);

        // 정점별 (본,가중치) 임시 리스트
        std::vector<std::vector<std::pair<uint16_t, float>>> vw(info.NumVertices);

        for (uint32_t b = 0; b < mesh->mNumBones; ++b)
        {
            aiBone* aiB = mesh->mBones[b];

            MeshBoneRaw raw{};
            std::memset(raw.Name, 0, sizeof(raw.Name));
            std::strncpy(raw.Name, aiB->mName.C_Str(), sizeof(raw.Name) - 1);

            const aiMatrix4x4& M = aiB->mOffsetMatrix;
            const float mat[16] = {
                M.a1, M.a2, M.a3, M.a4,
                M.b1, M.b2, M.b3, M.b4,
                M.c1, M.c2, M.c3, M.c4,
                M.d1, M.d2, M.d3, M.d4
            };
            std::memcpy(raw.Offset, mat, sizeof(mat));

            raw.GlobalIndex = FindGlobalBoneIndex(raw.Name); // 없으면 -1
            meshBones.push_back(raw);

            for (uint32_t w = 0; w < aiB->mNumWeights; ++w) {
                const uint32_t vertIdx = aiB->mWeights[w].mVertexId;
                const float    weight = aiB->mWeights[w].mWeight;
                if (vertIdx < vw.size() && weight > 0.f)
                    vw[vertIdx].emplace_back(static_cast<uint16_t>(b), weight);
            }
        }

        // 정점별 Top-4 가중치
        for (uint32_t v = 0; v < info.NumVertices; ++v)
        {
            auto& list = vw[v];
            if (list.empty()) continue;

            // 같은 본 합치기(선택)
            {
                std::sort(list.begin(), list.end(),
                    [](auto& L, auto& R) { return L.first < R.first; });
                std::vector<std::pair<uint16_t, float>> merged;
                for (auto& e : list) {
                    if (!merged.empty() && merged.back().first == e.first)
                        merged.back().second += e.second;
                    else
                        merged.push_back(e);
                }
                list.swap(merged);
            }

            // 가중치 내림차순
            std::sort(list.begin(), list.end(),
                [](auto& L, auto& R) { return L.second > R.second; });

            const size_t n = std::min<size_t>(4, list.size());
            float sum = 0.f;
            for (size_t k = 0; k < n; ++k) sum += list[k].second;
            if (sum <= 0.f) continue;

            const float inv = 1.f / sum;
            auto& out = verts[v];
            XMUINT4  bi = { 0,0,0,0 };
            XMFLOAT4 bw = { 0,0,0,0 };
            if (n > 0) { bi.x = list[0].first; bw.x = list[0].second * inv; }
            if (n > 1) { bi.y = list[1].first; bw.y = list[1].second * inv; }
            if (n > 2) { bi.z = list[2].first; bw.z = list[2].second * inv; }
            if (n > 3) { bi.w = list[3].first; bw.w = list[3].second * inv; }
            out.vBlendIndex = bi;
            out.vBlendWeight = bw;
        }

        // 파일 쓰기
        fwrite(&info, sizeof(MeshInfoBin), 1, fp);
        if (info.NumVertices) fwrite(verts.data(), sizeof(VTXANIMMESH), info.NumVertices, fp);
        if (info.NumIndices)  fwrite(indices.data(), sizeof(uint32_t), info.NumIndices, fp);

        const uint32_t boneSlotCount = static_cast<uint32_t>(meshBones.size());
        fwrite(&boneSlotCount, sizeof(uint32_t), 1, fp);
        if (boneSlotCount) fwrite(meshBones.data(), sizeof(MeshBoneRaw), boneSlotCount, fp);
    }

    // 3) 머티리얼
    uint32_t numMaterials = scene->mNumMaterials;
    fwrite(&numMaterials, sizeof(uint32_t), 1, fp);

    vector<MaterialInfoBin2> matInfos(numMaterials);
    for (uint32_t i = 0; i < numMaterials; ++i) {
        aiMaterial* material = scene->mMaterials[i];
        MaterialInfoBin2& matInfo = matInfos[i];
        std::memset(&matInfo, 0, sizeof(MaterialInfoBin2));
        matInfo.numTextures = 0;

        aiString path;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
            matInfo.textures[matInfo.numTextures].type = (int)TextureType::DIFFUSE;
            std::strncpy(matInfo.textures[matInfo.numTextures].path, path.C_Str(), 259);
            matInfo.textures[matInfo.numTextures].path[259] = 0;
            matInfo.numTextures++;
        }
        if (material->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
            matInfo.textures[matInfo.numTextures].type = (int)TextureType::NORMAL;
            std::strncpy(matInfo.textures[matInfo.numTextures].path, path.C_Str(), 259);
            matInfo.textures[matInfo.numTextures].path[259] = 0;
            matInfo.numTextures++;
        }
    }
    fwrite(matInfos.data(), sizeof(MaterialInfoBin2), numMaterials, fp);

    // 4) 애니메이션
    uint32_t animCount = scene->mNumAnimations;
    fwrite(&animCount, sizeof(animCount), 1, fp);

    for (uint32_t a = 0; a < animCount; ++a) {
        aiAnimation* anim = scene->mAnimations[a];

        AnimInfoBin ainfo{};
        std::strncpy(ainfo.name, anim->mName.C_Str(), 63);
        ainfo.duration = (float)anim->mDuration;
        ainfo.ticksPerSecond = (float)anim->mTicksPerSecond;
        ainfo.channelCount = anim->mNumChannels;
        fwrite(&ainfo, sizeof(AnimInfoBin), 1, fp);

        for (uint32_t c = 0; c < anim->mNumChannels; ++c) {
            aiNodeAnim* chan = anim->mChannels[c];

            ChannelInfoBin cinfo{};
            std::strncpy(cinfo.boneName, chan->mNodeName.C_Str(), 63);
            cinfo.boneName[63] = 0;
            uint32_t nScale = chan->mNumScalingKeys;
            uint32_t nRot = chan->mNumRotationKeys;
            uint32_t nPos = chan->mNumPositionKeys;

            uint32_t max12 = (nScale > nRot) ? nScale : nRot;
            cinfo.keyframeCount = (max12 > nPos) ? max12 : nPos;

            fwrite(&cinfo, sizeof(ChannelInfoBin), 1, fp);

            KEYFRAME kf{};
            for (uint32_t k = 0; k < cinfo.keyframeCount; ++k) {
                if (k < nScale) {
                    kf.vScale.x = (float)chan->mScalingKeys[k].mValue.x;
                    kf.vScale.y = (float)chan->mScalingKeys[k].mValue.y;
                    kf.vScale.z = (float)chan->mScalingKeys[k].mValue.z;
                    kf.fTrackPosition = (float)chan->mScalingKeys[k].mTime;
                }
                if (k < nRot) {
                    kf.vRotation.x = (float)chan->mRotationKeys[k].mValue.x;
                    kf.vRotation.y = (float)chan->mRotationKeys[k].mValue.y;
                    kf.vRotation.z = (float)chan->mRotationKeys[k].mValue.z;
                    kf.vRotation.w = (float)chan->mRotationKeys[k].mValue.w;
                    kf.fTrackPosition = (float)chan->mRotationKeys[k].mTime;
                }
                if (k < nPos) {
                    kf.vTranslation.x = (float)chan->mPositionKeys[k].mValue.x;
                    kf.vTranslation.y = (float)chan->mPositionKeys[k].mValue.y;
                    kf.vTranslation.z = (float)chan->mPositionKeys[k].mValue.z;
                    kf.fTrackPosition = (float)chan->mPositionKeys[k].mTime;
                }
                fwrite(&kf, sizeof(KEYFRAME), 1, fp);
            }
        }
    }

    fclose(fp);
    printf("BIN에 뼈/애니메이션 정보 저장 완료!\n");
}

// ===== Export: Non-Anim =====
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

    // 1) 메시 개수
    fwrite(&numMeshes, sizeof(uint32_t), 1, fp);

    vector<MeshInfoBin>          meshInfos(numMeshes);
    vector<vector<VTXMESH>>      allVertices(numMeshes);
    vector<vector<uint32_t>>     allIndices(numMeshes);

    for (uint32_t i = 0; i < numMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        MeshInfoBin& info = meshInfos[i];
        std::memset(&info, 0, sizeof(MeshInfoBin));
        std::strncpy(info.Name, mesh->mName.C_Str(), 63);
        info.MaterialIndex = mesh->mMaterialIndex;
        info.NumVertices = mesh->mNumVertices;
        info.NumFaces = mesh->mNumFaces;
        info.NumIndices = mesh->mNumFaces * 3;

        auto& vertices = allVertices[i];
        vertices.resize(info.NumVertices);

        for (uint32_t v = 0; v < info.NumVertices; ++v) {
            VTXMESH& vert = vertices[v];
            vert.vPosition.x = mesh->mVertices[v].x;
            vert.vPosition.y = mesh->mVertices[v].y;
            vert.vPosition.z = mesh->mVertices[v].z;

            if (mesh->HasNormals()) {
                vert.vNormal.x = mesh->mNormals[v].x;
                vert.vNormal.y = mesh->mNormals[v].y;
                vert.vNormal.z = mesh->mNormals[v].z;
            }
            else {
                vert.vNormal = XMFLOAT3(0, 0, 0);
            }

            if (mesh->HasTangentsAndBitangents()) {
                vert.vTangent.x = mesh->mTangents[v].x;
                vert.vTangent.y = mesh->mTangents[v].y;
                vert.vTangent.z = mesh->mTangents[v].z;
                vert.vBinormal.x = mesh->mBitangents[v].x;
                vert.vBinormal.y = mesh->mBitangents[v].y;
                vert.vBinormal.z = mesh->mBitangents[v].z;
            }
            else {
                vert.vTangent = XMFLOAT3(0, 0, 0);
                vert.vBinormal = XMFLOAT3(0, 0, 0);
            }

            if (mesh->HasTextureCoords(0)) {
                vert.vTexcoord.x = mesh->mTextureCoords[0][v].x;
                vert.vTexcoord.y = mesh->mTextureCoords[0][v].y;
            }
            else {
                vert.vTexcoord = XMFLOAT2(0, 0);
            }
        }

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

    // 2) MeshInfo들
    for (uint32_t i = 0; i < numMeshes; ++i)
        fwrite(&meshInfos[i], sizeof(MeshInfoBin), 1, fp);

    // 3) 각 메시의 버텍스
    for (uint32_t i = 0; i < numMeshes; ++i)
        fwrite(allVertices[i].data(), sizeof(VTXMESH), meshInfos[i].NumVertices, fp);

    // 4) 각 메시의 인덱스
    for (uint32_t i = 0; i < numMeshes; ++i)
        fwrite(allIndices[i].data(), sizeof(uint32_t), meshInfos[i].NumIndices, fp);

    // 5) 머티리얼
    uint32_t numMaterials = scene->mNumMaterials;
    fwrite(&numMaterials, sizeof(uint32_t), 1, fp);

    vector<MaterialInfoBin2> matInfos(numMaterials);
    for (uint32_t i = 0; i < numMaterials; ++i) {
        aiMaterial* material = scene->mMaterials[i];
        MaterialInfoBin2& matInfo = matInfos[i];
        std::memset(&matInfo, 0, sizeof(MaterialInfoBin2));
        matInfo.numTextures = 0;

        aiString path;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
            matInfo.textures[matInfo.numTextures].type = (int)TextureType::DIFFUSE;
            std::strncpy(matInfo.textures[matInfo.numTextures].path, path.C_Str(), 259);
            matInfo.textures[matInfo.numTextures].path[259] = 0;
            matInfo.numTextures++;
        }
        if (material->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
            matInfo.textures[matInfo.numTextures].type = (int)TextureType::NORMAL;
            std::strncpy(matInfo.textures[matInfo.numTextures].path, path.C_Str(), 259);
            matInfo.textures[matInfo.numTextures].path[259] = 0;
            matInfo.numTextures++;
        }
    }
    fwrite(matInfos.data(), sizeof(MaterialInfoBin2), numMaterials, fp);

    fclose(fp);
    printf("BIN 파일로 저장 완료: %s\n", binPath);
}

// ===== Undo =====
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
        CMapObject::MAPOBJECT_DESC d{};
        d.type = o.type;
        d.vScale = _float3(o.size[0], o.size[1], o.size[2]);
        d.vRot = _float3(o.rot[0], o.rot[1], o.rot[2]);
        d.vPos = _float3(o.pos[0], o.pos[1], o.pos[2]);
        m_pGameInstance->Add_GameObject_ToLayer(
            ENUM_CLASS(LEVEL::EDIT), L"Layer_MapObject",
            ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_GameObject_MapObject"), &d);
    }
}

CMainApp* CMainApp::Create()
{
    CMainApp* p = new CMainApp();
    if (FAILED(p->Initialize())) { MSG_BOX(TEXT("Failed to Created : CMainApp")); Safe_Release(p); }
    return p;
}

void CMainApp::Free()
{
    __super::Free();
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    Safe_Release(m_pDevice); Safe_Release(m_pContext);
    m_pGameInstance->Release_Engine(); Safe_Release(m_pGameInstance);
}
NS_END
