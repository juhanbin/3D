#include "VIBuffer.h"

CVIBuffer::CVIBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent { pDevice, pContext }
{

}

CVIBuffer::CVIBuffer(const CVIBuffer& Prototype)
	: CComponent{ Prototype }
	, m_pVB { Prototype.m_pVB }
	, m_pIB { Prototype.m_pIB }
	, m_iNumVertices{ Prototype.m_iNumVertices }
	, m_iVertexStride { Prototype.m_iVertexStride }
	, m_iNumIndices { Prototype.m_iNumIndices }
	, m_iIndexStride { Prototype.m_iIndexStride }
	, m_iNumVertexBuffers { Prototype.m_iNumVertexBuffers }
	, m_eIndexFormat{ Prototype.m_eIndexFormat }
	, m_ePrimitiveType { Prototype.m_ePrimitiveType }
{
	Safe_AddRef(m_pVB);
	Safe_AddRef(m_pIB);

}

HRESULT CVIBuffer::Initialize_Prototype()
{

	return S_OK;
}

HRESULT CVIBuffer::Initialize(void* pArg)
{

	return S_OK;
}

HRESULT CVIBuffer::Bind_Resources()
{
	ID3D11Buffer* pVertexBuffers[] = {
		m_pVB,				
	};

	_uint		iVertexStrides[] = {
		m_iVertexStride, 
	};

	_uint		iOffsets[] = {
		0
	};

	m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pVertexBuffers, iVertexStrides, iOffsets);
	m_pContext->IASetIndexBuffer(m_pIB, m_eIndexFormat, 0);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

	/* 장치에게 내 정점이 어떻게 생겼다? (FVF) : 폐기 */
	/* 내가 그리기위해 이용하려하는 쉐이더와 정점이 서로 호환이 되는지에 대한 검사과정을 수행해야한다. */

	//D3D11_INPUT_ELEMENT_DESC		Elements[] = {
	//	{}, 
	//	{}, 
	//	{}, 
	//};


	//m_pDevice->CreateInputLayout(Elements, 3, );
	//m_pContext->IASetInputLayout();

	//struct tagVertexPositionTexcoord
	//{
	//	XMFLOAT3		vPosition;
	//	XMFLOAT3		vNormal;
	//	XMFLOAT2		vTexcoord;
	//};

	//struct VS_IN
	//{
	//	float3		vPosition : POSITION;
	//	float2		vTexcoord : TEXCOORD0;
	//};

	return S_OK;
}

HRESULT CVIBuffer::Render()
{
	m_pContext->DrawIndexed(m_iNumIndices, 0, 0);

	return S_OK;
}

void CVIBuffer::Free()
{
	__super::Free();

	Safe_Release(m_pIB);
	Safe_Release(m_pVB);
}
