#pragma once

#include "Base.h"
#include "BinType.h"
/* 특정 동작하나의 정보 전체(동작의 시작에서부터 끝까지의 대한 정보를 가진다.)를 관장하낟. */
/* 시작부터 끝까지의 시간에 따른 뼈들의 상태행렬을 저장하고 있는다 .*/
NS_BEGIN(Engine)
struct TRS;

class CAnimation final : public CBase
{
private:
	CAnimation();
	CAnimation(const CAnimation& Prototype);
	virtual ~CAnimation() = default;

public:
	HRESULT Initialize(const aiAnimation* pAIAnimation, const vector<class CBone*>& Bones);
	HRESULT Initialize(ifstream& ifs, const AnimInfoBin& animBin, const std::vector<CBone*>& Bones);
	void Update_TransformationMatrices(const vector<class CBone*>& Bones, _bool isLoop, _bool* pFinished, _float fTimeDelta);
	void ResetTimeToZero()
	{
		m_fCurrentTrackPosition = 0.f;
		std::fill(m_CurrentKeyFrameIndices.begin(), m_CurrentKeyFrameIndices.end(), 0u);
	}

public:
	void  Advance_Time(_bool isLoop, _bool* pFinished, _float fTimeDelta);
	void  EvaluatePose(const std::vector<class CBone*>& Bones, vector<TRS>& outPose, vector<uint8_t>& outHas);
	_float Get_TicksPerSec() const { return (m_fTickPerSecond > 0.f) ? m_fTickPerSecond : 30.f; }


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
	static CAnimation* Create(ifstream& ifs,const AnimInfoBin& animBin, const std::vector<CBone*>& Bones);
	CAnimation* Clone();
	virtual void Free() override;
};

NS_END