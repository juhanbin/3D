// MainApp.cpp  (통째로 교체)

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
#include <cstdio>
#include <algorithm>
#include <cmath>            // roundf
#include <windows.h>        // OutputDebugStringA
#include <cstring>          // strncpy, memcpy, memset
#include <string>

using namespace DirectX;
using namespace Edit;

NS_BEGIN(Edit)

/* ------------------------ 엔진 수명주기 ------------------------ */

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

/* ----------------------------- 유틸 ----------------------------- */

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

// 렌더와 동일한 PreTransform (Loader와 1:1 매칭)
DirectX::XMMATRIX CMainApp::GetModelPreTransform(const MapObject&) const
{
    return XMMatrixScaling(0.01f, 0.01f, 0.01f) *
        XMMatrixRotationY(XMConvertToRadians(180.0f));
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

/* ------------------- FBX 로더(피킹 캐시) ------------------- */

bool CMainApp::LoadFbxForPicking_FBX(const char* fbx,
    const DirectX::XMMATRIX& P,
    ModelCache& out)
{
    Assimp::Importer imp;
    const aiScene* sc = imp.ReadFile(
        fbx,
        aiProcess_ConvertToLeftHanded |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices   // 노드 변환 버텍스 bake
    );
    if (!sc) {
        char dbg[512];
        std::snprintf(dbg, sizeof(dbg), "[FBX load fail] %s : %s\n", fbx, imp.GetErrorString());
        OutputDebugStringA(dbg);
        return false;
    }

    out.meshes.resize(sc->mNumMeshes);

    DirectX::XMFLOAT3 minV(FLT_MAX, FLT_MAX, FLT_MAX);
    DirectX::XMFLOAT3 maxV(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (uint32_t i = 0; i < sc->mNumMeshes; ++i) {
        aiMesh* m = sc->mMeshes[i];
        auto& mc = out.meshes[i];
        mc.vertices.resize(m->mNumVertices);

        for (uint32_t v = 0; v < m->mNumVertices; ++v) {
            DirectX::XMFLOAT3 p{ m->mVertices[v].x, m->mVertices[v].y, m->mVertices[v].z };

            XMVECTOR lp = XMLoadFloat3(&p);
            XMFLOAT3 pp;
            XMStoreFloat3(&pp, XMVector3TransformCoord(lp, P));
            mc.vertices[v] = pp;

            minV.x = std::min(minV.x, pp.x);  minV.y = std::min(minV.y, pp.y);  minV.z = std::min(minV.z, pp.z);
            maxV.x = max(maxV.x, pp.x);  maxV.y = max(maxV.y, pp.y);  maxV.z = max(maxV.z, pp.z);
        }

        mc.indices.resize(m->mNumFaces * 3);
        uint32_t w = 0;
        for (uint32_t f = 0; f < m->mNumFaces; ++f) {
            const aiFace& face = m->mFaces[f];
            mc.indices[w++] = face.mIndices[0];
            mc.indices[w++] = face.mIndices[1];
            mc.indices[w++] = face.mIndices[2];
        }
    }

    out.localCenter = DirectX::XMFLOAT3((minV.x + maxV.x) * 0.5f,
        (minV.y + maxV.y) * 0.5f,
        (minV.z + maxV.z) * 0.5f);
    out.localExtent = DirectX::XMFLOAT3((maxV.x - minV.x) * 0.5f,
        (maxV.y - minV.y) * 0.5f,
        (maxV.z - minV.z) * 0.5f);
    out.loaded = true;
    return true;
}

// 캐시 키: 경로 + PreTransform
static std::string MakeFbxCacheKey(const char* path, const XMMATRIX& pre)
{
    XMFLOAT4X4 M; XMStoreFloat4x4(&M, pre);
    char buf[512];
    std::snprintf(buf, sizeof(buf),
        "%s|%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
        path ? path : "",
        M._11, M._12, M._13, M._14,
        M._21, M._22, M._23, M._24,
        M._31, M._32, M._33, M._34,
        M._41, M._42, M._43, M._44);
    return std::string(buf);
}

const CMainApp::ModelCache* CMainApp::GetModelCacheFbx(const char* fbxPath, const XMMATRIX& pre)
{
    if (!fbxPath || !fbxPath[0]) return nullptr;

    const std::string key = MakeFbxCacheKey(fbxPath, pre);
    auto it = m_ModelCache.find(key);
    if (it != m_ModelCache.end()) return &it->second;

    ModelCache mc{};
    if (!LoadFbxForPicking_FBX(fbxPath, pre, mc)) {
        m_ModelCache.emplace(key, ModelCache{}); // 실패 캐시
        return nullptr;
    }
    m_ModelCache.emplace(key, std::move(mc));
    return &m_ModelCache.find(key)->second;
}


bool CMainApp::RayTriangleMT(FXMVECTOR ro, FXMVECTOR rd,
    FXMVECTOR v0, FXMVECTOR v1, FXMVECTOR v2,
    float& t, float& u, float& v)
{
    const XMVECTOR e1 = v1 - v0;
    const XMVECTOR e2 = v2 - v0;

    // 퇴화 삼각형 방지
    if (XMVectorGetX(XMVector3LengthSq(XMVector3Cross(e1, e2))) < 1e-12f) return false;

    const XMVECTOR p = XMVector3Cross(rd, e2);
    const float det = XMVectorGetX(XMVector3Dot(e1, p));
    if (fabsf(det) < 1e-7f) return false;
    const float invDet = 1.0f / det;

    const XMVECTOR s = ro - v0;
    u = XMVectorGetX(XMVector3Dot(s, p)) * invDet;
    if (u < 0.f || u > 1.f) return false;

    const XMVECTOR q = XMVector3Cross(s, e1);
    v = XMVectorGetX(XMVector3Dot(rd, q)) * invDet;
    if (v < 0.f || u + v > 1.f) return false;

    t = XMVectorGetX(XMVector3Dot(e2, q)) * invDet;
    return t > 0.f;
}

bool CMainApp::PickSurface_Mesh(XMFLOAT3& outHitPointW,
    XMFLOAT3& outNormalW,
    int& outObjIdx)
{
    const _float3& rp = m_pGameInstance->Get_RayPos();
    const _float3& rd = m_pGameInstance->Get_RayDir();
    XMVECTOR ro = XMLoadFloat3(&rp);
    XMVECTOR rdv = XMVector3Normalize(XMLoadFloat3(&rd));
    return PickSurface_Mesh(ro, rdv, outHitPointW, outNormalW, outObjIdx);
}

bool CMainApp::PickSurface_Mesh(const XMVECTOR& rayPosW, const XMVECTOR& rayDirW,
    XMFLOAT3& outHitPointW, XMFLOAT3& outNormalW, int& outObjIdx)
{
    float bestT = FLT_MAX;
    bool  found = false;
    int   bestIdx = -1;
    XMFLOAT3 bestP{}, bestN{};

    for (int i = 0; i < (int)m_Objects.size(); ++i)
    {
        const MapObject& o = m_Objects[i];

        XMMATRIX Pre = GetModelPreTransform(o);
        const ModelCache* model = GetModelCacheFbx(o.fbxPath, Pre);
        if (!model || !model->loaded) continue;

        XMMATRIX W = MakeWorld(o);
        XMMATRIX Nmat = XMMatrixInverse(nullptr, XMMatrixTranspose(W));

        for (const MeshCache& m : model->meshes)
        {
            if (m.indices.empty() || m.vertices.empty()) continue;

            for (size_t f = 0; f + 2 < m.indices.size(); f += 3)
            {
                uint32_t i0 = m.indices[f + 0];
                uint32_t i1 = m.indices[f + 1];
                uint32_t i2 = m.indices[f + 2];
                if (i0 >= m.vertices.size() || i1 >= m.vertices.size() || i2 >= m.vertices.size()) continue;

                // 로컬→월드
                XMVECTOR v0 = XMVector3TransformCoord(XMLoadFloat3(&m.vertices[i0]), W);
                XMVECTOR v1 = XMVector3TransformCoord(XMLoadFloat3(&m.vertices[i1]), W);
                XMVECTOR v2 = XMVector3TransformCoord(XMLoadFloat3(&m.vertices[i2]), W);

                float t, uu, vv;
                if (!RayTriangleMT(rayPosW, rayDirW, v0, v1, v2, t, uu, vv)) continue;
                if (t <= 1e-6f || t >= bestT) continue;

                // 교차점/노말 계산(월드)
                XMVECTOR P = rayPosW + rayDirW * t;
                XMVECTOR N = XMVector3Normalize(
                    XMVector3TransformNormal(XMVector3Cross(v1 - v0, v2 - v0), Nmat));

                bestT = t; bestIdx = i;
                XMStoreFloat3(&bestP, P);
                XMStoreFloat3(&bestN, N);
                found = true;
            }
        }
    }

    if (found) {
        outHitPointW = bestP; outNormalW = bestN; outObjIdx = bestIdx;
        return true;
    }

    XMFLOAT3 g{};
    if (RaycastGround(rayPosW, rayDirW, g)) {
        outHitPointW = g; outNormalW = XMFLOAT3(0, 1, 0); outObjIdx = -1;
        return true;
    }
    return false;
}

bool CMainApp::BuildOBB(const MapObject& o, const ModelCache& mdl, OBB& out) const
{
    if (!mdl.loaded) return false;

    XMMATRIX W = MakeWorld(o);

    XMFLOAT4X4 M; XMStoreFloat4x4(&M, W);
    XMVECTOR Ax = XMVectorSet(M._11, M._12, M._13, 0);
    XMVECTOR Ay = XMVectorSet(M._21, M._22, M._23, 0);
    XMVECTOR Az = XMVectorSet(M._31, M._32, M._33, 0);

    float sx = XMVectorGetX(XMVector3Length(Ax));
    float sy = XMVectorGetX(XMVector3Length(Ay));
    float sz = XMVectorGetX(XMVector3Length(Az));

    Ax = XMVector3Normalize(Ax);
    Ay = XMVector3Normalize(Ay);
    Az = XMVector3Normalize(Az);

    XMVECTOR Cw = XMVector3TransformCoord(XMLoadFloat3(&mdl.localCenter), W);

    XMFLOAT3 e = mdl.localExtent;
    XMFLOAT3 Ew(e.x * sx, e.y * sy, e.z * sz);

    XMStoreFloat3(&out.C, Cw);
    XMStoreFloat3(&out.AxisX, Ax);
    XMStoreFloat3(&out.AxisY, Ay);
    XMStoreFloat3(&out.AxisZ, Az);
    out.Extent = Ew;
    return true;
}

bool CMainApp::RayOBB_Face(const XMVECTOR& ro, const XMVECTOR& rd,
    const OBB& b, float& outT, int& outFace,
    XMVECTOR& outP, XMVECTOR& outN)
{
    XMVECTOR C = XMLoadFloat3(&b.C);
    XMVECTOR U = XMVector3Normalize(XMLoadFloat3(&b.AxisX));
    XMVECTOR V = XMVector3Normalize(XMLoadFloat3(&b.AxisY));
    XMVECTOR W = XMVector3Normalize(XMLoadFloat3(&b.AxisZ));
    const float ex = b.Extent.x, ey = b.Extent.y, ez = b.Extent.z;

    XMVECTOR d = ro - C;
    float rox = XMVectorGetX(XMVector3Dot(d, U));
    float roy = XMVectorGetX(XMVector3Dot(d, V));
    float roz = XMVectorGetX(XMVector3Dot(d, W));
    float rdx = XMVectorGetX(XMVector3Dot(rd, U));
    float rdy = XMVectorGetX(XMVector3Dot(rd, V));
    float rdz = XMVectorGetX(XMVector3Dot(rd, W));

    auto test = [&](float roS, float rdS, float eS, int face,
        float& bestT, int& bestFace) {
            if (fabsf(rdS) < 1e-7f) return;
            float plane = (face & 1) ? +eS : -eS;
            float t = (plane - roS) / rdS;
            if (t <= 0.f || t >= bestT) return;

            float x = rox + rdx * t;
            float y = roy + rdy * t;
            float z = roz + rdz * t;

            bool inside = false;
            switch (face >> 1) {
            case 0: inside = (fabsf(y) <= ey + 1e-6f) && (fabsf(z) <= ez + 1e-6f); break; // X면
            case 1: inside = (fabsf(x) <= ex + 1e-6f) && (fabsf(z) <= ez + 1e-6f); break; // Y면
            case 2: inside = (fabsf(x) <= ex + 1e-6f) && (fabsf(y) <= ey + 1e-6f); break; // Z면
            }
            if (inside) { bestT = t; bestFace = face; }
        };

    float bestT = FLT_MAX; int bestFace = -1;
    test(rox, rdx, ex, 0, bestT, bestFace);
    test(rox, rdx, ex, 1, bestT, bestFace);
    test(roy, rdy, ey, 2, bestT, bestFace);
    test(roy, rdy, ey, 3, bestT, bestFace);
    test(roz, rdz, ez, 4, bestT, bestFace);
    test(roz, rdz, ez, 5, bestT, bestFace);

    if (bestFace < 0) return false;

    outT = bestT; outFace = bestFace;
    outP = ro + rd * bestT;
    switch (bestFace) {
    case 0: outN = -U; break; case 1: outN = U; break;
    case 2: outN = -V; break; case 3: outN = V; break;
    case 4: outN = -W; break; case 5: outN = W; break;
    }
    return true;
}

/* --------------------------- 레이 생성 --------------------------- */

void CMainApp::MakeRayFromMouse(XMVECTOR& ro, XMVECTOR& rd,
    const _float4x4& V, const _float4x4& P)
{
    const ImVec2 mp = ImGui::GetIO().MousePos;
    float nx = (mp.x / (float)g_iWinSizeX) * 2.f - 1.f;
    float ny = -(mp.y / (float)g_iWinSizeY) * 2.f + 1.f;

    XMMATRIX VP = XMLoadFloat4x4(&V) * XMLoadFloat4x4(&P);
    XMMATRIX invVP = XMMatrixInverse(nullptr, VP);

    XMVECTOR pNear = XMVector3TransformCoord(XMVectorSet(nx, ny, 0.f, 1.f), invVP);
    XMVECTOR pFar = XMVector3TransformCoord(XMVectorSet(nx, ny, 1.f, 1.f), invVP);

    ro = pNear;
    rd = XMVector3Normalize(pFar - pNear);
}

/* ------------------- BIN Export (Anim / Non-Anim) ------------------- */

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
    m_Bones.clear();
    GatherBones(scene->mRootNode, -1);

    uint32_t boneCount = static_cast<uint32_t>(m_Bones.size());
    fwrite(&boneCount, sizeof(uint32_t), 1, fp);
    if (boneCount) fwrite(m_Bones.data(), sizeof(BoneInfoBin), boneCount, fp);

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
        std::memset(info.Name, 0, sizeof(MeshInfoBin));
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

    std::vector<MaterialInfoBin2> matInfos(numMaterials);
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
    if (numMaterials) fwrite(matInfos.data(), sizeof(MaterialInfoBin2), numMaterials, fp);

    // 4) 애니메이션
    uint32_t animCount = scene->mNumAnimations;
    fwrite(&animCount, sizeof(animCount), 1, fp);

    for (uint32_t a = 0; a < animCount; ++a) {
        aiAnimation* anim = scene->mAnimations[a];

        AnimInfoBin ainfo{};
        std::strncpy(ainfo.name, anim->mName.C_Str(), 63);
        ainfo.name[63] = 0;
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

            // 최대 키 수를 채널의 keyframeCount로 기록
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
    printf("BIN(애니) 저장 완료: %s\n", binPath);
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

    // 1) 메시 개수
    fwrite(&numMeshes, sizeof(uint32_t), 1, fp);

    std::vector<MeshInfoBin>          meshInfos(numMeshes);
    std::vector<std::vector<VTXMESH>> allVertices(numMeshes);
    std::vector<std::vector<uint32_t>> allIndices(numMeshes);

    for (uint32_t i = 0; i < numMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        MeshInfoBin& info = meshInfos[i];
        std::memset(&info, 0, sizeof(MeshInfoBin));
        std::strncpy(info.Name, mesh->mName.C_Str(), 63);
        info.Name[63] = 0;
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
        for (uint32_t f = 0; f < info.NumFaces; ++f) {
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
        if (meshInfos[i].NumVertices)
            fwrite(allVertices[i].data(), sizeof(VTXMESH), meshInfos[i].NumVertices, fp);

    // 4) 각 메시의 인덱스
    for (uint32_t i = 0; i < numMeshes; ++i)
        if (meshInfos[i].NumIndices)
            fwrite(allIndices[i].data(), sizeof(uint32_t), meshInfos[i].NumIndices, fp);

    // 5) 머티리얼
    uint32_t numMaterials = scene->mNumMaterials;
    fwrite(&numMaterials, sizeof(uint32_t), 1, fp);

    std::vector<MaterialInfoBin2> matInfos(numMaterials);
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
    if (numMaterials) fwrite(matInfos.data(), sizeof(MaterialInfoBin2), numMaterials, fp);

    fclose(fp);
    printf("BIN(논애니) 저장 완료: %s\n", binPath);
}


/* ------------------------------ IMGUI ------------------------------ */

void CMainApp::Render_ImGuiPanel()
{
    // Nav 편집 모드 토글(로컬 static: 프레임 간 유지)
    static bool sNavEnabled = true;

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
                case EObjectType::MONSTER:       strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster/Monster.fbx");           break;
                case EObjectType::ROCK_AA:       strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Rock/Rock_AA.fbx");              break;
                case EObjectType::HERO:          strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Hero/Hero.fbx");                 break;
                case EObjectType::SPEAR:         strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Spear/Spear.fbx");               break;
                case EObjectType::SPEAR_STATIC:         strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Spear_Static/Spear_Static.fbx");               break;
                case EObjectType::MONSTER_SPEAR: strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster_Spear/Monster_Spear.fbx"); break;
                case EObjectType::MONSTER_BOW:   strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster_Bow/Monster_Bow.fbx");   break;
                case EObjectType::BRIDGE:        strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Bridge/Bridge.fbx");             break;
                case EObjectType::CAVE:          strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Cave/Cave.fbx");                 break;
                case EObjectType::SKELETON_SPEAR: strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster/Monster.fbx");          break;
                case EObjectType::SKELETON_BOW:   strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster/Monster.fbx");          break;
                case EObjectType::BOSS_EYE_MID:       strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Boss_Eye_Mid/Boss_Eye_Mid.fbx");           break;
                case EObjectType::BOSS_EYE_TOP:       strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Boss_Eye_Top/Boss_Eye_Top.fbx");              break;
                case EObjectType::BOSS_FIRE:          strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Boss_Fire/Boss_Fire.fbx");                 break;
                case EObjectType::BOSS_HAND_L:         strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Boss_Hand_L/Boss_Hand_L.fbx");               break;
                case EObjectType::BOSS_HAND_R: strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Boss_Hand_R/Boss_Hand_R.fbx"); break;
                case EObjectType::BOSS_MASK:   strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Boss_Mask/Boss_Mask.fbx");   break;
                case EObjectType::EYESPAWNER:        strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/EyeSpawner/EyeSpawner.fbx");             break;
                case EObjectType::MONSTER_EYE:        strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Eye/Eye.fbx"); break;
                case EObjectType::MUSHROOM:          strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Mushroom/Mushroom.fbx");                 break;
                case EObjectType::SMALLMUSHROOM: strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Small_Mushroom/Small_Mushroom.fbx");          break;
                case EObjectType::PARASIT_EYE:   strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Eye/Eye.fbx");          break;
                case EObjectType::MONSTER_ARROW:   strcpy(o.fbxPath, "../Bin/Resources/Blood_Spear/Model/Monster_Arrow/Monster_Arrow.fbx");          break;
                default: o.fbxPath[0] = 0; break;
                }

                m_Objects.push_back(o); RefreshScene(); ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();
    ImGui::Checkbox("Enable Pick Debug", &m_PickDebugEnabled);
    ImGui::SameLine();
    ImGui::Checkbox("Draw Ray", &m_DrawRay);

    if (m_LastPickValid) {
        ImGui::Text("Hit Pos: (%.3f, %.3f, %.3f)%s",
            m_LastPickPos.x, m_LastPickPos.y, m_LastPickPos.z,
            m_LastPickObj >= 0 ? "" : "  (ground)");
        if (ImGui::Button("Copy XYZ")) {
            char buf[128]; std::snprintf(buf, sizeof(buf), "%.6f %.6f %.6f",
                m_LastPickPos.x, m_LastPickPos.y, m_LastPickPos.z);
            ImGui::SetClipboardText(buf);
        }
    }
    else {
        ImGui::TextUnformatted("No pick yet.");
    }

    // 클릭 → 메시표면(A) 정확 피킹
    // Nav 편집 모드일 때는 디버그 피킹 비활성화(!sNavEnabled 가드)
    if (m_PickDebugEnabled &&
        !sNavEnabled &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::GetIO().WantCaptureMouse)
    {
        _float4x4 V = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW);
        _float4x4 P = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ);
        XMVECTOR ro, rd;
        MakeRayFromMouse(ro, rd, V, P);

        XMFLOAT3 hit, nrm; int idx;
        if (PickSurface_Mesh(ro, rd, hit, nrm, idx)) {
            m_LastPickPos = hit; m_LastPickValid = true; m_LastPickObj = idx;

            // 디버그용 저장
            m_DebugRayPos = { XMVectorGetX(ro), XMVectorGetY(ro), XMVectorGetZ(ro) };
            m_DebugRayDir = { XMVectorGetX(rd), XMVectorGetY(rd), XMVectorGetZ(rd) };
            m_DebugHitPoint = hit;
            m_DebugHitNormal = nrm;
            m_DebugHasHit = (idx >= 0);

            char dbg[256];
            if (idx >= 0)
                std::snprintf(dbg, sizeof(dbg),
                    "[PickSurface_Mesh] obj=%d  hit=(%.3f, %.3f, %.3f)  n=(%.3f, %.3f, %.3f)\n",
                    idx, hit.x, hit.y, hit.z, nrm.x, nrm.y, nrm.z);
            else
            {
                std::snprintf(dbg, sizeof(dbg),
                    "[PickSurface_Mesh] ground hit=(%.3f, %.3f, %.3f)\n",
                    hit.x, hit.y, hit.z);
                OutputDebugStringA(dbg);
            }
                
            wchar_t wbuf[128];
            swprintf(wbuf, 128, L"[PickClick] (%.3f, %.3f, %.3f)\n", hit.x, hit.y, hit.z);
            OutputDebugStringW(wbuf);
        }
        else {
            m_LastPickValid = false; m_LastPickObj = -1; m_DebugHasHit = false;
        }
    }

    ImGui::Separator();
    for (int i = 0; i < (int)m_Objects.size(); ++i) {
        char label[64]; std::snprintf(label, sizeof(label), "[%s] Object %d",
            ToObjectTypeString(m_Objects[i].type), m_Objects[i].id);
        if (ImGui::Selectable(label, m_Selected == i)) {
            m_Selected = i;
            std::memcpy(m_TempSize, m_Objects[i].size, sizeof(float) * 3);
            std::memcpy(m_TempRot, m_Objects[i].rot, sizeof(float) * 3);
            std::memcpy(m_TempPos, m_Objects[i].pos, sizeof(float) * 3);
        }
    }
    ImGui::End();

    /* -------- Object Properties + BIN Export 버튼 -------- */

    ImGui::Begin("Object Properties");
    static int  s_PrevSelected = -1;
    static char s_BinPath[260] = "../../Mapdata/Model.bin";
    auto deriveBinFromFbx = [](const char* fbx, char* out, size_t outsz) {
        if (!fbx || !fbx[0]) { std::snprintf(out, outsz, "../../Mapdata/Model.bin"); return; }
        // 파일명 추출
        const char* p = fbx;
        const char* slash1 = strrchr(fbx, '/');
        const char* slash2 = strrchr(fbx, '\\');
        if (slash1) p = slash1 + 1;
        if (slash2 && slash2 > p) p = slash2 + 1;
        std::string stem = p ? p : fbx;
        // 확장자 제거
        size_t dot = stem.find_last_of('.');
        if (dot != std::string::npos) stem = stem.substr(0, dot);
        std::snprintf(out, outsz, "../../Mapdata/%s.bin", (int)stem.size() ? stem.c_str() : "Model");
        };

    if (m_Selected != -1) {
        if (s_PrevSelected != m_Selected) {
            deriveBinFromFbx(m_Objects[m_Selected].fbxPath, s_BinPath, sizeof(s_BinPath));
            s_PrevSelected = m_Selected;
        }

        ImGui::Text("Size");     ImGui::DragFloat3("x/y/z##size", m_TempSize, 0.1f);
        ImGui::Text("Rotation"); ImGui::DragFloat3("x/y/z##rot", m_TempRot, 0.1f);
        ImGui::Text("Position"); ImGui::DragFloat3("x/y/z##pos", m_TempPos, 0.1f);
        ImGui::InputText("FBX Path", m_Objects[m_Selected].fbxPath, 260);
        ImGui::InputText("BIN Path", s_BinPath, 260);

        if (ImGui::Button("Delete")) { PushUndo(); m_Objects.erase(m_Objects.begin() + m_Selected); m_Selected = -1; RefreshScene(); }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            PushUndo();
            std::memcpy(m_Objects[m_Selected].size, m_TempSize, sizeof(float) * 3);
            std::memcpy(m_Objects[m_Selected].rot, m_TempRot, sizeof(float) * 3);
            std::memcpy(m_Objects[m_Selected].pos, m_TempPos, sizeof(float) * 3);
            RefreshScene();
        }

        // --- BIN Export 버튼
        if (ImGui::Button("Export (Non-Anim)")) { ExportModelToBin_NonAnim(m_Objects[m_Selected], s_BinPath); }
        ImGui::SameLine();
        if (ImGui::Button("Export (Anim)")) { ExportModelToBin_Anim(m_Objects[m_Selected], s_BinPath); }
    }
    ImGui::End();

    /*  NavMesh Tool 패널 */

    ImGui::Begin("NavMesh Tool");

    ImGui::Checkbox("Enable Nav Edit (LMB to add)", &sNavEnabled);
    ImGui::SameLine();
    ImGui::Text("Cells: %d  Work: %d", (int)m_NavCells.size(), (int)m_NavWorking.size());

    static bool sAutoCommit = true;
    ImGui::Checkbox("Auto Commit Triangle", &sAutoCommit);
    ImGui::Checkbox("Snap to Grid", &m_NavSnapToGrid);
    ImGui::SameLine(); ImGui::DragFloat("Grid", &m_NavGridSize, 0.05f, 0.05f, 10.f, "%.2f");
    ImGui::DragFloat("Join Radius", &m_NavJoinRadius, 0.01f, 0.0f, 2.0f, "%.3f");
    ImGui::DragFloat("Snap Epsilon", &m_NavSnapEps, 1e-4f, 0.0f, 0.1f, "%.5f");
    ImGui::Checkbox("Force CW on XZ", &m_NavForceCW_XZ);

    if (sNavEnabled &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::GetIO().WantCaptureMouse)
    {
        _float4x4 V = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW);
        _float4x4 P = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ);
        XMVECTOR ro, rd;
        MakeRayFromMouse(ro, rd, V, P);

        _float3 hit{};
        if (Nav_TryPickPoint(ro, rd, hit)) {
            m_NavWorking.push_back(hit);
            if (sAutoCommit && m_NavWorking.size() >= 3)
                Nav_CommitIfTri();
        }
    }

    if (ImGui::Button("Commit Triangle")) { Nav_CommitIfTri(); }
    ImGui::SameLine();
    if (ImGui::Button("Undo Working Point")) { if (!m_NavWorking.empty()) m_NavWorking.pop_back(); }
    ImGui::SameLine();
    if (ImGui::Button("Undo Cell")) { Nav_UndoCell(); }
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) { Nav_ClearAll(); }

    static char sNavPath[260] = "../../Mapdata/navmesh.bin";
    ImGui::InputText("Nav BIN Path", sNavPath, 260);
    if (ImGui::Button("Save Nav")) {
        if (Nav_Save(sNavPath)) OutputDebugStringA("[NavMesh] save OK\n");
        else                    OutputDebugStringA("[NavMesh] save FAILED\n");
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Nav")) {
        if (Nav_Load(sNavPath)) { OutputDebugStringA("[NavMesh] load OK\n"); RefreshScene(); }
        else                    OutputDebugStringA("[NavMesh] load FAILED\n");
    }

    ImGui::End();

    // ===== 2D 디버그 오버레이 (셀/작업점 표시) =====
    auto WorldToScreen = [this](const DirectX::XMFLOAT3& wpos, ImVec2& out)->bool {
        using namespace DirectX;
        _float4x4 V = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW);
        _float4x4 P = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ);
        XMMATRIX VP = XMLoadFloat4x4(&V) * XMLoadFloat4x4(&P);

        XMVECTOR p = XMVector3TransformCoord(XMLoadFloat3(&wpos), VP); // NDC
        float x = XMVectorGetX(p), y = XMVectorGetY(p), z = XMVectorGetZ(p);
        if (z < 0.f || z > 1.f) return false;
        float sx = (x * 0.5f + 0.5f) * (float)g_iWinSizeX;
        float sy = (-y * 0.5f + 0.5f) * (float)g_iWinSizeY;
        out = ImVec2(sx, sy);
        return true;
        };

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // 레이
    if (m_DrawRay) {
        XMFLOAT3 p0 = m_DebugRayPos;
        XMFLOAT3 dir = m_DebugRayDir;
        XMFLOAT3 p1;
        if (m_LastPickValid) p1 = m_DebugHitPoint;
        else {
            p1 = XMFLOAT3(p0.x + dir.x * 1000.f,
                p0.y + dir.y * 1000.f,
                p0.z + dir.z * 1000.f);
        }
        ImVec2 a, b;
        if (WorldToScreen(p0, a) && WorldToScreen(p1, b))
            dl->AddLine(a, b, IM_COL32(255, 215, 0, 255), 2.0f);
    }

    if (m_LastPickValid) {
        ImVec2 sh;
        if (WorldToScreen(m_DebugHitPoint, sh)) {
            ImU32 col = m_DebugHasHit ? IM_COL32(64, 255, 64, 255) : IM_COL32(64, 255, 255, 255);
            dl->AddCircleFilled(sh, 5.f, col, 16);
        }

        XMFLOAT3 tip = {
            m_DebugHitPoint.x + m_DebugHitNormal.x * 0.35f,
            m_DebugHitPoint.y + m_DebugHitNormal.y * 0.35f,
            m_DebugHitPoint.z + m_DebugHitNormal.z * 0.35f
        };
        ImVec2 a, b;
        if (WorldToScreen(m_DebugHitPoint, a) && WorldToScreen(tip, b)) {
            dl->AddLine(a, b, IM_COL32(200, 200, 255, 255), 2.0f);
        }
    }

    Nav_RenderOverlay();
}

/* ---------------- Scene Save / Load & Helpers ---------------- */

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

/* ------------------------ Nav 편집 보조 유틸 ------------------------ */

// 보조 유틸(그리드 양자화)
static inline _float3 Quantize(const _float3& p, float grid) {
    _float3 q;
    q.x = std::roundf(p.x / grid) * grid;
    q.y = std::roundf(p.y / grid) * grid;
    q.z = std::roundf(p.z / grid) * grid;
    return q;
}

// 반경 내 가장 가까운 기존 정점 찾기
static inline const _float3* FindNearestNavVertex(const std::vector<_float3>& verts,
    const _float3& p, float radius)
{
    const _float3* nearest = nullptr;
    float best2 = radius * radius;
    for (const auto& v : verts) {
        float dx = v.x - p.x, dy = v.y - p.y, dz = v.z - p.z;
        float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 <= best2) { best2 = d2; nearest = &v; }
    }
    return nearest;
}

static inline float SignedAreaXZ(const _float3& A, const _float3& B, const _float3& C)
{
    float x1 = B.x - A.x, z1 = B.z - A.z;
    float x2 = C.x - A.x, z2 = C.z - A.z;
    return x1 * z2 - z1 * x2; // +: CCW, -: CW
}

_bool CMainApp::Nav_TryPickPoint(const XMVECTOR& ro, const XMVECTOR& rd, _float3& out)
{
    XMFLOAT3 hit, nrm; int idx;
    if (PickSurface_Mesh(ro, rd, hit, nrm, idx)) { out = hit; return true; }
    return false;
}

_float3 CMainApp::Nav_SnapAndRegister(const _float3& pIn)
{
    _float3 p = pIn;

    if (const _float3* nv = FindNearestNavVertex(m_NavVerts, p, m_NavJoinRadius)) {
        return *nv;
    }
    _float3 key = m_NavSnapToGrid ? Quantize(p, m_NavGridSize) : p;

    for (const auto& v : m_NavVerts) {
        if (fabsf(v.x - key.x) <= m_NavSnapEps &&
            fabsf(v.y - key.y) <= m_NavSnapEps &&
            fabsf(v.z - key.z) <= m_NavSnapEps)
        {
            return v;
        }
    }

    m_NavVerts.push_back(key);
    return key;
}

void CMainApp::Nav_EnsureCCW_Up(_float3& A, _float3& B, _float3& C)
{
    XMVECTOR a = XMLoadFloat3(&A), b = XMLoadFloat3(&B), c = XMLoadFloat3(&C);
    float ny = XMVectorGetY(XMVector3Cross(b - a, c - a));
    if (ny < 0.f) std::swap(B, C);

    if (m_NavForceCW_XZ) {
        float area = SignedAreaXZ(A, B, C); // +: CCW, -: CW
        if (area > 0.f) std::swap(B, C);  // CCW면 뒤집어서 CW로
    }
}

void CMainApp::Nav_CommitIfTri()
{
    if (m_NavWorking.size() < 3) return;

    _float3 A = Nav_SnapAndRegister(m_NavWorking[0]);
    _float3 B = Nav_SnapAndRegister(m_NavWorking[1]);
    _float3 C = Nav_SnapAndRegister(m_NavWorking[2]);

    // 퇴화 방지
    float area2 = XMVectorGetX(XMVector3LengthSq(
        XMVector3Cross(XMLoadFloat3(&B) - XMLoadFloat3(&A),
            XMLoadFloat3(&C) - XMLoadFloat3(&A))));
    if (area2 < 1e-10f) {
        OutputDebugStringA("[NavMesh] Degenerate triangle ignored.\n");
        m_NavWorking.clear();
        return;
    }

    Nav_EnsureCCW_Up(A, B, C);
    m_NavCells.push_back({ A,B,C });
    m_NavWorking.clear();
    OutputDebugStringA("[NavMesh] Cell committed.\n");
}

bool CMainApp::Nav_Save(const char* path)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;
    for (const auto& c : m_NavCells) {
        ofs.write(reinterpret_cast<const char*>(&c.A), sizeof(_float3));
        ofs.write(reinterpret_cast<const char*>(&c.B), sizeof(_float3));
        ofs.write(reinterpret_cast<const char*>(&c.C), sizeof(_float3));
    }
    return true;
}

bool CMainApp::Nav_Load(const char* path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;

    m_NavCells.clear();
    m_NavVerts.clear();
    m_NavWorking.clear();

    while (true) {
        NavCell c{};
        ifs.read(reinterpret_cast<char*>(&c.A), sizeof(_float3)); if (!ifs) break;
        ifs.read(reinterpret_cast<char*>(&c.B), sizeof(_float3)); if (!ifs) break;
        ifs.read(reinterpret_cast<char*>(&c.C), sizeof(_float3)); if (!ifs) break;

        c.A = Nav_SnapAndRegister(c.A);
        c.B = Nav_SnapAndRegister(c.B);
        c.C = Nav_SnapAndRegister(c.C);
        Nav_EnsureCCW_Up(c.A, c.B, c.C);
        m_NavCells.push_back(c);
    }
    return true;
}

void CMainApp::Nav_ClearAll()
{
    m_NavCells.clear();
    m_NavVerts.clear();
    m_NavWorking.clear();
}

void CMainApp::Nav_UndoCell()
{
    if (!m_NavCells.empty()) m_NavCells.pop_back();
}

void CMainApp::Nav_RenderOverlay()
{
    auto WorldToScreen = [this](const DirectX::XMFLOAT3& wpos, ImVec2& out)->bool {
        using namespace DirectX;
        _float4x4 V = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW);
        _float4x4 P = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ);
        XMMATRIX VP = XMLoadFloat4x4(&V) * XMLoadFloat4x4(&P);

        XMVECTOR p = XMVector3TransformCoord(XMLoadFloat3(&wpos), VP); // NDC
        float x = XMVectorGetX(p), y = XMVectorGetY(p), z = XMVectorGetZ(p);
        if (z < 0.f || z > 1.f) return false;
        float sx = (x * 0.5f + 0.5f) * (float)g_iWinSizeX;
        float sy = (-y * 0.5f + 0.5f) * (float)g_iWinSizeY;
        out = ImVec2(sx, sy);
        return true;
        };

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // 완성된 셀
    for (const auto& c : m_NavCells) {
        ImVec2 a, b, d;
        if (WorldToScreen(c.A, a) && WorldToScreen(c.B, b) && WorldToScreen(c.C, d)) {
            dl->AddTriangle(a, b, d, IM_COL32(0, 255, 0, 200), 2.0f);
            dl->AddTriangleFilled(a, b, d, IM_COL32(0, 255, 0, 40));
        }
    }

    // 작업 점
    for (const auto& p : m_NavWorking) {
        ImVec2 s; if (WorldToScreen(p, s))
            dl->AddCircleFilled(s, 5.f, IM_COL32(255, 128, 0, 255), 16);
    }
}

NS_END
