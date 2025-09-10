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

    // ---------- 애니메이션 진행/이벤트 질의 (클라에서 사용) ----------
public:
    // 현재 재생 중 애니의 정규화 진행도(0~1) ? 블렌딩 중에는 마지막 업데이트한 값 유지
    float GetAnimProgress01() const { return m_curAnim01; }

    // 지난 틱에서 이번 틱 사이에 t01(0~1) 지점을 "통과"했는가? (루프/되감김 고려)
    bool  AnimCrossedNormalized(float t01) const;

    // 애니 전환/리셋 시 이벤트 창 초기화
    void  ClearAnimEventWindow() { m_prevAnim01 = m_curAnim01 = 0.f; }

    // 현재/다음 애니 인덱스 조회(원하면 디버그용으로)
    int   GetCurrentAnimIndex() const { return m_iCurrentAnimIndex; }
    int   GetNextAnimIndex()    const { return m_iNextAnimIndex; }
    bool  IsInTransition()      const { return m_inTransition; }

private:
    const aiScene* m_pAIScene = { nullptr };
    Assimp::Importer          m_Importer = {};
    MODELTYPE                 m_eModelType = {};
    _float4x4                 m_PreTransformMatrix = {};
    bool                      m_bFromBin = false;

private:
    _uint                     m_iNumMeshes = {};
    vector<class CMesh*>      m_Meshes;

private:
    _uint                     m_iNumMaterials = {};
    vector<class CMeshMaterial*> m_Materials;

private:
    vector<class CBone*>      m_Bones;

private:
    _int                      m_iCurrentAnimIndex = { -1 };
    _uint                     m_iNumAnimations = { 0 };
    vector<class CAnimation*> m_Animations;
    _bool                     m_isLoop = {};
    _bool                     m_isFinished = {};

public:
    _int   m_iNextAnimIndex = -1;
    _bool  m_inTransition = false;
    _float m_blendDur = 0.f;
    _float m_blendAcc = 0.f;

    // 포즈 버퍼(블렌딩용)
    std::vector<TRS>         m_poseCur, m_poseNext;
    std::vector<uint8_t>     m_hasCur, m_hasNext;

    // ---- 이벤트용 진행도 샘플(비블렌딩 구간 기준) ----
private:
    float m_prevAnim01 = 0.f;
    float m_curAnim01 = 0.f;

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
    static CModel* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
        MODELTYPE eModelType, FILETYPE eFileType,
        const _char* pModelFilePath,
        _fmatrix PreTransformMatrix = DirectX::XMMatrixIdentity());
    virtual CComponent* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END
