// CCamera_Player.cpp
#include "Camera_Player.h"
#include "GameInstance.h"
#include "PlayerManager.h"

USING(Client)

CCamera_Player::CCamera_Player(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CCamera{ pDevice, pContext } {
}

CCamera_Player::CCamera_Player(const CCamera_Player& Prototype)
    : CCamera{ Prototype } {
}

HRESULT CCamera_Player::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CCamera_Player::Initialize(void* pArg)
{
    CAMERA_FREE_DESC* pDesc = static_cast<CAMERA_FREE_DESC*>(pArg);
    m_MouseSensor = pDesc ? pDesc->fMouseSensor : 0.01f;
    m_Distance = pDesc ? pDesc->fInitDistance : 5.f;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_DefaultDist = m_Distance;
    m_DefaultFov = m_fFovy;

    return S_OK;
}

void CCamera_Player::Priority_Update(_float /*fTimeDelta*/)
{

}

void CCamera_Player::Update(_float fTimeDelta)
{
    UpdateInput(fTimeDelta);
    ComputeCamera(fTimeDelta);
}

void CCamera_Player::Late_Update(_float /*fTimeDelta*/)
{
    __super::Update_PipeLines(); 
}

HRESULT CCamera_Player::Render()
{
    return S_OK;
}

void CCamera_Player::UpdateInput(_float dt)
{

    const _long dx = m_pGameInstance->Get_DIMouseMove(MOUSEMOVESTATE::X);
    const _long dy = m_pGameInstance->Get_DIMouseMove(MOUSEMOVESTATE::Y);

    if (dx) m_Yaw -= dx * m_MouseSensor;
    if (dy) m_Pitch -= dy * m_MouseSensor;
    m_Pitch = clamp_compat(m_Pitch, m_PitchMin, m_PitchMax);

    // 좌/우 클릭 줌
    const bool aiming =
        m_pGameInstance->MousePressing(MOUSEKEYSTATE::LB) ||
        m_pGameInstance->MousePressing(MOUSEKEYSTATE::RB);

    // 거리 보간
    const float targetDist = aiming ? m_AimDist : m_DefaultDist;
    const float kZoom = 1.f - expf(-dt * m_ZoomLerpSpeed);
    m_Distance += (targetDist - m_Distance) * kZoom;
    m_Distance = clamp_compat(m_Distance, m_MinDist, m_MaxDist);

    const float targetFov = aiming ? m_AimFov : m_DefaultFov;
    const float kFov = 1.f - expf(-dt * m_FovLerpSpeed);
    m_fFovy += (targetFov - m_fFovy) * kFov;
    m_fFovy = clamp_compat(m_fFovy, XMConvertToRadians(10.f), XMConvertToRadians(120.f));
}

void CCamera_Player::ComputeCamera(_float dt)
{
    auto* PM = CPlayerManager::GetInstance();
    if (!PM || !PM->GetActiveRaw()) return;   // was: PM->GetActive()

    // 플레이어 월드 축/위치는 매니저에서 바로 얻기
    const _vector P = PM->GetPos();
    const _vector F = PM->GetForward(true);   // 필요 시 Y평탄화
    const _vector R = PM->GetRight();
    const _vector U = PM->GetUp();

    const _vector target =
        P + XMVectorScale(U, m_TargetHeight)
        + XMVectorScale(R, m_ShoulderRight);

    const _matrix rotYaw = XMMatrixRotationAxis(U, m_Yaw);
    const _vector yawedDir = XMVector3TransformNormal(XMVectorNegate(F), rotYaw);
    const _vector yawedRight = XMVector3TransformNormal(R, rotYaw);

    const _matrix rotPitch = XMMatrixRotationAxis(yawedRight, m_Pitch);
    _vector dir = XMVector3TransformNormal(yawedDir, rotPitch);
    dir = XMVector3Normalize(dir);

    const _vector desiredPos = target + XMVectorScale(dir, m_Distance);

    // 위치 보간
    _vector newPos;
    if (m_FirstTick) {
        newPos = desiredPos;
        m_FirstTick = false;
    }
    else {
        const float a = 1.f - expf(-dt / max(0.0001f, m_PosSmoothTime));
        newPos = XMVectorLerp(m_PrevPos, desiredPos, a);
    }
    m_PrevPos = newPos;

    const _vector lookTarget = target + XMVectorScale(R, m_LookRightBias);

    _vector z = XMVector3Normalize(lookTarget - newPos);        // LOOK
    _vector x = XMVector3Normalize(XMVector3Cross(U, z));       // RIGHT
    if (fabsf(XMVectorGetX(XMVector3Dot(z, U))) > 0.995f) {
        const _vector worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
        x = XMVector3Normalize(XMVector3Cross(worldUp, z));
    }
    _vector y = XMVector3Normalize(XMVector3Cross(z, x));       // UP

    m_pTransformCom->Set_State(Engine::STATE::RIGHT, x);
    m_pTransformCom->Set_State(Engine::STATE::UP, y);
    m_pTransformCom->Set_State(Engine::STATE::LOOK, z);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, newPos);
}

CCamera_Player* CCamera_Player::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CCamera_Player* pInstance = new CCamera_Player(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CCamera_Player"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CCamera_Player::Clone(void* pArg)
{
    CCamera_Player* pInstance = new CCamera_Player(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CCamera_Player"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CCamera_Player::Free()
{
    __super::Free();
}
