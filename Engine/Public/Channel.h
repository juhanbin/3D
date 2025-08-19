#pragma once

#include "Base.h"
#include "BinType.h"
/* 시간에 따른 뼈의 상태행렬들을 보관한다. */

NS_BEGIN(Engine)


class CChannel final : public CBase
{
	
private:
	CChannel();
	virtual ~CChannel() = default;

public:
	HRESULT Initialize(const aiNodeAnim* pAIChannel, const vector<class CBone*>& Bones);
	HRESULT Initialize(const ChannelInfoBin& channelBin, const std::vector<KEYFRAME>& keyframes, const std::vector<CBone*>& Bones);
	void Update_TransformationMatrix(const vector<class CBone*>& Bones, _float fCurrentTrackPosition, _uint* pCurrentKeyFrameIndex);

public:
	_uint Get_BoneIndex() const { return m_iBoneIndex; }
	void  SampleTRS(_float fCurrentTrackPosition, _uint& ioKeyIndex, TRS& out) const;

private:
	_char							m_szName[MAX_PATH] = { };

	_uint							m_iBoneIndex = {};


	_uint							m_iNumKeyFrames = {};
	vector<KEYFRAME>				m_KeyFrames;

public:
	static CChannel* Create(const aiNodeAnim* pAIChannel, const vector<class CBone*>& Bones);
	static CChannel* Create(const ChannelInfoBin& channelBin, const std::vector<KEYFRAME>& keyframes, const std::vector<CBone*>& Bones);
	virtual void Free() override;
};

NS_END

