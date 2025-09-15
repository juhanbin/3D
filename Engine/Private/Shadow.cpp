#include "Shadow.h"

CShadow::CShadow()
{
}

const _float4x4* CShadow::Get_Transform_Float4x4(D3DTS eTransformState) const
{
	return &m_Matrices[ENUM_CLASS(eTransformState)];
}

HRESULT CShadow::Initialize(_uint iWinSizeX, _uint iWinSizeY)
{
	m_fViewportWidth = iWinSizeX;
	m_fViewportHeight = iWinSizeY;

	return S_OK;
}

HRESULT CShadow::Ready_ShadowLight(SHADOW_LIGHT_DESC LightDesc)
{
	XMStoreFloat4x4(&m_Matrices[ENUM_CLASS(D3DTS::VIEW)],
		XMMatrixLookAtLH(XMLoadFloat4(&LightDesc.vEye), XMLoadFloat4(&LightDesc.vAt), XMVectorSet(0.f, 1.f, 0.f, 0.f)));

	XMStoreFloat4x4(&m_Matrices[ENUM_CLASS(D3DTS::PROJ)],
		XMMatrixPerspectiveFovLH(LightDesc.fFovy, m_fViewportWidth / m_fViewportHeight, LightDesc.fNear, LightDesc.fFar));

	return S_OK;
}

CShadow* CShadow::Create(_uint iWinSizeX, _uint iWinSizeY)
{
	CShadow* pInstance = new CShadow();

	if (FAILED(pInstance->Initialize(iWinSizeX, iWinSizeY)))
	{
		MSG_BOX(TEXT("Failed to Created : CShadow"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CShadow::Free()
{
	__super::Free();


}
