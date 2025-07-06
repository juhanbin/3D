#include "UIObject.h"
#include "Transform.h"

CUIObject::CUIObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject { pDevice, pContext }
{
}

CUIObject::CUIObject(const CUIObject& Prototype)
	: CGameObject{ Prototype}
{
}

HRESULT CUIObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CUIObject::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	UIOBJECT_DESC* pDesc = static_cast<UIOBJECT_DESC*>(pArg);

	m_fX = pDesc->fX;
	m_fY = pDesc->fY;
	m_fSizeX = pDesc->fSizeX;
	m_fSizeY = pDesc->fSizeY;

	/* 정해놓은 상태대로 유아이를 그리기위해서 월드행렬의 상태를 조절한다.(Transform) */
	/* 뷰, 투영행렬을 직교투영에 맞게끔 생성한다.(Interface) */

	/*D3DXMatrixOrthoLH();*/

	D3D11_VIEWPORT			Viewport{};

	_uint			iNumViewports = { 1 };

	m_pContext->RSGetViewports(&iNumViewports, &Viewport);	

	XMStoreFloat4x4(&m_ViewMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&m_ProjMatrix, XMMatrixOrthographicLH(Viewport.Width, Viewport.Height, 0.0f, 1.f));

	m_iWinSizeX = Viewport.Width;
	m_iWinSizeY = Viewport.Height;

	return S_OK;
}

void CUIObject::Priority_Update(_float fTimeDelta)
{
}

void CUIObject::Update(_float fTimeDelta)
{
}

void CUIObject::Late_Update(_float fTimeDelta)
{
}

HRESULT CUIObject::Render()
{
	return S_OK;
}

HRESULT CUIObject::Begin()
{
	/* 트랜스폼이 들고 있는 월드 행렬이 fx, fy, fsizex, fsizey로 뷰포트상에  그려질 수 있도록 보정한다. */
	m_pTransformCom->Scale(_float3(m_fSizeX, m_fSizeY, 1.f));
	m_pTransformCom->Set_State(STATE::POSITION, XMVectorSet(m_fX - m_iWinSizeX * 0.5f, -m_fY + m_iWinSizeY * 0.5f, 0.f, 1.f));

	return S_OK;
}

void CUIObject::Free()
{
	__super::Free();


}
