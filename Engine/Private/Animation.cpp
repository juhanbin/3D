#include "Animation.h"
#include "Channel.h"
#include <fstream>
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

    /*char buf[256];
    sprintf_s(buf, "[CAnimation] Animation 생성: name=%s, Duration=%.3f, TicksPerSec=%.3f, ChannelCount=%d\n",
        pAIAnimation->mName.data, m_fDuration, m_fTickPerSecond, m_iNumChannels);*/
    //OutputDebugStringA(buf);

    for (size_t i = 0; i < m_iNumChannels; i++)
    {
        CChannel* pChannel = CChannel::Create(pAIAnimation->mChannels[i], Bones);
        if (nullptr == pChannel)
            return E_FAIL;

        m_Channels.push_back(pChannel);
    }

    return S_OK;
}

HRESULT CAnimation::Initialize(std::ifstream& ifs, const AnimInfoBin& animBin, const std::vector<CBone*>& Bones)
{
    m_fDuration = (float)animBin.duration;
    m_fTickPerSecond = (float)animBin.ticksPerSecond;
    m_iNumChannels = animBin.channelCount;
    m_CurrentKeyFrameIndices.resize(m_iNumChannels);

   char buf[256];
    /*sprintf_s(buf, "  [CAnimation::Initialize] 이름='%s', 채널수=%d, duration=%.2f\n", animBin.name, animBin.channelCount, animBin.duration);
    OutputDebugStringA(buf);*/

   for (size_t i = 0; i < m_iNumChannels; ++i)
   {
       // 1. 채널정보 한개씩 읽기
       ChannelInfoBin channelBin{};
       ifs.read(reinterpret_cast<char*>(&channelBin), sizeof(ChannelInfoBin));
       if (!ifs) {
           OutputDebugStringA("채널 정보 읽기 실패!\n");
           return E_FAIL;
       }

      //sprintf_s(buf, "    [Anim->Channel] [%zu] boneName='%s' keyCount=%u\n", i, channelBin.boneName, channelBin.keyframeCount);
      //OutputDebugStringA(buf);

       // 2. 키프레임들 바로 읽기
       uint32_t keyCount = channelBin.keyframeCount;
       std::vector<KEYFRAME> keyframes(keyCount);
       if (keyCount > 0)
           ifs.read(reinterpret_cast<char*>(keyframes.data()), sizeof(KEYFRAME) * keyCount);

       if (channelBin.keyframeCount > 0) {
          // sprintf_s(buf, "      첫 keyframe pos=(%.2f,%.2f,%.2f) time=%.2f\n",
          //     keyframes[0].vTranslation.x, keyframes[0].vTranslation.y, keyframes[0].vTranslation.z, keyframes[0].fTrackPosition);
          // OutputDebugStringA(buf);

           if (!ifs) {
               /* sprintf_s(buf, "키프레임 읽기 실패! (i=%zu, keyCount=%u)\n", i, keyCount);
                OutputDebugStringA(buf);*/
               return E_FAIL;
           }

           // 3. 채널 생성
           CChannel* pChannel = CChannel::Create(channelBin, keyframes, Bones);
           if (nullptr == pChannel) {
               /* sprintf_s(buf, "  [Anim->Channel] Channel 생성 실패! (i=%zu, boneName=%s, keyframeCount=%u)\n", i, channelBin.boneName, channelBin.keyframeCount);
                OutputDebugStringA(buf);*/
               return E_FAIL;
           }
           m_Channels.push_back(pChannel);

           /*sprintf_s(buf, "    [Anim->Channel] 채널[%zu] bone='%s' keyCount=%u\n", i, channelBin.boneName, keyCount);
           OutputDebugStringA(buf);*/
       }
       
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

CAnimation* CAnimation::Create(ifstream& ifs, const AnimInfoBin& animBin, const std::vector<CBone*>& Bones)
{
    CAnimation* pInstance = new CAnimation();
    if (FAILED(pInstance->Initialize(ifs, animBin, Bones)))
    {
        MSG_BOX(TEXT("Failed to Created : CAnimation(bin)"));
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
