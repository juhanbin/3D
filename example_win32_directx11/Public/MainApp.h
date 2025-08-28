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
#include "BinType.h"  // BoneInfoBin, MeshInfoBin, VTX(MESH/ANIMMESH) 등

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

    // 렌더와 동일한 전처리(로더와 1:1)
    DirectX::XMMATRIX GetModelPreTransform(const MapObject& o) const;

    bool RaycastGround(const DirectX::XMVECTOR& rayPosW,
        const DirectX::XMVECTOR& rayDirW,
        DirectX::XMFLOAT3& outHitW) const;

    // 화면 좌표 -> 레이 생성
    static void MakeRayFromMouse(DirectX::XMVECTOR& ro, DirectX::XMVECTOR& rd,
        const _float4x4& V, const _float4x4& P);

    // 메시(삼각형) 피킹 
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

    static bool RayTriangleMT(DirectX::FXMVECTOR ro, DirectX::FXMVECTOR rd,
        DirectX::FXMVECTOR v0, DirectX::FXMVECTOR v1, DirectX::FXMVECTOR v2,
        float& t, float& u, float& v);

    bool PickSurface_Mesh(DirectX::XMFLOAT3& outHitPointW,
        DirectX::XMFLOAT3& outNormalW,
        int& outObjIdx); 

    bool PickSurface_Mesh(const DirectX::XMVECTOR& ro, const DirectX::XMVECTOR& rd,
        DirectX::XMFLOAT3& outHitPointW,
        DirectX::XMFLOAT3& outNormalW,
        int& outObjIdx);

    struct OBB {
        DirectX::XMFLOAT3 C;      
        DirectX::XMFLOAT3 AxisX;  
        DirectX::XMFLOAT3 AxisY;
        DirectX::XMFLOAT3 AxisZ;
        DirectX::XMFLOAT3 Extent; 
    };
    bool  BuildOBB(const MapObject& o, const ModelCache& mdl, OBB& out) const;
    static bool RayOBB_Face(const DirectX::XMVECTOR& ro, const DirectX::XMVECTOR& rd,
        const OBB& box, float& outT, int& outFace,
        DirectX::XMVECTOR& outP, DirectX::XMVECTOR& outN);

    // ====== BIN Export ======
public:
    void ExportModelToBin_NonAnim(const MapObject& obj, const char* binPath);
    void ExportModelToBin_Anim(const MapObject& obj, const char* binPath);

private:
    // UI에서 쓸 기본 출력 경로 버퍼
    char m_ExportBinPath[260];

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

    std::unordered_map<std::string, ModelCache> m_ModelCache;

    // ======== Navigation Editing ========
public:
    struct NavCell { _float3 A, B, C; };

private:
    bool  m_NavEditMode = false;    
    bool  m_NavSnapToGrid = true;   
    float m_NavGridSize = 0.01f;    
    float m_NavSnapEps = 0.002f;    
    bool  m_NavForceCW_XZ = true;   
    float m_NavJoinRadius = 0.5f;   

    std::vector<_float3> m_NavVerts;          
    std::vector<NavCell> m_NavCells;          
    std::vector<_float3> m_NavWorking;        

    bool    Nav_TryPickPoint(const DirectX::XMVECTOR& ro, const DirectX::XMVECTOR& rd, _float3& out);
    _float3 Nav_SnapAndRegister(const _float3& p);
    void    Nav_EnsureCCW_Up(_float3& A, _float3& B, _float3& C);
    void    Nav_CommitIfTri();
    bool    Nav_Save(const char* path);
    bool    Nav_Load(const char* path);
    void    Nav_ClearAll();
    void    Nav_UndoCell();
    void    Nav_RenderOverlay();

public:
    static CMainApp* Create();
    virtual void Free() override;
};

NS_END
