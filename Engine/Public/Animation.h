#pragma once

#include "Base.h"

/* 특정 동작하나의 정보 전체(동작의 시작에서부터 끝까지의 대한 정보를 가진다.)를 관장하낟. */
/* 시작부터 끝까지의 시간에 따른 뼈들의 상태행렬을 저장하고 있는다 .*/
NS_BEGIN(Engine)

class CAnimation final : public CBase
{
private:
	CAnimation();
	CAnimation(const CAnimation& Prototype);
	virtual ~CAnimation() = default;

public:
	HRESULT Initialize(const aiAnimation* pAIAnimation, const vector<class CBone*>& Bones);
	void Update_TransformationMatrices(const vector<class CBone*>& Bones, _bool isLoop, _bool* pFinished, _float fTimeDelta);

private:

	/* 애니메이션의 전체 재생 길이 */
	_float						m_fDuration = {};

	/* 초당 이동해야할 거리 : 재생속도 */
	_float						m_fTickPerSecond = {};

	_float						m_fCurrentTrackPosition = {};

	/* 이 동작을 위한 뼈들!!! 의 상태*/
	/* CChannel == 뼈(시간에 따른 뼈의 상태행렬) */
	_uint						m_iNumChannels = {};
	vector<class CChannel*>		m_Channels;
	vector<_uint>				m_CurrentKeyFrameIndices;



public:
	static CAnimation* Create(const aiAnimation* pAIAnimation, const vector<class CBone*>& Bones);
	CAnimation* Clone();
	virtual void Free() override;
};

NS_END