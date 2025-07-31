#pragma once

#include "Base.h"
#include "BinType.h"
/*
aiNodeAnim : 애니메이션을 위한 뼈의 상태행렬을 보관한다.
aiNode : 뼈들의 관계를 표현하기위한 데이터
aiBone : 이 뼈는 몇개의 정점에게 영향을 주며 그중 어떤 정점들에게 영향을 준다!! + 얼마나 영향을 준다.
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
	_matrix Get_CombinedTransformationMatrix() const {
		return XMLoadFloat4x4(&m_CombinedTransformationMatrix);
	}

	void Set_TransformationMatrix(_fmatrix Matrix) {
		XMStoreFloat4x4(&m_TransformationMatrix, Matrix);
	}
public:

	HRESULT Initialize(const aiNode* pAINode, _int iParentBoneIndex);
	HRESULT Initialize(const BoneInfo& bin, _int iParentBoneIndex);
	void Update_CombinedTransformationMatrix(const _float4x4& PreTransformMatrix, const vector<CBone*>& Bones);

	_bool Compare_Name(const _char* pName) {
		return !strcmp(pName, m_szName);
	}

private:
	_char				m_szName[MAX_PATH] = {};
	_float4x4			m_TransformationMatrix = {};
	_float4x4			m_CombinedTransformationMatrix = {};

	_int				m_iParentBoneIndex = { -1 };

public:
	static CBone* Create(const aiNode* pAINode, _int iParentBoneIndex);
	static CBone* Create(const BoneInfo& bin, _int iParentBoneIndex); // BIN 버전
	CBone* Clone();
	virtual void Free() override;
};

NS_END