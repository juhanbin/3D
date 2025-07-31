#include "Channel.h"
#include "Bone.h"

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

    //char buf[256];
    ////sprintf_s(buf, "[CChannel] Channel 생성: boneName=%s, 매핑된 BoneIndex=%d (Bones.size()=%d)\n", pAIChannel->mNodeName.data, m_iBoneIndex, (int)Bones.size());
    ////OutputDebugStringA(buf);

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

HRESULT CChannel::Initialize(const ChannelInfo& chInfo, const std::vector<KeyFrame >& keyframes, const std::vector<CBone*>& Bones)
{
    m_iBoneIndex = -1;
    for (size_t i = 0; i < Bones.size(); ++i)
    {
        if (Bones[i]->Compare_Name(chInfo.boneName)) {
            m_iBoneIndex = static_cast<int>(i);
            break;
        }
    }

    //char dbg[256];
    //sprintf_s(dbg, "[BIN] Channel BoneName=%s -> BoneIndex=%d KeyFrames=%u\n",
    //    chInfo.boneName, m_iBoneIndex, chInfo.keyframeCount);
    //OutputDebugStringA(dbg);

    m_iNumKeyFrames = chInfo.keyframeCount;

    for (auto& kf : keyframes)
    {
        KEYFRAME k;
        k.vScale = _float3(kf.scale[0], kf.scale[1], kf.scale[2]);
        k.vRotation = _float4(kf.rotation[0], kf.rotation[1], kf.rotation[2], kf.rotation[3]);
        k.vTranslation = _float3(kf.translation[0], kf.translation[1], kf.translation[2]);
        k.fTrackPosition = static_cast<float>(kf.time);
        m_KeyFrames.push_back(k);
    }

    return S_OK;
}

void CChannel::Update_TransformationMatrix(const vector<class CBone*>& Bones, _float fCurrentTrackPosition, _uint* pCurrentKeyFrameIndex)
{
    if (m_iBoneIndex < 0 || m_iBoneIndex >= Bones.size()) {
        //OutputDebugStringA("[ERR] Invalid BoneIndex!\n");
        return;
    }

    //char dbg[128];
    ////sprintf_s(dbg, "[AnimUpdate] BoneIndex=%d KeyFrames=%zu CurrentTime=%.3f\n", m_iBoneIndex, m_KeyFrames.size(), fCurrentTrackPosition);
    //OutputDebugStringA(dbg);

    if (fCurrentTrackPosition == 0.f)
        *pCurrentKeyFrameIndex = 0;

    _vector vScale, vRotation, vTranslation;
    KEYFRAME LastKeyFrame = m_KeyFrames.back();

    if (fCurrentTrackPosition >= LastKeyFrame.fTrackPosition)
    {
        vScale = XMLoadFloat3(&LastKeyFrame.vScale);
        vRotation = XMLoadFloat4(&LastKeyFrame.vRotation);
        vTranslation = XMVectorSetW(XMLoadFloat3(&LastKeyFrame.vTranslation), 1.f);
    }
    else
    {
        while (*pCurrentKeyFrameIndex + 1 < m_KeyFrames.size() &&
            fCurrentTrackPosition >= m_KeyFrames[*pCurrentKeyFrameIndex + 1].fTrackPosition)
        {
            ++*pCurrentKeyFrameIndex;
        }

        _vector vSourScale = XMLoadFloat3(&m_KeyFrames[*pCurrentKeyFrameIndex].vScale);
        _vector vSourRotation = XMLoadFloat4(&m_KeyFrames[*pCurrentKeyFrameIndex].vRotation);
        _vector vSourTranslation = XMVectorSetW(XMLoadFloat3(&m_KeyFrames[*pCurrentKeyFrameIndex].vTranslation), 1.f);

        _vector vDestScale = XMLoadFloat3(&m_KeyFrames[*pCurrentKeyFrameIndex + 1].vScale);
        _vector vDestRotation = XMLoadFloat4(&m_KeyFrames[*pCurrentKeyFrameIndex + 1].vRotation);
        _vector vDestTranslation = XMVectorSetW(XMLoadFloat3(&m_KeyFrames[*pCurrentKeyFrameIndex + 1].vTranslation), 1.f);

        _float fRatio = (fCurrentTrackPosition - m_KeyFrames[*pCurrentKeyFrameIndex].fTrackPosition) /
            (m_KeyFrames[*pCurrentKeyFrameIndex + 1].fTrackPosition - m_KeyFrames[*pCurrentKeyFrameIndex].fTrackPosition);

        vScale = XMVectorLerp(vSourScale, vDestScale, fRatio);
        vRotation = XMQuaternionSlerp(vSourRotation, vDestRotation, fRatio);
        vTranslation = XMVectorSetW(XMVectorLerp(vSourTranslation, vDestTranslation, fRatio), 1.f);
    }

    _matrix TransformationMatrix = XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);
    Bones[m_iBoneIndex]->Set_TransformationMatrix(TransformationMatrix);
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

CChannel* CChannel::Create(const ChannelInfo& chInfo, const std::vector<KeyFrame >& keyframes, const std::vector<CBone*>& Bones)
{
    CChannel* pInstance = new CChannel();
    if (FAILED(pInstance->Initialize(chInfo, keyframes, Bones)))
    {
        MSG_BOX(TEXT("Failed to Created : CChannel (BIN)"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CChannel::Free()
{
}
