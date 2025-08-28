#include "Collider.h"



#include "GameInstance.h"

CCollider::CCollider(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent { pDevice, pContext }
{

}

CCollider::CCollider(const CCollider& Prototype)
	: CComponent{ Prototype }
	, m_eType { Prototype.m_eType }
#ifdef _DEBUG
	, m_pBatch { Prototype.m_pBatch }
	, m_pEffect { Prototype.m_pEffect } 
#endif
	, m_pInputLayout { Prototype.m_pInputLayout}
	, m_isColl { Prototype.m_isColl}
{
	Safe_AddRef(m_pInputLayout);
}

HRESULT CCollider::Initialize_Prototype(COLLIDER eType)
{
	m_eType = eType;

#ifdef _DEBUG
	m_pBatch = new PrimitiveBatch<VertexPositionColor>(m_pContext);
	m_pEffect = new BasicEffect(m_pDevice);

	m_pEffect->SetVertexColorEnabled(true);

	const void* pShaderByteCode = { nullptr };
	size_t		iShaderByteCodeLength = {};

	m_pEffect->GetVertexShaderBytecode(&pShaderByteCode, &iShaderByteCodeLength);

	if (FAILED(m_pDevice->CreateInputLayout(VertexPositionColor::InputElements, VertexPositionColor::InputElementCount,
		pShaderByteCode, iShaderByteCodeLength, &m_pInputLayout)))
		return E_FAIL;
	
#endif

	return S_OK;
}

HRESULT CCollider::Initialize(void* pArg)
{
	CBounding::BOUNDING_DESC* pDesc = static_cast<CBounding::BOUNDING_DESC*>(pArg);

	switch (m_eType)
	{
	case COLLIDER::AABB:
		m_pBounding = CBounding_AABB::Create(m_pDevice, m_pContext, pDesc);
		break;
	case COLLIDER::OBB:
		m_pBounding = CBounding_OBB::Create(m_pDevice, m_pContext, pDesc);
		break;
	case COLLIDER::SPHERE:
		m_pBounding = CBounding_Sphere::Create(m_pDevice, m_pContext, pDesc);
		break;
	}	

	return S_OK;
}

void CCollider::Update(_fmatrix WorldMatrix)
{
	m_pBounding->Update(WorldMatrix);
}

_bool CCollider::Intersect(CCollider* pTarget)
{
	return m_isColl = m_pBounding->Intersect(pTarget->m_eType, pTarget->m_pBounding);
}

#ifdef _DEBUG

HRESULT CCollider::Render()
{
	m_pEffect->SetWorld(XMMatrixIdentity());
	m_pEffect->SetView(m_pGameInstance->Get_Transform_Matrix(D3DTS::VIEW));
	m_pEffect->SetProjection(m_pGameInstance->Get_Transform_Matrix(D3DTS::PROJ));

	m_pContext->IASetInputLayout(m_pInputLayout);
	m_pEffect->Apply(m_pContext);	

	m_pBatch->Begin();

	m_pBounding->Render(m_pBatch, m_isColl == true ? XMVectorSet(1.f, 0.f, 0.f, 1.f) : XMVectorSet(0.f, 1.f, 0.f, 1.f));

	m_pBatch->End();

	return S_OK;
}

#endif

CCollider* CCollider::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, COLLIDER eType)
{
	CCollider* pInstance = new CCollider(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(eType)))
	{
		MSG_BOX(TEXT("Failed to Created : CCollider"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CCollider::Clone(void* pArg)
{
	CCollider* pInstance = new CCollider(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX(TEXT("Failed to Cloned : CCollider"));
		Safe_Release(pInstance);
	}

	return pInstance;
}


void CCollider::Free()
{
	__super::Free();

	if (false == m_isCloned)
	{
		Safe_Delete(m_pEffect);
		Safe_Delete(m_pBatch);
	}
	Safe_Release(m_pInputLayout);
	Safe_Release(m_pBounding);
}
