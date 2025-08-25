#pragma once

#include "VIBuffer.h"
#include "BinType.h"
NS_BEGIN(Engine)

class ENGINE_DLL CMesh final : public CVIBuffer
{
private:
    CMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CMesh(const CMesh& Prototype);
    virtual ~CMesh() = default;

public:
    _uint Get_MaterialIndex() const { return m_iMaterialIndex; }

public:
    virtual HRESULT Initialize_Prototype(MODELTYPE eType, const aiMesh* pAIMesh, const vector<class CBone*>& Bones, _fmatrix PreTransformMatrix);
    virtual HRESULT Initialize_Prototype(MODELTYPE eType, MeshInfoBin& meshInfos, const std::vector<VTXMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix);
    virtual HRESULT Initialize_Prototype(MODELTYPE eType, MeshInfoBin& meshInfo,
        const std::vector<VTXANIMMESH>& verts, const vector<uint32_t>& indices,
        const std::vector<MeshBoneRaw>& meshBones,
        const vector<CBone*>& Bones, _fmatrix /*PreTransformMatrix*/);
    virtual HRESULT Initialize(void* pArg) override;

public:
    HRESULT Bind_BoneMatrices(class CShader* pShader, const _char* pConstantName, const vector<class CBone*>& Bones);

private:
    _char     m_szName[MAX_PATH] = {};
    _uint     m_iMaterialIndex = {};
    _uint     m_iNumBones = {};

    vector<_int>          m_BoneIndices;
    _float4x4             m_BoneMatrices[g_iMaxNumBones] = {};
    vector<_float4x4>     m_OffsetMatrices;

    // ★ CPU 피킹용 데이터
    vector<_float3>       m_vecPositions;
    vector<uint32_t>      m_vecIndices;

public:
    // ★ 접근자 (피킹에서 사용)
    const vector<_float3>& GetPositions() const { return m_vecPositions; }
    const vector<uint32_t>& GetIndices()   const { return m_vecIndices; }

private:
    HRESULT Ready_Vertices_For_NonAnim(const aiMesh* pAIMesh, _fmatrix PreTransformMatrix);
    HRESULT Ready_Vertices_For_NonAnim(const vector<VTXMESH>& verts, _fmatrix PreTransformMatrix);
    HRESULT Ready_Vertices_For_Anim(const aiMesh* pAIMesh, const vector<CBone*>& Bones);
    HRESULT Ready_Vertices_For_Anim(const vector<VTXANIMMESH>& verts, const vector<CBone*>& Bones);

public:
    static CMesh* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eType, const aiMesh* pAIMesh, const vector<class CBone*>& Bones, _fmatrix PreTransformMatrix);
    static CMesh* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eType, MeshInfoBin& meshInfos, const vector<VTXMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix);
    static CMesh* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eType, MeshInfoBin& meshInfo, const vector<VTXANIMMESH>& verts, const vector<uint32_t>& indices, const vector<MeshBoneRaw>& meshBones, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix);
    virtual CComponent* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END
