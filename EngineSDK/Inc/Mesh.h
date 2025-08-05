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
	_uint Get_MaterialIndex() const {
		return m_iMaterialIndex;
	}



public:
	virtual HRESULT Initialize_Prototype(MODELTYPE eType, const aiMesh* pAIMesh, const vector<class CBone*>& Bones, _fmatrix PreTransformMatrix);
	virtual HRESULT Initialize_Prototype(MODELTYPE eType, MeshInfoBin& meshInfos, const std::vector<VTXMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix);
	virtual HRESULT Initialize_Prototype(MODELTYPE eType, MeshInfoBin& meshInfos, const std::vector<VTXANIMMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix);
	virtual HRESULT Initialize(void* pArg) override;

public:
	HRESULT Bind_BoneMatrices(class CShader* pShader, const _char* pConstantName, const vector<class CBone*>& Bones);

private:
	_char			m_szName[MAX_PATH] = {};
	_uint			m_iMaterialIndex = {};
	_uint			m_iNumBones = {};
	/* 메시 당 이 메시에 영향을 주는 뼈들을 모아서 저장하는 이유? */
	/* : 점이 뼈를 따라가기위해 점마다 정의되어있는 BlendIndex 전체 뼈 기준이 아닌 이 메시에 영향을 주는 뼈 기준의 인덱스로 저장되어있다. */
	/* : 정점의 블렌드 인덱스에 보관된 인덱스로 바로 메시에 영향을 주주는 뼈들 중 특정 뼈에 접근하기위해. */

	/* 이 메시에 영향을 주는 뼈들의 인덱스(전체뼈기준) 집합. */
	/* 전체 뼈기준 인덱스를 저장해주는 이유? */
	/* 이 뼈에게 기대하는 데이터 -> 행렬(CombinedTransformationMatrix)을 가져오고자한다. */
	/* 행렬은 CBone에 저장되어있다. */
	vector<_int>			m_BoneIndices;
	_float4x4				m_BoneMatrices[g_iMaxNumBones] = {};

	vector<_float4x4>		m_OffsetMatrices;

	vector<_float3>			m_vecPositions;

public:
	const vector<_float3>& GetPositions() const { return m_vecPositions; }

private:
	HRESULT Ready_Vertices_For_NonAnim(const aiMesh* pAIMesh, _fmatrix PreTransformMatrix);
	HRESULT Ready_Vertices_For_NonAnim(const vector<VTXMESH>& verts, _fmatrix PreTransformMatrix);
	HRESULT Ready_Vertices_For_Anim(const aiMesh* pAIMesh, const vector<CBone*>& Bones);
	HRESULT Ready_Vertices_For_Anim(const vector<VTXANIMMESH>& verts, const vector<CBone*>& Bones);

public:
	static CMesh* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eType, const aiMesh* pAIMesh, const vector<class CBone*>& Bones, _fmatrix PreTransformMatrix);
	static CMesh* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eType, MeshInfoBin& meshInfos, const vector<VTXMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix);
	static CMesh* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eType, MeshInfoBin& meshInfos, const vector<VTXANIMMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix);
	virtual CComponent* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END