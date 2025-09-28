#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
#include <unordered_map>

NS_BEGIN(Engine)
class CCollider;
NS_END

NS_BEGIN(Client)

class CBoss_Hand_L;
class CBoss_Hand_R;
class CBoss_Mask;

class CBoss_Controller final : public CGameObject
{
public:
    struct DESC {
        CBoss_Hand_L* handL = nullptr;
        CBoss_Hand_R* handR = nullptr;
        CBoss_Mask* mask = nullptr;

        // 1P: 정지, 2P: 선회
        _float phaseChangeHP = 0.5f; // 남은 HP 비율이 이하면 2페이즈
        _float maxHP = 3000.f;
    };

public:
    CBoss_Controller(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    CBoss_Controller(const CBoss_Controller& rhs);
    virtual ~CBoss_Controller() = default;

    static CBoss_Controller* Create(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    virtual CGameObject* Clone(void* pArg) override;

    // CGameObject
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* pArg) override;
    void    Priority_Update(_float /*dt*/) override {}
    void    Update(_float dt) override;
    void    Late_Update(_float dt) override;
    HRESULT Render() override;
    void    Free() override;

    // 외부에서 데미지
    void ApplyDamage(_float amount);

    // HP 게터(HP UI에서 사용)
    float GetHP()     const { return m_hp; }
    float GetMaxHP()  const { return m_maxHP; }
    float GetHPRate() const { return (m_maxHP > 0.f) ? (m_hp / m_maxHP) : 0.f; }

private:
    enum class Phase { P1, P2 };
    enum class State { IDLE, PATTERN_FIRE, PATTERN_SLAM, COOLDOWN };

    // 내부 로직
    void TickPhase(_float dt);
    void TickMoveP1(_float dt);
    void TickMoveP2(_float dt);
    void TickPattern(_float dt);

    void EnterFire();
    void EnterSlam();
    void ExitPattern();

    // 유틸
    _vector PlayerPos() const;

    // ==== 컴포넌트/충돌 ====
    HRESULT Ready_Components();        // 피격 콜라이더 준비
    void    TickCollisions(_float dt); // 플레이어 창과의 충돌 체크

    // ==== 야매 사망 연출 ====
    void EnterDead();        // 사망 상태 진입(손 오프셋 저장 포함)
    void TickDead(float dt); // 사망 틱(마스크/손 함께 낙하)

private:
    // 참조 파츠
    CBoss_Hand_L* m_L = nullptr;
    CBoss_Hand_R* m_R = nullptr;
    CBoss_Mask* m_M = nullptr;

    // 전투 파라미터
    Phase m_phase = Phase::P1;
    State m_state = State::IDLE;

    _float m_maxHP = 3000.f;
    _float m_hp = 3000.f;
    _float m_phaseChangeHP = 0.5f;

    // 패턴 타이머
    _float m_stateTimer = 0.f;
    _float m_firePeriod = 1.0f;   // (미사용) 양손 교차 발사 주기
    _float m_fireTick = 0.f;
    bool   m_fireTurnL = true;

    _float m_slamCd = 4.0f;
    _float m_slamTimer = 0.f;

    _float m_coolTime = 1.0f;

    // 2P 선회
    _float m_orbitRadius = 10.f;
    _float m_orbitSpd = 0.7f;  // 라디안/초
    _float m_orbitAngle = 0.f;

    // 손 기본 기준 위치(마스크 주변)
    _float3 m_center{ 0.f, 3.0f, 0.f };
    _float  m_handYOffset = 0.0f;

    // 디버그
    bool  m_kHeld = false;
    float m_debugDamage = 100.f;

    // 히트 콜라이더 & 무적시간
    Engine::CCollider* m_pColliderCom = nullptr; // 보스 피격 콜라이더(구체)
    float  m_hurtRadius = 1.4f;                 // 피격반경
    float  m_worldTime = 0.f;                   // 누적 시간
    float  m_iframeSec = 0.f;                   // 전역 무적시간(연속 타격 방지)
    const  float kIFrame = 0.25f;               // 무적시간 길이
    std::unordered_map<uintptr_t, float> m_lastHitAt; // 창별 재히트 쿨다운
    const  float kPerSourceCD = 0.15f;          // 동일 창 재히트 최소 간격

    // ===== 야매 사망 상태 =====
    bool  m_isDead = false;
    float m_deadFallSpeed = 8.0f;   // 떨어지는 속도
    float m_deadSinkFloor = -20.0f; // 여기까지 떨어지면 비활성화
    float m_deadTimer = 0.f;    // 사망 경과 시간
    float m_deadLife = 2.0f;   // (옵션) 시간으로도 정리하고 싶으면 사용

    // 손이 마스크 대비 어디 있었는지(사망 연출 동안 유지)
    _float3 m_deadOffL{ 0,0,0 };
    _float3 m_deadOffR{ 0,0,0 };
};

NS_END
