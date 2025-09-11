#include "VIBuffer_Rect_Instance.h"

#include "GameInstance.h"

CVIBuffer_Rect_Instance::CVIBuffer_Rect_Instance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer_Instance{ pDevice, pContext }
{
}

CVIBuffer_Rect_Instance::CVIBuffer_Rect_Instance(const CVIBuffer_Rect_Instance& Prototype)
	: CVIBuffer_Instance{ Prototype }

{
}

HRESULT CVIBuffer_Rect_Instance::Initialize_Prototype(const INSTANCE_DESC* pDesc)
{
	const RECT_INSTANCE_DESC* pRectDesc = static_cast<const RECT_INSTANCE_DESC*>(pDesc);

	m_iNumIndexPerInstance = 6;
	m_iInstanceVertexStride = sizeof(VTXINSTANCE_PARTICLE);
	m_iNumInstance = pRectDesc->iNumInstance;
	m_iNumVertices = 4;
	m_iVertexStride = sizeof(VTXPOSTEX);
	m_iNumIndices = 6;
	m_iIndexStride = 2;
	m_iNumVertexBuffers = 2;
	m_eIndexFormat = DXGI_FORMAT_R16_UINT;
	m_ePrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	D3D11_BUFFER_DESC		VBDesc{};
	VBDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
	VBDesc.Usage = D3D11_USAGE_DEFAULT;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.CPUAccessFlags = 0;
	VBDesc.MiscFlags = 0;
	VBDesc.StructureByteStride = m_iVertexStride;

	VTXPOSTEX* pVertices = new VTXPOSTEX[m_iNumVertices];

	pVertices[0].vPosition = _float3(-0.5f, 0.5f, 0.f);
	pVertices[0].vTexcoord = _float2(0.f, 0.f);

	pVertices[1].vPosition = _float3(0.5f, 0.5f, 0.f);
	pVertices[1].vTexcoord = _float2(1.f, 0.f);

	pVertices[2].vPosition = _float3(0.5f, -0.5f, 0.f);
	pVertices[2].vTexcoord = _float2(1.f, 1.f);

	pVertices[3].vPosition = _float3(-0.5f, -0.5f, 0.f);
	pVertices[3].vTexcoord = _float2(0.f, 1.f);

	D3D11_SUBRESOURCE_DATA	VBInitialData{};
	VBInitialData.pSysMem = pVertices;

	if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
		return E_FAIL;

	Safe_Delete_Array(pVertices);

	D3D11_BUFFER_DESC		IBDesc{};
	IBDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
	IBDesc.Usage = D3D11_USAGE_DEFAULT;
	IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IBDesc.CPUAccessFlags = 0;
	IBDesc.MiscFlags = 0;
	IBDesc.StructureByteStride = m_iIndexStride;

	_ushort* pIndices = new _ushort[m_iNumIndices];

	pIndices[0] = 0;
	pIndices[1] = 1;
	pIndices[2] = 2;

	pIndices[3] = 0;
	pIndices[4] = 2;
	pIndices[5] = 3;

	D3D11_SUBRESOURCE_DATA	IBInitialData{};
	IBInitialData.pSysMem = pIndices;

	if (FAILED(m_pDevice->CreateBuffer(&IBDesc, &IBInitialData, &m_pIB)))
		return E_FAIL;

	Safe_Delete_Array(pIndices);


	/* 복제되는 렉트인스턴스버퍼는 각각 m_pVBInstance독립적으로 가지길 바란다. */
	/* 실질적인 인스턴스 정점버퍼의 경우에는 사본이 생성될때 할당되게 만들어줄 것이다. */
	/* dx11에서는 인스턴스 정점버퍼를 할당할 경우 복사해서 채워줄 정점의 데이터가 사전에 필요하다 */

	/* 복사해서 채워줄 정점의 사전데이터다.(사본마다 하나씩 만들어줄 이유강벗으나가ㅓ  */
	m_VBInstanceDesc.ByteWidth = m_iNumInstance * m_iInstanceVertexStride;
	m_VBInstanceDesc.Usage = D3D11_USAGE_DYNAMIC;
	m_VBInstanceDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	m_VBInstanceDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	m_VBInstanceDesc.MiscFlags = 0;
	m_VBInstanceDesc.StructureByteStride = m_iInstanceVertexStride;

	m_pInstanceVertices = new VTXINSTANCE_PARTICLE[m_iNumInstance];

	for (size_t i = 0; i < m_iNumInstance; i++)
	{
		VTXINSTANCE_PARTICLE* pInstanceVertices = static_cast<VTXINSTANCE_PARTICLE*>(m_pInstanceVertices);

		_float		fScale = m_pGameInstance->Rand(pRectDesc->vSize.x, pRectDesc->vSize.y);
		_float		fLifeTime = m_pGameInstance->Rand(pRectDesc->vLifeTime.x, pRectDesc->vLifeTime.y);

		pInstanceVertices[i].vRight = _float4(fScale, 0.f, 0.f, 0.f);
		pInstanceVertices[i].vUp = _float4(0.f, fScale, 0.f, 0.f);
		pInstanceVertices[i].vLook = _float4(0.f, 0.f, fScale, 0.f);
		pInstanceVertices[i].vTranslation = _float4(
			m_pGameInstance->Rand(pRectDesc->vCenter.x - pRectDesc->vRange.x * 0.5f, pRectDesc->vCenter.x + pRectDesc->vRange.x * 0.5f),
			m_pGameInstance->Rand(pRectDesc->vCenter.y - pRectDesc->vRange.y * 0.5f, pRectDesc->vCenter.y + pRectDesc->vRange.y * 0.5f),
			m_pGameInstance->Rand(pRectDesc->vCenter.z - pRectDesc->vRange.z * 0.5f, pRectDesc->vCenter.z + pRectDesc->vRange.z * 0.5f),
			1.f
		);

		pInstanceVertices[i].vLifeTime = _float2(0.f, fLifeTime);

	}

	return S_OK;
}

HRESULT CVIBuffer_Rect_Instance::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;


	return S_OK;
}

void CVIBuffer_Rect_Instance::Spread(_float fTimeDelta)
{
}

void CVIBuffer_Rect_Instance::Drop(_float fTimeDelta)
{
}

CVIBuffer_Rect_Instance* CVIBuffer_Rect_Instance::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const INSTANCE_DESC* pDesc)
{
	CVIBuffer_Rect_Instance* pInstance = new CVIBuffer_Rect_Instance(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(pDesc)))
	{
		MSG_BOX(TEXT("Failed to Created : CVIBuffer_Rect_Instance"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CVIBuffer_Rect_Instance::Clone(void* pArg)
{
	CVIBuffer_Rect_Instance* pInstance = new CVIBuffer_Rect_Instance(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX(TEXT("Failed to Cloned : CVIBuffer_Rect_Instance"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CVIBuffer_Rect_Instance::Free()
{
	__super::Free();


}
