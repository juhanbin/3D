#include "Channel.h"
#include "Bone.h"
#include <fstream>
CChannel::CChannel()
{
}

HRESULT CChannel::Initialize(const aiNodeAnim* pAIChannel, const vector<class CBone*>& Bones)
{
    auto	iter = find_if(Bones.begin(), Bones.end(), [&](CBone* pBone)->_bool
        {
            if (true == pBone->Compare_Name(pAIChannel->mNodeName.data))
                return true;

            m_iBoneIndex++;

            return false;
        });

    m_iNumKeyFrames = max(pAIChannel->mNumScalingKeys, pAIChannel->mNumRotationKeys);
    m_iNumKeyFrames = max(m_iNumKeyFrames, pAIChannel->mNumPositionKeys);

    _float3     vScale{};
    _float4     vRotation{};
    _float3     vTranslation{};

    for (size_t i = 0; i < m_iNumKeyFrames; i++)
    {
        KEYFRAME            KeyFrame{};

        if (i < pAIChannel->mNumScalingKeys)
        {
            memcpy(&vScale, &pAIChannel->mScalingKeys[i].mValue, sizeof(_float3));
            KeyFrame.fTrackPosition = pAIChannel->mScalingKeys[i].mTime;
        }

        if (i < pAIChannel->mNumRotationKeys)
        {
            vRotation.x = pAIChannel->mRotationKeys[i].mValue.x;
            vRotation.y = pAIChannel->mRotationKeys[i].mValue.y;
            vRotation.z = pAIChannel->mRotationKeys[i].mValue.z;
            vRotation.w = pAIChannel->mRotationKeys[i].mValue.w;

            KeyFrame.fTrackPosition = pAIChannel->mRotationKeys[i].mTime;
        }

        if (i < pAIChannel->mNumPositionKeys)
        {
            memcpy(&vTranslation, &pAIChannel->mPositionKeys[i].mValue, sizeof(_float3));
            KeyFrame.fTrackPosition = pAIChannel->mPositionKeys[i].mTime;
        }

        KeyFrame.vScale = vScale;
        KeyFrame.vRotation = vRotation;
        KeyFrame.vTranslation = vTranslation;

        m_KeyFrames.push_back(KeyFrame);
    }


    return S_OK;
}

HRESULT CChannel::Initialize(const ChannelInfoBin& channelBin, const std::vector<KEYFRAME>& keyframes, const std::vector<CBone*>& Bones)
{
    strcpy_s(m_szName, channelBin.boneName);
    m_iNumKeyFrames = channelBin.keyframeCount;
    m_KeyFrames = keyframes;

    // 본 인덱스 찾기
    m_iBoneIndex = 0;
    auto iter = std::find_if(Bones.begin(), Bones.end(), [&](CBone* pBone) {
        return pBone->Compare_Name(m_szName);
        });
    if (iter == Bones.end())
        return E_FAIL;
    m_iBoneIndex = static_cast<uint32_t>(std::distance(Bones.begin(), iter));
    return S_OK;
}

void CChannel::Update_TransformationMatrix(const vector<class CBone*>& Bones, _float fCurrentTrackPosition, _uint* pCurrentKeyFrameIndex)
{
    if (fCurrentTrackPosition == 0.f)
        *pCurrentKeyFrameIndex = 0;

    /* 선택된 애니메이션이 이용하고 있는 이 뼈(Channel)의 현재 재생된 위치(fCurrrentTrackPosition)에 맞는 상태행렬을 만들어 준다. */
    _vector         vScale, vRotation, vTranslation;

    /* 마지막 키프레임상태를 취하낟. */
    KEYFRAME        LastKeyFrame = m_KeyFrames.back();

    if (fCurrentTrackPosition >= LastKeyFrame.fTrackPosition)
    {
        vScale = XMLoadFloat3(&LastKeyFrame.vScale);
        vRotation = XMLoadFloat4(&LastKeyFrame.vRotation);
        vTranslation = XMVectorSetW(XMLoadFloat3(&LastKeyFrame.vTranslation), 1.f);
    }

    /* 양쪽 키프레임사이에서의 중간상태를 보간하여 만든다. */
    else
    {
        while (fCurrentTrackPosition >= m_KeyFrames[*pCurrentKeyFrameIndex + 1].fTrackPosition)
            ++*pCurrentKeyFrameIndex;

        _vector    vSourScale, vDestScale;
        _vector    vSourRotation, vDestRotation;
        _vector    vSourTranslation, vDestTranslation;

        vSourScale = XMLoadFloat3(&m_KeyFrames[*pCurrentKeyFrameIndex].vScale);
        vSourRotation = XMLoadFloat4(&m_KeyFrames[*pCurrentKeyFrameIndex].vRotation);
        vSourTranslation = XMVectorSetW(XMLoadFloat3(&m_KeyFrames[*pCurrentKeyFrameIndex].vTranslation), 1.f);

        vDestScale = XMLoadFloat3(&m_KeyFrames[*pCurrentKeyFrameIndex + 1].vScale);
        vDestRotation = XMLoadFloat4(&m_KeyFrames[*pCurrentKeyFrameIndex + 1].vRotation);
        vDestTranslation = XMVectorSetW(XMLoadFloat3(&m_KeyFrames[*pCurrentKeyFrameIndex + 1].vTranslation), 1.f);

        _float      fRatio = (fCurrentTrackPosition - m_KeyFrames[*pCurrentKeyFrameIndex].fTrackPosition) / (m_KeyFrames[*pCurrentKeyFrameIndex + 1].fTrackPosition - m_KeyFrames[*pCurrentKeyFrameIndex].fTrackPosition);

        vScale = XMVectorLerp(vSourScale, vDestScale, fRatio);
        vRotation = XMQuaternionSlerp(vSourRotation, vDestRotation, fRatio);
        vTranslation = XMVectorSetW(XMVectorLerp(vSourTranslation, vDestTranslation, fRatio), 1.f);


    }

    /*_matrix         TransformationMatrix = XMMatrixScaling() * XMMatrixRotationQuaternion() * XMMatrixTranslation();*/
    _matrix         TransformationMatrix = XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);

    Bones[m_iBoneIndex]->Set_TransformationMatrix(TransformationMatrix);
}

void CChannel::SampleTRS(_float fCurrentTrackPosition, _uint& ioKeyIndex, TRS& out) const
{
    if (fCurrentTrackPosition == 0.f) ioKeyIndex = 0;

    const KEYFRAME& last = m_KeyFrames.back();
    if (fCurrentTrackPosition >= last.fTrackPosition)
    {
        out.vScale = last.vScale;
        out.vRotation = last.vRotation;
        out.vTranslation = last.vTranslation;
        return;
    }

    while (fCurrentTrackPosition >= m_KeyFrames[ioKeyIndex + 1].fTrackPosition)
        ++ioKeyIndex;

    const KEYFRAME& k0 = m_KeyFrames[ioKeyIndex];
    const KEYFRAME& k1 = m_KeyFrames[ioKeyIndex + 1];
    const _float a = (fCurrentTrackPosition - k0.fTrackPosition)
        / (k1.fTrackPosition - k0.fTrackPosition);

    // scale
    _vector s0 = XMLoadFloat3(&k0.vScale);
    _vector s1 = XMLoadFloat3(&k1.vScale);
    XMStoreFloat3(&out.vScale, XMVectorLerp(s0, s1, a));

    // rotation (quat)
    _vector q0 = XMLoadFloat4(&k0.vRotation);
    _vector q1 = XMLoadFloat4(&k1.vRotation);
    // (선택) 최단호 회전 보장: if (XMVectorGetX(XMVector4Dot(q0, q1)) < 0) q1 = XMVectorNegate(q1);
    XMStoreFloat4(&out.vRotation, XMQuaternionSlerp(q0, q1, a));

    // translation
    _vector p0 = XMLoadFloat3(&k0.vTranslation);
    _vector p1 = XMLoadFloat3(&k1.vTranslation);
    XMStoreFloat3(&out.vTranslation, XMVectorLerp(p0, p1, a));
}


CChannel* CChannel::Create(const aiNodeAnim* pAIChannel, const vector<class CBone*>& Bones)
{
    CChannel* pInstance = new CChannel();

    if (FAILED(pInstance->Initialize(pAIChannel, Bones)))
    {
        MSG_BOX(TEXT("Failed to Created : CChannel"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CChannel* CChannel::Create(const ChannelInfoBin& channelBin, const std::vector<KEYFRAME>& keyframes, const std::vector<CBone*>& Bones)
{
    CChannel* pInstance = new CChannel();
    if (FAILED(pInstance->Initialize(channelBin, keyframes, Bones)))
    {
        MSG_BOX(TEXT("Failed to Created : CChannel(bin)"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CChannel::Free()
{
}

