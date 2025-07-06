#include "Transform.h"

CTransform::CTransform(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent { pDevice, pContext }
{

}

HRESULT CTransform::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CTransform::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return S_OK;

	TRANSFORM_DESC* pDesc = static_cast<TRANSFORM_DESC*>(pArg);

	m_fSpeedPerSec = pDesc->fSpeedPerSec;
	m_fRotationPerSec = pDesc->fRotationPerSec;

	return S_OK;
}

void CTransform::Scale(_float3 vScale)
{
	Set_State(STATE::RIGHT, XMVector3Normalize(Get_State(STATE::RIGHT)) * vScale.x);
	Set_State(STATE::UP, XMVector3Normalize(Get_State(STATE::UP)) * vScale.y);
	Set_State(STATE::LOOK, XMVector3Normalize(Get_State(STATE::LOOK)) * vScale.z);
}

void CTransform::Scaling(_float3 vScale)
{
	Set_State(STATE::RIGHT, Get_State(STATE::RIGHT) * vScale.x);
	Set_State(STATE::UP, Get_State(STATE::UP) * vScale.y);
	Set_State(STATE::LOOK, Get_State(STATE::LOOK) * vScale.z);
}

void CTransform::Go_Straight(_float fTimeDelta)
{
	_vector		vPosition = Get_State(STATE::POSITION);
	_vector		vLook = Get_State(STATE::LOOK);

	vPosition += XMVector3Normalize(vLook) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform::Go_Left(_float fTimeDelta)
{
	_vector		vPosition = Get_State(STATE::POSITION);
	_vector		vRight = Get_State(STATE::RIGHT);

	vPosition -= XMVector3Normalize(vRight) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform::Go_Right(_float fTimeDelta)
{
	_vector		vPosition = Get_State(STATE::POSITION);
	_vector		vRight = Get_State(STATE::RIGHT);

	vPosition += XMVector3Normalize(vRight) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform::Go_Backward(_float fTimeDelta)
{
	_vector		vPosition = Get_State(STATE::POSITION);
	_vector		vLook = Get_State(STATE::LOOK);

	vPosition -= XMVector3Normalize(vLook) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

void CTransform::Rotation(_fvector vAxis, _float fRadian)
{
	_float3		vScaled = Get_Scaled();

	_vector		vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f) * vScaled.x;
	_vector		vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f) * vScaled.y;
	_vector		vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f) * vScaled.z;

	_matrix		RotationMatrix = XMMatrixRotationAxis(vAxis, fRadian);

	Set_State(STATE::RIGHT, XMVector4Transform(vRight, RotationMatrix));
	Set_State(STATE::UP, XMVector4Transform(vUp, RotationMatrix));
	Set_State(STATE::LOOK, XMVector4Transform(vLook, RotationMatrix));
	
	// XMVector3TransformNormal();


}

void CTransform::Turn(_fvector vAxis, _float fTimeDelta)
{
	_vector		vRight = Get_State(STATE::RIGHT);
	_vector		vUp = Get_State(STATE::UP);
	_vector		vLook = Get_State(STATE::LOOK);

	_matrix		RotationMatrix = XMMatrixRotationAxis(vAxis, m_fRotationPerSec * fTimeDelta);

	Set_State(STATE::RIGHT, XMVector4Transform(vRight, RotationMatrix));
	Set_State(STATE::UP, XMVector4Transform(vUp, RotationMatrix));
	Set_State(STATE::LOOK, XMVector4Transform(vLook, RotationMatrix));

}

void CTransform::LookAt(_fvector vAt)
{
	_vector		vLook = vAt - Get_State(STATE::POSITION);
	_vector		vRight = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vLook);
	_vector		vUp = XMVector3Cross(vLook, vRight);

	_float3		vScaled = Get_Scaled();

	Set_State(STATE::RIGHT, XMVector3Normalize(vRight) * vScaled.x);
	Set_State(STATE::UP, XMVector3Normalize(vUp) * vScaled.y);
	Set_State(STATE::LOOK, XMVector3Normalize(vLook) * vScaled.z);
}

void CTransform::Chase(_fvector vTargetPos, _float fTimeDelta, _float fLimit)
{
	_vector		vPosition = Get_State(STATE::POSITION);
	_vector		vMoveDir = vTargetPos - vPosition;

	_float		fDistance = XMVectorGetX(XMVector3Length(vMoveDir));

	if(fDistance >= fLimit)
		vPosition += XMVector3Normalize(vMoveDir) * m_fSpeedPerSec * fTimeDelta;

	Set_State(STATE::POSITION, vPosition);
}

CTransform* CTransform::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CTransform* pInstance = new CTransform(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX(TEXT("Failed to Created : CTransform"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CTransform::Free()
{
	__super::Free();
}
