#include "Boss_Controller.h"
#include "GameInstance.h"
#include "PlayerManager.h"
#include "Boss_Hand_L.h"
#include "Boss_Hand_R.h"
#include "Boss_Mask.h"

using namespace Client;
using namespace DirectX;

CBoss_Controller::CBoss_Controller(ID3D11Device* dev, ID3D11DeviceContext* ctx)
    : CGameObject(dev, ctx) {
}

CBoss_Controller::CBoss_Controller(const CBoss_Controller& rhs)
    : CGameObject(rhs) {
}

HRESULT CBoss_Controller::Initialize_Prototype() { return S_OK; }

HRESULT CBoss_Controller::Initialize(void* pArg)
{
    GAMEOBJECT_DESC d{}; d.fSpeedPerSec = 0; d.fRotationPerSec = 0;
    if (FAILED(__super::Initialize(&d))) return E_FAIL;

    if (!pArg) return E_FAIL;
    const DESC* in = static_cast<DESC*>(pArg);
    m_L = in->handL;
    m_R = in->handR;
    m_M = in->mask;
    m_maxHP = in->maxHP;
    m_hp = in->maxHP;
    m_phaseChangeHP = in->phaseChangeHP;

    // ?? 시작부터 슬램 패턴
    m_state = State::PATTERN_SLAM;
    m_stateTimer = 0.f;

    _float3 tgt{}; XMStoreFloat3(&tgt, PlayerPos());
    if (m_L) m_L->StartSlam(tgt);
    if (m_R) m_R->StartSlam(tgt);

    OutputDebugStringA("[BOSS] Controller INIT -> PATTERN_SLAM (fire disabled)\n");
    return S_OK;
}

void CBoss_Controller::Update(_float dt)
{
    // 페이즈는 유지(2P 선회 쓰고 싶으면 켜짐)
    const float hpRate = (m_maxHP > 0.f) ? (m_hp / m_maxHP) : 1.f;
    m_phase = (hpRate <= m_phaseChangeHP) ? Phase::P2 : Phase::P1;

    TickPhase(dt);
    TickPattern(dt);
}

void CBoss_Controller::Late_Update(_float)
{
}

void CBoss_Controller::ApplyDamage(_float amount)
{
    if (amount <= 0.f) return;
    m_hp = max(0.f, m_hp - amount);
}

// =============== 내부 로직 ===============

void CBoss_Controller::TickPhase(_float dt)
{
    if (m_phase == Phase::P1) TickMoveP1(dt);
    else                      TickMoveP2(dt);
}

void CBoss_Controller::TickMoveP1(_float /*dt*/)
{
    if (m_M)
    {
        _float3 pos{};
        XMStoreFloat3(&pos, m_M->Get_Transform()->Get_State(STATE::POSITION));
        m_center = pos;
    }
}

void CBoss_Controller::TickMoveP2(_float dt)
{
    if (!m_L || !m_R) return;

    _float3 C = m_center;
    if (m_M)
        XMStoreFloat3(&C, m_M->Get_Transform()->Get_State(STATE::POSITION));

    m_orbitAngle += m_orbitSpd * dt;

    const float aL = m_orbitAngle;
    const float aR = aL + XM_PI;

    const _float3 pL{ C.x + m_orbitRadius * cosf(aL), C.y + m_handYOffset, C.z + m_orbitRadius * sinf(aL) };
    const _float3 pR{ C.x + m_orbitRadius * cosf(aR), C.y + m_handYOffset, C.z + m_orbitRadius * sinf(aR) };

    if (auto* t = m_L->Get_Transform()) t->Set_State(STATE::POSITION, XMLoadFloat3(&pL));
    if (auto* t = m_R->Get_Transform()) t->Set_State(STATE::POSITION, XMLoadFloat3(&pR));
}

void CBoss_Controller::TickPattern(_float dt)
{
    m_stateTimer += dt;

    switch (m_state)
    {
    case State::PATTERN_SLAM:
        // 손 내부에서 RISE→DOWN→RECOVER 진행. 여기서는 기간만 체크.
        if (m_stateTimer >= 1.2f)
            ExitPattern(); // → COOLDOWN
        break;

    case State::COOLDOWN:
        if (m_stateTimer >= m_coolTime) {
            // 다음 슬램 즉시 시작
            m_state = State::PATTERN_SLAM;
            m_stateTimer = 0.f;

            _float3 tgt{}; XMStoreFloat3(&tgt, PlayerPos());
            if (m_L) m_L->StartSlam(tgt);
            if (m_R) m_R->StartSlam(tgt);
        }
        break;

    case State::IDLE:
    case State::PATTERN_FIRE:
        // 발사/대기 전부 사용 안 함 → 슬램으로 전환
        m_state = State::PATTERN_SLAM;
        m_stateTimer = 0.f;
        {
            _float3 tgt{}; XMStoreFloat3(&tgt, PlayerPos());
            if (m_L) m_L->StartSlam(tgt);
            if (m_R) m_R->StartSlam(tgt);
        }
        break;
    }
}

void CBoss_Controller::EnterFire()
{
    // 사용 안 함
    m_state = State::COOLDOWN;
    m_stateTimer = 0.f;
}

void CBoss_Controller::EnterSlam()
{
    // 직접 호출 안 하지만 남겨둠(디버그용)
    m_state = State::PATTERN_SLAM;
    m_stateTimer = 0.f;

    _float3 tgt{}; XMStoreFloat3(&tgt, PlayerPos());
    if (m_L) m_L->StartSlam(tgt);
    if (m_R) m_R->StartSlam(tgt);
}

void CBoss_Controller::ExitPattern()
{
    m_state = State::COOLDOWN;
    m_stateTimer = 0.f;
}

_vector CBoss_Controller::PlayerPos() const
{
    if (auto* pm = CPlayerManager::GetInstance()) return pm->GetPos();
    return XMVectorZero();
}

// ===== 생성/클론 =====

CBoss_Controller* CBoss_Controller::Create(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    auto* p = new CBoss_Controller(dev, ctx);
    if (FAILED(p->Initialize_Prototype())) { MSG_BOX(L"Failed to Created : CBoss_Controller"); Safe_Release(p); }
    return p;
}

CGameObject* CBoss_Controller::Clone(void* pArg)
{
    auto* p = new CBoss_Controller(*this);
    if (FAILED(p->Initialize(pArg))) { MSG_BOX(L"Failed to Clone : CBoss_Controller"); Safe_Release(p); }
    return p;
}
