#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer_Instance abstract : public CVIBuffer
{
public:
	/* 인스턴싱을 이용하는 모든 입자들에게 피요한 데이터 */
	typedef struct tagInstanceDesc
	{
		_uint			iNumInstance;
		_float3			vCenter;
		_float3			vRange;
		_float2			vSize;

	}INSTANCE_DESC;
protected:
	CVIBuffer_Instance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVIBuffer_Instance(const CVIBuffer_Instance& Prototype);
	virtual ~CVIBuffer_Instance() = default;

public:
	virtual HRESULT Initialize_Prototype(const INSTANCE_DESC* pDesc);
	virtual HRESULT Initialize(void* pArg) override;
	virtual HRESULT Bind_Resources() override;
	virtual HRESULT Render() override;

protected:
	ID3D11Buffer* m_pVBInstance = { nullptr };
	_uint					m_iNumInstance = {};
	_uint					m_iNumIndexPerInstance = {};
	_uint					m_iInstanceVertexStride = {};


protected:
	D3D11_BUFFER_DESC		m_VBInstanceDesc{};
	void* m_pInstanceVertices = { nullptr };

public:
	virtual CComponent* Clone(void* pArg) = 0;
	virtual void Free();
};

NS_END