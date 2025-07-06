#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer abstract : public CComponent
{
protected:
	CVIBuffer(ID3D11Device * pDevice, ID3D11DeviceContext * pContext);
	CVIBuffer(const CVIBuffer& Prototype);
	virtual ~CVIBuffer() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);

protected:
	ID3D11Buffer*	m_pVB = { nullptr };
	ID3D11Buffer*	m_pIB = { nullptr };

	_uint			m_iNumVertices = {};
	_uint			m_iVertexStride = {};


public:
	virtual CComponent* Clone(void* pArg) = 0;
	virtual void Free();
};

NS_END
