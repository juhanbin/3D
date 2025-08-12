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

struct Ray {
    DirectX::XMFLOAT3 origin;
    DirectX::XMFLOAT3 dir;
};

NS_BEGIN(Edit)

class CMainApp final : public CBase
{
private:
    CMainApp();
    virtual ~CMainApp() = default;

public:
    HRESULT Initialize();
    void Update(_float fTimeDelta);
    HRESULT Render();

private:
    Engine::CGameInstance* m_pGameInstance = nullptr;
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;

private:
    HRESULT Ready_Prototype_ForStatic();
    HRESULT Start_Level(LEVEL eStartLevelID);

    //bone
public:
    void GatherBones(const aiNode* node, int parentIdx);

public:
    // MapTool ÇÔ¼ö
    void Render_ImGuiPanel();
    void SaveScene(const char* filename);
    bool LoadScene(const char* filename);
    void ExportModelToBin_Anim(const MapObject& obj, const char* binPath);
    void ExportModelToBin_NonAnim(const MapObject& obj, const char* binPath);
    void PushUndo();
    void RefreshScene();

    Ray CreatePickingRay(int mx, int my, int w, int h, const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj);
    bool RayIntersectsAABB(const Ray& ray, const DirectX::BoundingBox& box, float* outDist = nullptr);

private:
    std::vector<MapObject> m_Objects;
    std::vector<std::vector<MapObject>> m_UndoStack;
    int m_Selected = -1;
    float m_TempSize[3] = { 1,1,1 };
    float m_TempRot[3] = { 0,0,0 };
    float m_TempPos[3] = { 0,0,0 };

    //º»
    std::vector<BoneInfoBin> m_Bones;

    vector<_int>			m_BoneIndices;
    vector<_float4x4>		m_OffsetMatrices;
    ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
    ID3D11DepthStencilView* m_pDepthStencilView = nullptr;

    const aiScene* m_pAIScene = { nullptr };
    Assimp::Importer m_Importer = {};
    std::vector<ID3D11ShaderResourceView*> m_SRVs[AI_TEXTURE_TYPE_MAX];

public:
    static CMainApp* Create();
    virtual void Free() override;
};

NS_END
