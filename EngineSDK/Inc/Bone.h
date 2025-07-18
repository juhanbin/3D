#pragma once

#include "Base.h"

/*
aiNodeAnim
aiNode : 뼈들의 관계를 표현하기위한 데이터
aiBone
*/

/*
TransformationMatrix : 이 뼈 만의 자체적인 변환행렬(원점기준(x), 부모기준)
CombindTransformationMatrix : m_TransformationMatrix * Parent`s CombindTransformationMatrix
*/

NS_BEGIN(Engine)

class CBone final : public CBase
{
private:
	CBone();
	virtual ~CBone() = default;

public:
	HRESULT Initialize(const aiNode* pAINode, _int iParentBoneIndex);
	void Update_CombinedTransformatrix(const vector<CBone*>& Bones);

private:
	_char				m_szName[MAX_PATH] = {};
	_float4x4			m_TransformationMatrix = {};
	_float4x4			m_CombinedTransformationMatrix = {};

	_int				m_iParentBoneIndex = { -1 };

public:
	static CBone* Create(const aiNode* pAINode, _int iParentBoneIndex);
	virtual void Free() override;
};

NS_END