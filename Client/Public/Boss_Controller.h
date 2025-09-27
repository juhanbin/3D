#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

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
        CBoss_Mask*   mask  = nullptr;

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
    void    Priority_Update(_float dt) override {}
    void    Update(_float dt) override;
    void    Late_Update(_float dt) override;
    HRESULT Render() override { return S_OK; } // 컨트롤러는 렌더 없음

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

private:
    // 참조 파츠
    CBoss_Hand_L* m_L = nullptr;
    CBoss_Hand_R* m_R = nullptr;
    CBoss_Mask*   m_M = nullptr;

    // 전투 파라미터
    Phase m_phase = Phase::P1;
    State m_state = State::IDLE;

    _float m_maxHP = 3000.f;
    _float m_hp = 3000.f;
    _float m_phaseChangeHP = 0.5f;

    // 패턴 타이머
    _float m_stateTimer = 0.f;
    _float m_firePeriod = 1.0f;   // 양손 교차 발사 주기
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
    _float3 m_center{ 0.f, 3.0f, 0.f }; // 마스크가 중앙에 있을 때 기준
    _float  m_handYOffset = 0.0f;

    // === 디버그 입력(K키 1회 눌림 처리) ===
    bool  m_kHeld = false;      // 이전 프레임 K 상태
    float m_debugDamage = 100.f; // K 한번에 줄 HP량
};

NS_END
