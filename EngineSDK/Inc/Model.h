#pragma once

#include "Component.h"

/* 모델이 움직인다 -> 정점이 움직인다 -> 모든 정점에 대한 움직임 정보를 저장하기가 힘들다 */
/* -> 뼈를 움직이게끔 처리해주면 조헥싿. -> 어떤 타이밍에 어떤 상태를 가지고 어떤 뼈가 움직여야하는지에 대한 정보가 필요하다. */
/* 앞에서 이야기한 정보들을 애니메이션이라고 부른다. */

/* 1. 뼈 자체의 생성. */
/* 2. 정점들은 대체 어떤 뼈의 정보를 따라서 갱신되야하는가에 대한 정보가 필요하다. */
/* 3. 애니메이션정보(뼈들의 시간에 따른 상태값들)를 로드한다. */

NS_BEGIN(Engine)

class ENGINE_DLL CModel final : public CComponent
{
private:
	CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CModel(const CModel& Prototype);
	virtual ~CModel() = default;

public:
	_uint Get_NumMeshes() const {
		return m_iNumMeshes;
	}

public:
	virtual HRESULT Initialize_Prototype(MODELTYPE eModelType, const _char* pModelFilePath, _fmatrix PreTransformMatrix);
	virtual HRESULT Initialize(void* pArg);
	virtual HRESULT Render(_uint iMeshIndex);

public:
	HRESULT Bind_Materials(class CShader* pShader, const _char* pConstantName, _uint iMeshIndex, aiTextureType eTextureType, _uint iIndex);

private:
	/* 파일로부터 읽은 모든 정보를 다 저장해주는 구조체. */
	const aiScene* m_pAIScene = { nullptr };
	Assimp::Importer		m_Importer = {};
	MODELTYPE				m_eModelType = {};
	_float4x4				m_PreTransformMatrix = {};

	// m_pAIScene = m_Importer.ReadFile(경로);

private:
	_uint					m_iNumMeshes = {};
	vector<class CMesh*>	m_Meshes;

private:
	/* Diffuse, Ambient, Specular */
	_uint							m_iNumMaterials = {};
	vector<class CMeshMaterial*>	m_Materials;

private:
	HRESULT Ready_Meshes();
	HRESULT Ready_Materials(const _char* pModelFilePath);
	//HRESULT Ready_Bones();

public:
	static CModel* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eModelType, const _char* pModelFilePath, _fmatrix PreTransformMatrix);
	virtual CComponent* Clone(void* pArg) override;
	virtual void Free() override;

};

NS_END