#pragma once

#include "Edit_Defines.h"
#include "Base.h"
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>
#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "BinType.h"

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

public: // bones (원본 유지)
    void GatherBones(const aiNode* node, int parentIdx);

public: // MapTool
    void Render_ImGuiPanel();
    void SaveScene(const char* filename);
    bool LoadScene(const char* filename);
    void ExportModelToBin_Anim(const MapObject& obj, const char* binPath);
    void ExportModelToBin_NonAnim(const MapObject& obj, const char* binPath);
    void PushUndo();
    void RefreshScene();

private: // Picking helpers (에디터용 임시 오브젝트 OBB)
    DirectX::XMMATRIX MakeWorld(const MapObject& o) const;

    bool RaycastObject_AABB(const DirectX::XMVECTOR& rayPosW,
        const DirectX::XMVECTOR& rayDirW,
        const MapObject& o,
        float& outWorldDist,
        DirectX::XMFLOAT3& outHitW) const;

    bool RaycastObject_OBB(DirectX::FXMVECTOR rayPosW, DirectX::FXMVECTOR rayDirW,
        const MapObject& o, float& outWorldDist, DirectX::XMFLOAT3& outHitW) const;

    bool RaycastGround(const DirectX::XMVECTOR& rayPosW,
        const DirectX::XMVECTOR& rayDirW,
        DirectX::XMFLOAT3& outHitW) const;

    bool PickPoint_OBB(DirectX::XMFLOAT3& outHitW, int& outObjIdx);

private:
    std::vector<MapObject>               m_Objects;
    std::vector<std::vector<MapObject>>  m_UndoStack;
    int   m_Selected = -1;
    float m_TempSize[3] = { 1,1,1 };
    float m_TempRot[3] = { 0,0,0 };
    float m_TempPos[3] = { 0,0,0 };

    // bones
    std::vector<BoneInfoBin> m_Bones;

    vector<_int>       m_BoneIndices;
    vector<_float4x4>  m_OffsetMatrices;
    ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
    ID3D11DepthStencilView* m_pDepthStencilView = nullptr;

    const aiScene* m_pAIScene = nullptr;
    Assimp::Importer        m_Importer{};
    std::vector<ID3D11ShaderResourceView*> m_SRVs[AI_TEXTURE_TYPE_MAX];

    // Picking debug
    bool     m_PickDebugEnabled = true;
    bool     m_LastPickValid = false;
    _float3  m_LastPickPos{ 0,0,0 };
    int      m_LastPickObj = -1;

public:
    static CMainApp* Create();
    virtual void Free() override;
};

NS_END
