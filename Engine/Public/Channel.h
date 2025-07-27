#pragma once

#include "Base.h"

/* 시간에 따른 뼈의 상태행렬들을 보관한다. */

NS_BEGIN(Engine)

class CChannel final : public CBase
{
private:
	CChannel();
	virtual ~CChannel() = default;

public:
	HRESULT Initialize(const aiNodeAnim* pAIChannel, const vector<class CBone*>& Bones);
	void Update_TransformationMatrix(const vector<class CBone*>& Bones, _float fCurrentTrackPosition, _uint* pCurrentKeyFrameIndex);
private:
	_char							m_szName[MAX_PATH] = { };

	_uint							m_iBoneIndex = {};


	_uint							m_iNumKeyFrames = {};
	vector<KEYFRAME>				m_KeyFrames;

public:
	static CChannel* Create(const aiNodeAnim* pAIChannel, const vector<class CBone*>& Bones);
	virtual void Free() override;
};

NS_END

