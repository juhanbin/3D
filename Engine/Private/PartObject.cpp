#include "PartObject.h"
#include "Transform.h"

CPartObject::CPartObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject{ pDevice, pContext }
{
}

CPartObject::CPartObject(const CPartObject& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CPartObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CPartObject::Initialize(void* pArg)
{
	PARTOBJECT_DESC* pDesc = static_cast<PARTOBJECT_DESC*>(pArg);

	m_pParentMatrix = pDesc->pParentMatrix;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CPartObject::Priority_Update(_float fTimeDelta)
{

}

void CPartObject::Update(_float fTimeDelta)
{

}

void CPartObject::Late_Update(_float fTimeDelta)
{

}

HRESULT CPartObject::Render()
{
	return S_OK;
}


void CPartObject::Update_CombinedMatrix() {

	/* ctrl + shift + v */
	XMStoreFloat4x4(&m_CombinedWorldMatrix, m_pTransformCom->Get_WorldMatrix() * XMLoadFloat4x4(m_pParentMatrix));
}

void CPartObject::Free()
{
	__super::Free();


}
