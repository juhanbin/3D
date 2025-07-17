#pragma once

#include "Component.h"

/* ���� �����δ� -> ������ �����δ� -> ��� ������ ���� ������ ������ �����ϱⰡ ����� */
/* -> ���� �����̰Բ� ó�����ָ� �����. -> � Ÿ�ֿ̹� � ���¸� ������ � ���� ���������ϴ����� ���� ������ �ʿ��ϴ�. */
/* �տ��� �̾߱��� �������� �ִϸ��̼��̶�� �θ���. */

/* 1. �� ��ü�� ����. */
/* 2. �������� ��ü � ���� ������ ���� ���ŵǾ��ϴ°��� ���� ������ �ʿ��ϴ�. */
/* 3. �ִϸ��̼�����(������ �ð��� ���� ���°���)�� �ε��Ѵ�. */

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
	/* ���Ϸκ��� ���� ��� ������ �� �������ִ� ����ü. */
	const aiScene* m_pAIScene = { nullptr };
	Assimp::Importer		m_Importer = {};
	MODELTYPE				m_eModelType = {};
	_float4x4				m_PreTransformMatrix = {};

	// m_pAIScene = m_Importer.ReadFile(���);

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