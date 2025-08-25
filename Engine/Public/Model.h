#pragma once

#include "Component.h"
#include <DirectXCollision.h>
#include "BinType.h"

NS_BEGIN(Engine)

class ENGINE_DLL CModel final : public CComponent
{
private:
    CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CModel(const CModel& Prototype);
    virtual ~CModel() = default;

public:
    _uint Get_NumMeshes() const { return m_iNumMeshes; }
    _float4x4* Get_BoneMatrix(const _char* pBoneName);

public:
    virtual HRESULT Initialize_Prototype(MODELTYPE eModelType, FILETYPE eFileType, const _char* pModelFilePath, _fmatrix PreTransformMatrix);
    virtual HRESULT Initialize(void* pArg);
    virtual HRESULT Render(_uint iMeshIndex);

public:
    void Set_Animation(_uint iIndex, _bool isLoop = false, float blendDuration = 0.f, bool forceRestart = false);

public:
    HRESULT Bind_Materials(class CShader* pShader, const _char* pConstantName, _uint iMeshIndex, aiTextureType eTextureType, _uint iIndex);
    HRESULT Bind_Materials_Bin(CShader* pShader, const _char* pConstantName, _uint iMeshIndex, int texType, _uint iIndex);
    HRESULT Bind_BoneMatrices(class CShader* pShader, const _char* pConstantName, _uint iMeshIndex);
    _bool   Play_Animation(_float fTimeDelta);

public:
    // ★ FBX일 때만 실제 바운딩박스 계산, BIN은 "없어도 됨" 규칙에 따라 0 extents 반환
    void ComputeBoundingBox(DirectX::BoundingBox& outBox) const;

private:
    /* 파일로부터 읽은 모든 정보를 다 저장해주는 구조체. */
    const aiScene* m_pAIScene = { nullptr };
    Assimp::Importer       m_Importer = {};
    MODELTYPE              m_eModelType = {};
    _float4x4              m_PreTransformMatrix = {};

    // ★ 추가: BIN 로드 여부 플래그
    bool                   m_bFromBin = false;

private:
    _uint                  m_iNumMeshes = {};
    vector<class CMesh*>   m_Meshes;

private:
    _uint                           m_iNumMaterials = {};
    vector<class CMeshMaterial*>    m_Materials;

private:
    vector<class CBone*>            m_Bones;

private:
    _int                            m_iCurrentAnimIndex = { -1 };
    _uint                           m_iNumAnimations = { 0 };
    vector<class CAnimation*>       m_Animations;
    _bool                           m_isLoop = {};
    _bool                           m_isFinished = {};

public:
    _int   m_iNextAnimIndex = -1;
    _bool  m_inTransition = false;
    _float m_blendDur = 0.f;
    _float m_blendAcc = 0.f;

    // 포즈 버퍼
    std::vector<TRS>     m_poseCur, m_poseNext;
    std::vector<uint8_t> m_hasCur, m_hasNext;

private:
    HRESULT Ready_Meshes();
    HRESULT Ready_Materials(const _char* pModelFilePath);
    HRESULT Ready_Bones(const aiNode* pAINode, _int iParentIndex);
    HRESULT Ready_Animations();

private:
    HRESULT Ready_Meshes(ifstream& ifs, MODELTYPE eModelType);
    HRESULT Ready_Materials(const _char* pModelFilePath, const vector<MaterialInfoBin2>& binMaterials);
    HRESULT Ready_Bones(const vector<BoneInfoBin>& binBones, _int iParentIndex);
    HRESULT Ready_Animations(ifstream& ifs);

public:
    static CModel* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eModelType, FILETYPE eFileType, const _char* pModelFilePath, _fmatrix PreTransformMatrix);
    virtual CComponent* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END
