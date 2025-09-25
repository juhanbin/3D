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
    _uint       Get_NumMeshes() const { return m_iNumMeshes; }
    _float4x4* Get_BoneMatrix(const _char* pBoneName);

public:
    virtual HRESULT Initialize_Prototype(MODELTYPE eModelType, FILETYPE eFileType,
        const _char* pModelFilePath, _fmatrix PreTransformMatrix);
    virtual HRESULT Initialize(void* pArg);
    virtual HRESULT Render(_uint iMeshIndex);

public:
    void  Set_Animation(_uint iIndex, _bool isLoop = false,
        float blendDuration = 0.f, bool forceRestart = false);
    _bool Play_Animation(_float fTimeDelta);

public:
    HRESULT Bind_Materials(class CShader* pShader, const _char* pConstantName,
        _uint iMeshIndex, aiTextureType eTextureType, _uint iIndex);
    HRESULT Bind_Materials_Bin(CShader* pShader, const _char* pConstantName,
        _uint iMeshIndex, int texType, _uint iIndex);
    HRESULT Bind_BoneMatrices(class CShader* pShader, const _char* pConstantName, _uint iMeshIndex);

public:
    float GetAnimProgress01() const { return m_curAnim01; }
    bool  AnimCrossedNormalized(float t01) const;
    void  ClearAnimEventWindow() { m_prevAnim01 = m_curAnim01 = 0.f; }
    int   GetCurrentAnimIndex() const { return m_iCurrentAnimIndex; }
    int   GetNextAnimIndex()    const { return m_iNextAnimIndex; }
    bool  IsInTransition()      const { return m_inTransition; }

private:
    const aiScene* m_pAIScene = { nullptr };
    Assimp::Importer         m_Importer = {};
    MODELTYPE                m_eModelType = {};
    _float4x4                m_PreTransformMatrix = {};
    bool                     m_bFromBin = false;

private:
    _uint                    m_iNumMeshes = {};
    vector<class CMesh*>     m_Meshes;

private:
    _uint                    m_iNumMaterials = {};
    vector<class CMeshMaterial*> m_Materials;

private:
    vector<class CBone*>     m_Bones;

private:
    _int                     m_iCurrentAnimIndex = { -1 };
    _uint                    m_iNumAnimations = { 0 };
    vector<class CAnimation*> m_Animations;
    _bool                    m_isLoop = {};
    _bool                    m_isFinished = {};

public:
    _int   m_iNextAnimIndex = -1;
    _bool  m_inTransition = false;
    _float m_blendDur = 0.f;
    _float m_blendAcc = 0.f;

    std::vector<TRS>         m_poseCur, m_poseNext;
    std::vector<uint8_t>     m_hasCur, m_hasNext;

private:
    float m_prevAnim01 = 0.f;
    float m_curAnim01 = 0.f;

private:
    // ASSIMP 경로
    HRESULT Ready_Meshes();
    HRESULT Ready_Materials(const _char* pModelFilePath);
    HRESULT Ready_Bones(const aiNode* pAINode, _int iParentIndex);
    HRESULT Ready_Animations();

    // BIN 경로
    HRESULT Ready_Meshes(std::ifstream& ifs, MODELTYPE eModelType);
    HRESULT Ready_Materials(const _char* pModelFilePath, const vector<MaterialInfoBin2>& binMaterials);
    HRESULT Ready_Bones(const vector<BoneInfoBin>& binBones, _int iParentIndex);
    HRESULT Ready_Animations(std::ifstream& ifs);

public:
    static CModel* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
        MODELTYPE eModelType, FILETYPE eFileType,
        const _char* pModelFilePath,
        _fmatrix PreTransformMatrix = DirectX::XMMatrixIdentity());
    virtual CComponent* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END
