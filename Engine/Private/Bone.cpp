#include "Bone.h"

CBone::CBone()
{

}

HRESULT CBone::Initialize(const aiNode* pAINode, _int iParentBoneIndex)
{
	strcpy_s(m_szName, pAINode->mName.data);

	memcpy(&m_TransformationMatrix, &pAINode->mTransformation, sizeof(_float4x4));

	XMStoreFloat4x4(&m_TransformationMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_TransformationMatrix)));

	XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMMatrixIdentity());

	m_iParentBoneIndex = iParentBoneIndex;

	return S_OK;
}

HRESULT CBone::Initialize(const BoneInfoBin& bin, _int iParentBoneIndex)
{
	strcpy_s(m_szName, bin.name);
	// BIN에서 오프셋 행렬도 읽어야 한다!
	memcpy(&m_OffsetMatrix, bin.offset, sizeof(_float4x4));

	memcpy(&m_TransformationMatrix, bin.transform, sizeof(_float4x4));
	XMStoreFloat4x4(&m_TransformationMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_TransformationMatrix)));
	XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMMatrixIdentity());
	m_iParentBoneIndex = bin.parentIdx; // BIN에 저장된 값을 그대로 사용

	return S_OK;
}

void CBone::Update_CombinedTransformationMatrix(const _float4x4& PreTransformMatrix, const vector<CBone*>& Bones)
{
	if (-1 == m_iParentBoneIndex)
	{
		XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMLoadFloat4x4(&PreTransformMatrix) * XMLoadFloat4x4(&m_TransformationMatrix));
		return;
	}

	XMStoreFloat4x4(&m_CombinedTransformationMatrix,
		XMLoadFloat4x4(&m_TransformationMatrix) * XMLoadFloat4x4(&Bones[m_iParentBoneIndex]->m_CombinedTransformationMatrix));


}

CBone* CBone::Create(const aiNode* pAINode, _int iParentBoneIndex)
{
	CBone* pInstance = new CBone();

	if (FAILED(pInstance->Initialize(pAINode, iParentBoneIndex)))
	{
		MSG_BOX(TEXT("Failed to Created : CBone"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBone* CBone::Create(const BoneInfoBin& bin, _int iParentBoneIndex)
{
	CBone* pInstance = new CBone();

	if (FAILED(pInstance->Initialize(bin, iParentBoneIndex)))
	{
		MSG_BOX(TEXT("Failed to Created : CBone(bin)"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBone* CBone::Clone()
{
	return new CBone(*this);
}

void CBone::Free()
{
	__super::Free();


}
