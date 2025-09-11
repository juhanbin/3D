#include "VIBuffer_Instance.h"

CVIBuffer_Instance::CVIBuffer_Instance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer{ pDevice, pContext }
{

}

CVIBuffer_Instance::CVIBuffer_Instance(const CVIBuffer_Instance& Prototype)
	: CVIBuffer{ Prototype }
	, m_iNumInstance{ Prototype.m_iNumInstance }
	, m_iNumIndexPerInstance{ Prototype.m_iNumIndexPerInstance }
	, m_pInstanceVertices{ Prototype.m_pInstanceVertices }
	, m_iInstanceVertexStride{ Prototype.m_iInstanceVertexStride }
	, m_VBInstanceDesc{ Prototype.m_VBInstanceDesc }
{

}

HRESULT CVIBuffer_Instance::Initialize_Prototype(const INSTANCE_DESC* pDesc)
{
	return S_OK;
}

HRESULT CVIBuffer_Instance::Initialize(void* pArg)
{
	D3D11_SUBRESOURCE_DATA	InitialDesc{};
	InitialDesc.pSysMem = m_pInstanceVertices;

	if (FAILED(m_pDevice->CreateBuffer(&m_VBInstanceDesc, &InitialDesc, &m_pVBInstance)))
		return E_FAIL;

	return S_OK;
}

HRESULT CVIBuffer_Instance::Bind_Resources()
{
	ID3D11Buffer* pVertexBuffers[] = {
		m_pVB,
		m_pVBInstance,
	};

	_uint		iVertexStrides[] = {
		m_iVertexStride,
		m_iInstanceVertexStride,
	};

	_uint		iOffsets[] = {
		0,
		0
	};

	m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pVertexBuffers, iVertexStrides, iOffsets);
	m_pContext->IASetIndexBuffer(m_pIB, m_eIndexFormat, 0);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

	return S_OK;
}

HRESULT CVIBuffer_Instance::Render()
{
	m_pContext->DrawIndexedInstanced(m_iNumIndexPerInstance, m_iNumInstance, 0, 0, 0);

	return S_OK;


}

void CVIBuffer_Instance::Free()
{
	__super::Free();

	if (false == m_isCloned)
		Safe_Delete_Array(m_pInstanceVertices);

	Safe_Release(m_pVBInstance);
}
