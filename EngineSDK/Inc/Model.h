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
	_uint Get_NumMeshes() const {
		return m_iNumMeshes;
	}

public:
	virtual HRESULT Initialize_Prototype(MODELTYPE eModelType, FILETYPE eFileType, const _char* pModelFilePath, _fmatrix PreTransformMatrix);
	virtual HRESULT Initialize(void* pArg);
	virtual HRESULT Render(_uint iMeshIndex);

public:
	void Set_Animation(_uint iIndex, _bool isLoop = false);

public:
	HRESULT Bind_Materials(class CShader* pShader, const _char* pConstantName, _uint iMeshIndex, aiTextureType eTextureType, _uint iIndex);
	HRESULT Bind_BoneMatrices(class CShader* pShader, const _char* pConstantName, _uint iMeshIndex);
	_bool Play_Animation(_float fTimeDelta);

public:
	void ComputeBoundingBox(DirectX::BoundingBox& outBox) const;

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
	vector<class CBone*>			m_Bones;

private:
	_uint							m_iCurrentAnimIndex = { 0 };
	_uint							m_iNumAnimations = { 0 };
	vector<class CAnimation*>		m_Animations;
	_bool							m_isLoop = {};
	_bool							m_isFinished = {};


private:
	HRESULT Ready_Meshes();
	HRESULT Ready_Materials(const _char* pModelFilePath);
	HRESULT Ready_Bones(const aiNode* pAINode, _int iParentIndex);
	HRESULT Ready_Animations();

private:
	HRESULT Ready_Meshes(ifstream& ifs);
	HRESULT Ready_Materials(const _char* pModelFilePath,const vector<MaterialInfo>& binMaterials);
	HRESULT Ready_Bones(const vector<BoneInfo>& binBones, _int iParentIndex);
	HRESULT Ready_Animations(ifstream& ifs);

public:
	static CModel* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eModelType, FILETYPE eFileType , const _char* pModelFilePath, _fmatrix PreTransformMatrix);
	virtual CComponent* Clone(void* pArg) override;
	virtual void Free() override;

};

NS_END