#include "Animation.h"
#include "Channel.h"

CAnimation::CAnimation()
{
    /*   XMMatrixDecompose(스케일, 로테이션, 이동, 행렬);*/
}

CAnimation::CAnimation(const CAnimation& Prototype)
    : m_fDuration(Prototype.m_fDuration)
    , m_fTickPerSecond(Prototype.m_fTickPerSecond)
    , m_fCurrentTrackPosition(Prototype.m_fCurrentTrackPosition)
    , m_iNumChannels(Prototype.m_iNumChannels)
    , m_Channels(Prototype.m_Channels)
    , m_CurrentKeyFrameIndices(Prototype.m_CurrentKeyFrameIndices)
{
    for (auto& pChannel : m_Channels)
        Safe_AddRef(pChannel);
}

HRESULT CAnimation::Initialize(const aiAnimation* pAIAnimation, const vector<class CBone*>& Bones)
{
    m_fDuration = pAIAnimation->mDuration;
    m_fTickPerSecond = pAIAnimation->mTicksPerSecond;
    m_iNumChannels = pAIAnimation->mNumChannels;
    m_CurrentKeyFrameIndices.resize(m_iNumChannels);

    //char buf[256];
    ////sprintf_s(buf, "[CAnimation] Animation 생성: name=%s, Duration=%.3f, TicksPerSec=%.3f, ChannelCount=%d\n",
    ////    pAIAnimation->mName.data, m_fDuration, m_fTickPerSecond, m_iNumChannels);
    ////OutputDebugStringA(buf);

    for (size_t i = 0; i < m_iNumChannels; i++)
    {
        CChannel* pChannel = CChannel::Create(pAIAnimation->mChannels[i], Bones);
        if (nullptr == pChannel)
            return E_FAIL;

        m_Channels.push_back(pChannel);
    }

    return S_OK;
}

HRESULT CAnimation::Initialize(const AnimInfo& animInfo, const std::vector<ChannelInfo >& channels, const std::vector<KeyFrame >& keyframes, const std::vector<class CBone*>& Bones)
{
    m_fDuration = static_cast<float>(animInfo.duration);
    m_fTickPerSecond = static_cast<float>(animInfo.ticksPerSecond);
    m_iNumChannels = animInfo.channelCount;
    m_CurrentKeyFrameIndices.resize(m_iNumChannels);

    size_t keyframeOffset = 0;
    for (size_t i = 0; i < m_iNumChannels; ++i)
    {
        const ChannelInfo& chInfo = channels[i];
        std::vector<KeyFrame > chKeyframes(
            keyframes.begin() + keyframeOffset,
            keyframes.begin() + keyframeOffset + chInfo.keyframeCount
        );
        keyframeOffset += chInfo.keyframeCount;

        CChannel* pChannel = CChannel::Create(chInfo, chKeyframes, Bones);
        if (!pChannel)
            return E_FAIL;
        m_Channels.push_back(pChannel);
    }
    return S_OK;
}

void CAnimation::Update_TransformationMatrices(const vector<class CBone*>& Bones, _bool isLoop, _bool* pFinished, _float fTimeDelta)
{
    m_fCurrentTrackPosition += m_fTickPerSecond * fTimeDelta;

    if (m_fCurrentTrackPosition >= m_fDuration)
    {
        if (false == isLoop)
        {
            *pFinished = true;
            m_fCurrentTrackPosition = m_fDuration;
            return;
        }
        else
            m_fCurrentTrackPosition = 0.f;
    }

    for (_uint i = 0; i < m_iNumChannels; ++i)
    {
        m_Channels[i]->Update_TransformationMatrix(Bones, m_fCurrentTrackPosition, &m_CurrentKeyFrameIndices[i]);
    }
}

CAnimation* CAnimation::Create(const aiAnimation* pAIAnimation, const vector<class CBone*>& Bones)
{
    CAnimation* pInstance = new CAnimation();

    if (FAILED(pInstance->Initialize(pAIAnimation, Bones)))
    {
        MSG_BOX(TEXT("Failed to Created : CAnimation"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CAnimation* CAnimation::Create(const AnimInfo& animInfo, const std::vector<ChannelInfo >& channels, const std::vector<KeyFrame >& keyframes, const std::vector<class CBone*>& Bones)
{
    CAnimation* pInstance = new CAnimation();
    if (FAILED(pInstance->Initialize(animInfo, channels, keyframes, Bones)))
    {
        MSG_BOX(TEXT("Failed to Created : CAnimation (BIN)"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

CAnimation* CAnimation::Clone()
{
    return new CAnimation(*this);
}

void CAnimation::Free()
{
    __super::Free();

    for (auto& pChannel : m_Channels)
        Safe_Release(pChannel);

    m_Channels.clear();
}
