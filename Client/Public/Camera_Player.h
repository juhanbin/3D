// CCamera_Player.h
#pragma once

#include "Client_Defines.h"
#include "Camera.h"

NS_BEGIN(Client)

class CCamera_Player final : public CCamera
{
public:
    struct CAMERA_FREE_DESC : public CCamera::CAMERA_DESC
    {
        _float  fMouseSensor = 0.01f;  // 마우스 감도(라디안/픽셀)
        _float  fInitDistance = 5.f;   // 시작 거리
    };

private:
    CCamera_Player(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CCamera_Player(const CCamera_Player& Prototype);
    virtual ~CCamera_Player() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void   Priority_Update(_float fTimeDelta) override;
    virtual void   Update(_float fTimeDelta) override;
    virtual void   Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

public:
    // 런타임 조정용
    void SetDistance(float d) { m_Distance = clamp_compat(d, m_MinDist, m_MaxDist); }
    void SetShoulderRight(float r) { m_ShoulderRight = r; }
    void SetTargetHeight(float h) { m_TargetHeight = h; }
    void SetLookRightBias(float r) { m_LookRightBias = r; }

    template<typename T>
    static inline T clamp_compat(T v, T lo, T hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

private:
    void UpdateInput(_float dt);
    void ComputeCamera(_float dt);

private:
    // 입력/각도/거리
    _float  m_MouseSensor = 0.01f;
    _float  m_Yaw = 0.f;                           // 라디안
    _float  m_Pitch = XMConvertToRadians(15.f);      // 라디안
    _float  m_PitchMin = XMConvertToRadians(-60.f);
    _float  m_PitchMax = XMConvertToRadians(75.f);

    _float  m_Distance = 5.f;
    _float  m_MinDist = 3.0f;
    _float  m_MaxDist = 7.5f;

    // 줌 파라미터
    float m_DefaultDist = 0.1f;                     // 평상시 거리
    float m_AimDist = -1.f;                    // 조준(줌) 거리
    float m_ZoomLerpSpeed = 10.f;                    // 거리 보간 속도

    // FOV 보간(선택)
    float m_DefaultFov = XMConvertToRadians(60.f);
    float m_AimFov = XMConvertToRadians(45.f);
    float m_FovLerpSpeed = 8.f;

    // 숄더뷰 오프셋(플레이어 로컬 기준)
    _float  m_ShoulderRight = 0.5f;  // +X
    _float  m_TargetHeight = 1.5f;  // +Y
    _float  m_LookRightBias = 0.2f;  // 살짝 오른쪽을 보게

    // 위치 부드러움
    _float  m_PosSmoothTime = 0.12f;
    _vector m_PrevPos = XMVectorZero();
    _bool   m_FirstTick = true;

public:
    static CCamera_Player* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void           Free() override;
};

NS_END
