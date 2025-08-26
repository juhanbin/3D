#pragma once

#include "Edit_Defines.h"
#include "Base.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "BinType.h"  // BoneInfoBin 등 사용 시 유지

NS_BEGIN(Engine)
class CGameInstance;
NS_END

NS_BEGIN(Edit)

class CMainApp final : public CBase
{
private:
    CMainApp();
    virtual ~CMainApp() = default;

public:
    HRESULT Initialize();
    void    Update(_float fTimeDelta);
    HRESULT Render();

private:
    Engine::CGameInstance* m_pGameInstance = nullptr;
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;

private:
    HRESULT Ready_Prototype_ForStatic();
    HRESULT Start_Level(LEVEL eStartLevelID);

public: // (필요하면 유지)
    void GatherBones(const aiNode* node, int parentIdx);

public: // MapTool
    void Render_ImGuiPanel();
    void SaveScene(const char* filename);
    bool LoadScene(const char* filename);
    void PushUndo();
    void RefreshScene();

private: // Picking helpers (월드 변환/레이)
    DirectX::XMMATRIX MakeWorld(const MapObject& o) const;
    DirectX::XMMATRIX GetModelPreTransform(const MapObject& o) const; // 렌더와 동일 전처리
    bool RaycastGround(const DirectX::XMVECTOR& rayPosW,
        const DirectX::XMVECTOR& rayDirW,
        DirectX::XMFLOAT3& outHitW) const;

    // 화면 좌표 → 레이 생성
    static void MakeRayFromMouse(DirectX::XMVECTOR& ro, DirectX::XMVECTOR& rd,
        const _float4x4& V, const _float4x4& P);

    // ====== 메시(삼각형) 피킹 ======
    struct MeshCache {
        std::vector<DirectX::XMFLOAT3> vertices; // 로컬 좌표(PreTransform bake)
        std::vector<uint32_t>          indices;  // 3의 배수
    };
    struct ModelCache {
        bool loaded = false;
        std::vector<MeshCache> meshes;
        // OBB용 로컬 AABB
        DirectX::XMFLOAT3 localCenter{ 0,0,0 };
        DirectX::XMFLOAT3 localExtent{ 0,0,0 };
    };

    // FBX만 읽어서 캐시 (pre-transform 적용)
    bool LoadFbxForPicking_FBX(const char* fbx, const DirectX::XMMATRIX& P, ModelCache& out);
    const ModelCache* GetModelCacheFbx(const char* fbxPath, const DirectX::XMMATRIX& pre);

    // 레이-삼각형 교차 (M?ller?Trumbore)
    static bool RayTriangleMT(DirectX::FXMVECTOR ro, DirectX::FXMVECTOR rd,
        DirectX::FXMVECTOR v0, DirectX::FXMVECTOR v1, DirectX::FXMVECTOR v2,
        float& t, float& u, float& v);

    // 가장 가까운 “실제 표면(A)” 피킹
    bool PickSurface_Mesh(DirectX::XMFLOAT3& outHitPointW,
        DirectX::XMFLOAT3& outNormalW,
        int& outObjIdx); // 엔진 레이 사용 버전

    // 레이를 직접 넣는 버전(첫 클릭 안정화용)
    bool PickSurface_Mesh(const DirectX::XMVECTOR& ro, const DirectX::XMVECTOR& rd,
        DirectX::XMFLOAT3& outHitPointW,
        DirectX::XMFLOAT3& outNormalW,
        int& outObjIdx);

    // ====== OBB (브로드페이즈용, 선택) ======
    struct OBB {
        DirectX::XMFLOAT3 C;       // center (world)
        DirectX::XMFLOAT3 AxisX;   // world unit axis
        DirectX::XMFLOAT3 AxisY;
        DirectX::XMFLOAT3 AxisZ;
        DirectX::XMFLOAT3 Extent;  // half-size (scale 반영)
    };
    bool  BuildOBB(const MapObject& o, const ModelCache& mdl, OBB& out) const;
    static bool RayOBB_Face(const DirectX::XMVECTOR& ro, const DirectX::XMVECTOR& rd,
        const OBB& box, float& outT, int& outFace,
        DirectX::XMVECTOR& outP, DirectX::XMVECTOR& outN);

private:
    std::vector<BoneInfoBin> m_Bones;

    std::vector<MapObject>               m_Objects;
    std::vector<std::vector<MapObject>>  m_UndoStack;
    int   m_Selected = -1;
    float m_TempSize[3] = { 1,1,1 };
    float m_TempRot[3] = { 0,0,0 };
    float m_TempPos[3] = { 0,0,0 };

    ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
    ID3D11DepthStencilView* m_pDepthStencilView = nullptr;

    const aiScene* m_pAIScene = nullptr;
    Assimp::Importer m_Importer{};
    std::vector<ID3D11ShaderResourceView*> m_SRVs[AI_TEXTURE_TYPE_MAX];

    // Picking debug
    bool     m_PickDebugEnabled = true;
    bool     m_DrawRay = true;
    bool     m_LastPickValid = false;
    _float3  m_LastPickPos{ 0,0,0 };
    int      m_LastPickObj = -1;

    bool     m_DebugHasHit = false; // 메시/OBB를 맞췄는지
    _float3  m_DebugRayPos{ 0,0,0 };
    _float3  m_DebugRayDir{ 0,0,1 };
    _float3  m_DebugHitPoint{ 0,0,0 };
    _float3  m_DebugHitNormal{ 0,1,0 };

    // FBX 캐시(키: fbxPath + pre-transform)
    std::unordered_map<std::string, ModelCache> m_ModelCache;

public:
    static CMainApp* Create();
    virtual void Free() override;
};

NS_END
