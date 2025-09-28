#include "Boss_Controller.h"
#include "GameInstance.h"
#include "PlayerManager.h"
#include "Boss_Hand_L.h"
#include "Boss_Hand_R.h"
#include "Boss_Mask.h"

// 충돌/콜라이더 관련
#include "Collider.h"
#include "Bounding_Sphere.h"

// 플레이어 창
#include "Player_Speare.h"

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
    GAMEOBJECT_DESC d{}; d.fSpeedPerSec = 0.f; d.fRotationPerSec = 0.f;
    if (FAILED(__super::Initialize(&d))) return E_FAIL;

    if (!pArg) return E_FAIL;
    const DESC* in = static_cast<DESC*>(pArg);
    m_L = in->handL;
    m_R = in->handR;
    m_M = in->mask;
    m_maxHP = in->maxHP;
    m_hp = in->maxHP;
    m_phaseChangeHP = in->phaseChangeHP;

    // 피격 콜라이더 준비
    if (FAILED(Ready_Components()))
        return E_FAIL;

    // 시작: 슬램 패턴
    m_state = State::PATTERN_SLAM;
    m_stateTimer = 0.f;

    _float3 tgt{}; XMStoreFloat3(&tgt, PlayerPos());
    if (m_L) m_L->StartSlam(tgt);
    if (m_R) m_R->StartSlam(tgt);

    OutputDebugStringA("[BOSS] Controller INIT -> PATTERN_SLAM\n");
    return S_OK;
}

void CBoss_Controller::Update(_float dt)
{
    m_worldTime += dt;
    if (m_iframeSec > 0.f) m_iframeSec = max(0.f, m_iframeSec - dt);

    // === 야매 즉사 핫키: R ===
    if (m_pGameInstance->KeyDown(DIK_R)) {
        m_hp = 0.f;
        EnterDead();
    }

    // 마스크 위치를 컨트롤러(=피격 콜라이더)의 기준점으로 사용
    if (m_M)
        m_pTransformCom->Set_State(STATE::POSITION, m_M->Get_Transform()->Get_State(STATE::POSITION));
    else
        m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat3(&m_center));

    // 이미 죽었으면 낙하만 처리하고 나머지 로직 스킵
    if (m_isDead) {
        TickDead(dt);
        if (m_pColliderCom) m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
        return;
    }

    // 콜라이더 월드 갱신
    if (m_pColliderCom)
        m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());

    // 페이즈 결정
    const float hpRate = (m_maxHP > 0.f) ? (m_hp / m_maxHP) : 1.f;
    m_phase = (hpRate <= m_phaseChangeHP) ? Phase::P2 : Phase::P1;

    // 이동/패턴
    TickPhase(dt);
    TickPattern(dt);

    // 플레이어 창과의 충돌 처리
    TickCollisions(dt);

    // 충돌 처리 후 HP가 0 이하라면 사망 연출 진입
    if (m_hp <= 0.f && !m_isDead) {
        EnterDead();
    }

    if (m_pColliderCom) m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
}

void CBoss_Controller::Late_Update(_float /*dt*/)
{
    // 디버그용 렌더 그룹 등록(콜라이더 표시 등)
    m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this);
}

HRESULT CBoss_Controller::Render()
{
#ifdef _DEBUG
    if (m_pColliderCom)
        m_pColliderCom->Render();
#endif
    return S_OK;
}

void CBoss_Controller::ApplyDamage(_float amount)
{
    if (amount <= 0.f) return;
    m_hp = max(0.f, m_hp - amount);
    if (m_hp <= 0.f && !m_isDead) {
        EnterDead();
    }
}

/* ================= 내부 로직 ================= */

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
        // 발사/대기 사용 안 함 → 슬램으로 전환
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
    // 디버그 용도
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

/* ============== 충돌/컴포넌트 ============== */

HRESULT CBoss_Controller::Ready_Components()
{
    // 구체 콜라이더(보스 피격판정)
    CBounding_Sphere::BOUNDING_SPHERE_DESC S{};
    S.fRadius = m_hurtRadius;
    S.vCenter = _float3(0.f, 0.f, 0.f); // 마스크 위치에 그대로

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &S)))
        return E_FAIL;

    return S_OK;
}

void CBoss_Controller::TickCollisions(_float /*dt*/)
{
    if (!m_pColliderCom || m_isDead) return;

    // 전역 무적시간이면 스킵
    if (m_iframeSec > 0.f) return;

    const _uint level = ENUM_CLASS(LEVEL::BRIDGE);
    const std::wstring layerSpear = L"Layer_Spear";

    for (_uint i = 0;; ++i)
    {
        CGameObject* obj = m_pGameInstance->Find_GameObject(level, layerSpear, i);
        if (!obj) break;

        if (!obj->Is_Active()) continue;

        auto* spearCol = static_cast<Engine::CCollider*>(obj->Get_Component(L"Com_Collider"));
        if (!spearCol) continue;

        if (m_pColliderCom->Intersect(spearCol))
        {
            // 같은 창 오브젝트에 대한 재히트 간격 체크
            uintptr_t key = reinterpret_cast<uintptr_t>(obj);
            auto it = m_lastHitAt.find(key);
            if (it != m_lastHitAt.end() && (m_worldTime - it->second) < kPerSourceCD)
                continue;

            // 데미지 적용
            constexpr float kSpearDamageToBoss = 40.f;
            ApplyDamage(kSpearDamageToBoss);

            // 로그
            wchar_t wbuf[128];
            swprintf(wbuf, 128, L"[BOSS] hit by Spear: -%.1f  HP: %.1f / %.1f\n",
                kSpearDamageToBoss, m_hp, m_maxHP);
            OutputDebugStringW(wbuf);

            // 전역 무적시간 & 해당 창 타임스탬프 기록
            m_iframeSec = kIFrame;
            m_lastHitAt[key] = m_worldTime;

            // 창 회수(풀 쓰면 ReturnToPool, 아니면 비활성)
            if (auto* spear = dynamic_cast<Client::CPlayer_Speare*>(obj))
            {
                // spear->ReturnToPool(); // public이면 사용
                spear->Set_Active(false);
            }
            else
            {
                obj->Set_Active(false);
            }

            // 같은 프레임의 중복 피격 방지
            return;
        }
    }
}

/* ============== 야매 사망 연출 ============== */

void CBoss_Controller::EnterDead()
{
    if (m_isDead) return;
    m_isDead = true;

    // 패턴 정지
    m_state = State::IDLE;
    m_stateTimer = 0.f;

    // 무한 무적(추가 피격 방지)
    m_iframeSec = 9999.f;

    // --- 손 오프셋 계산(마스크 기준 상대위치 저장) ---
    _vector mposV = m_pTransformCom->Get_State(STATE::POSITION);
    if (m_M && m_M->Get_Transform())
        mposV = m_M->Get_Transform()->Get_State(STATE::POSITION);

    _float3 mpos; XMStoreFloat3(&mpos, mposV);

    if (m_L && m_L->Get_Transform()) {
        _float3 lpos; XMStoreFloat3(&lpos, m_L->Get_Transform()->Get_State(STATE::POSITION));
        m_deadOffL = _float3(lpos.x - mpos.x, lpos.y - mpos.y, lpos.z - mpos.z);
    }
    else {
        m_deadOffL = _float3(0, 0, 0);
    }

    if (m_R && m_R->Get_Transform()) {
        _float3 rpos; XMStoreFloat3(&rpos, m_R->Get_Transform()->Get_State(STATE::POSITION));
        m_deadOffR = _float3(rpos.x - mpos.x, rpos.y - mpos.y, rpos.z - mpos.z);
    }
    else {
        m_deadOffR = _float3(0, 0, 0);
    }

    OutputDebugStringA("[BOSS][DEBUG] EnterDead() : fake death start\n");
}

void CBoss_Controller::TickDead(float dt)
{
    // 1) 마스크(또는 컨트롤러)를 아래로 낙하
    _vector p = m_pTransformCom->Get_State(STATE::POSITION);
    if (m_M && m_M->Get_Transform())
        p = m_M->Get_Transform()->Get_State(STATE::POSITION);

    float y = XMVectorGetY(p) - m_deadFallSpeed * dt;
    p = XMVectorSetY(p, y);

    // 마스크/컨트롤러 동기화
    if (m_M && m_M->Get_Transform())
        m_M->Get_Transform()->Set_State(STATE::POSITION, p);
    m_pTransformCom->Set_State(STATE::POSITION, p);

    // 2) 손도 마스크 기준 오프셋 유지하며 같이 낙하
    _float3 m3; XMStoreFloat3(&m3, p);

    if (m_L && m_L->Get_Transform()) {
        _float3 lp{ m3.x + m_deadOffL.x, m3.y + m_deadOffL.y, m3.z + m_deadOffL.z };
        m_L->Get_Transform()->Set_State(STATE::POSITION, XMLoadFloat3(&lp));
    }
    if (m_R && m_R->Get_Transform()) {
        _float3 rp{ m3.x + m_deadOffR.x, m3.y + m_deadOffR.y, m3.z + m_deadOffR.z };
        m_R->Get_Transform()->Set_State(STATE::POSITION, XMLoadFloat3(&rp));
    }

    m_deadTimer += dt;

    // 충분히 가라앉으면 비활성화
    if (m3.y <= m_deadSinkFloor /*|| m_deadTimer >= m_deadLife*/) {
        this->Set_Active(false);
        OutputDebugStringA("[BOSS][DEBUG] dead finished, boss deactivated\n");
    }
}

/* ===== 생성/클론/해제 ===== */

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

void CBoss_Controller::Free()
{
    __super::Free();
    Safe_Release(m_pColliderCom);
}
