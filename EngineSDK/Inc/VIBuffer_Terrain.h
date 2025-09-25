#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer_Terrain final : public CVIBuffer
{
private:
	CVIBuffer_Terrain(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVIBuffer_Terrain(const CVIBuffer_Terrain& Prototype);
	virtual ~CVIBuffer_Terrain() = default;

public:
	virtual HRESULT Initialize_Prototype(const _tchar* pHeightMapFilePath);
	virtual HRESULT Initialize(void* pArg) override;
	HRESULT Initialize_Flat(_uint nx, _uint nz, float cell = 1.f, float baseHeight = 0.f);
	static CVIBuffer_Terrain* CreateFlat(ID3D11Device* d, ID3D11DeviceContext* c,
		_uint nx, _uint nz, float cell = 1.f, float baseHeight = 0.f);
public:
	void Culling(_fmatrix WorldMatrix);

private:
	_uint				m_iNumVerticesX = { };
	_uint				m_iNumVerticesZ = { };

	//class CQuadTree* m_pQuadTree = { nullptr };

public:
	static CVIBuffer_Terrain* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pHeightMapFilePath);
	virtual CComponent* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END