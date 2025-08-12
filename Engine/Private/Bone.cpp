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

HRESULT CBone::Initialize(const BoneInfoBin& binBone)
{
	strcpy_s(m_szName, binBone.Name);

	m_iParentBoneIndex = binBone.ParentIndex;

	memcpy(&m_TransformationMatrix, binBone.Transform, sizeof(float) * 16);

	XMStoreFloat4x4(&m_TransformationMatrix,
		XMMatrixTranspose(XMLoadFloat4x4(&m_TransformationMatrix)));

	XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMMatrixIdentity());

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

CBone* CBone::Create(const BoneInfoBin& bin)
{
	CBone* pInstance = new CBone();

	if (FAILED(pInstance->Initialize(bin)))
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
