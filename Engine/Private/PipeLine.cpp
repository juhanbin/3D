#include "PipeLine.h"

CPipeLine::CPipeLine()
{
}

_matrix CPipeLine::Get_Transform_Matrix(D3DTS eTransformState) const
{
	return XMLoadFloat4x4(&m_TransformMatrices[ENUM_CLASS(eTransformState)]);
}

const _float4x4* CPipeLine::Get_Transform_Float4x4(D3DTS eTransformState) const
{
	return &m_TransformMatrices[ENUM_CLASS(eTransformState)];
}

_matrix CPipeLine::Get_Transform_Matrix_Inverse(D3DTS eTransformState) const
{
	return XMLoadFloat4x4(&m_TransformMatrixInverse[ENUM_CLASS(eTransformState)]);
}

const _float4x4* CPipeLine::Get_Transform_Float4x4_Inverse(D3DTS eTransformState) const
{
	return &m_TransformMatrixInverse[ENUM_CLASS(eTransformState)];
}

const _float4* CPipeLine::Get_CamPosition() const
{
	return &m_vCamPosition;
}

void CPipeLine::Set_Transform(D3DTS eTransformState, _fmatrix Matrix)
{
	XMStoreFloat4x4(&m_TransformMatrices[ENUM_CLASS(eTransformState)], Matrix);
}

void CPipeLine::Set_Transform(D3DTS eTransformState, const _float4x4& Matrix)
{
	m_TransformMatrices[ENUM_CLASS(eTransformState)] = Matrix;
}

HRESULT CPipeLine::Initialize()
{
	for (size_t i = 0; i < ENUM_CLASS(D3DTS::END); i++)
	{
		XMStoreFloat4x4(&m_TransformMatrices[i], XMMatrixIdentity());
		XMStoreFloat4x4(&m_TransformMatrixInverse[i], XMMatrixIdentity());
	}

	return S_OK;
}

void CPipeLine::Update()
{
	for (size_t i = 0; i < ENUM_CLASS(D3DTS::END); i++)
		XMStoreFloat4x4(&m_TransformMatrixInverse[i], XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_TransformMatrices[i])));

	memcpy(&m_vCamPosition, &m_TransformMatrixInverse[ENUM_CLASS(D3DTS::VIEW)].m[3], sizeof(_float4));
}

CPipeLine* CPipeLine::Create()
{
	CPipeLine* pInstance = new CPipeLine();

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX(TEXT("Failed to Created : CPipeLine"));
		Safe_Release(pInstance);
	}

	return pInstance;
}


void CPipeLine::Free()
{
	__super::Free();

}
