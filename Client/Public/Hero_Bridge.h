#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"

NS_BEGIN(Engine)
class CCollider;
class CNavigation;
NS_END

NS_BEGIN(Client)
class CBody_Into;
class CWeapon_Intro;

class CHero_Bridge final : public CContainerObject
{
public:
    struct HERO_DESC
    {
        EObjectType type{ EObjectType::HERO };
        _float3 vScale{ 1.f,1.f,1.f };
        _float3 vRot{ 0.f,0.f,0.f };
        _float3 vPos{ 0.f,0.f,0.f };
    };

private:
    CHero_Bridge(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CHero_Bridge(const CHero_Bridge& Prototype);
    virtual ~CHero_Bridge() = default;

public:
    // CGameObject
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

public: // helpers
    _vector Get_TransformState(Engine::STATE s) const;
    _vector GetPos() const;
    _vector GetForward(bool flattenY) const;
    _vector GetRight() const;
    _vector GetUp() const;

    void    Throw_Spear();

private:
    HRESULT Ready_Components();
    HRESULT Ready_PartObjects();

    // ---- 충돌/트리거 유틸 ----
    bool    CheckBlockingWithLayer(const _wstring& layerName) const;
    bool    CheckTriggerWithLayer(const _wstring& layerName) const;

    // 이동 후 막히면 되돌리기
    bool    ResolveBlockingCollisions();

    // 트리거 데미지 처리(틱 쿨다운)
    void    TickDamageTriggers(float dt);

    // HP 처리(매니저 연동)
    void    ApplyDamage(int amount);
    void TickHostileHits(float dt);
    void DebugTickHPKeys();
    void ApplyDamagePM(float amount);
    void HealPM(float amount);

private:
    Engine::CNavigation* m_pNavigationCom = nullptr;
    CCollider* m_pColliderCom = nullptr;

    // 상태
    _uint       m_iState = 0;
    EObjectType m_eType = EObjectType::HERO;

    MOVING  m_eMoving = MOVING::IDLE;
    ATTACK  m_eAttack = ATTACK::NONE;

    bool    m_bShiftPressed = false;
    float   m_fShiftHeldSec = 0.f;

    int     m_iDashFlagFrames = 0;

    // 트리거 데미지 관리
    float   m_damageTickGap = 0.25f;  // 데미지 틱 간격(초)
    float   m_damageTickAcc = 0.f;    // 누적

private:
    float m_iframeSec = 0.f;                 // 남은 무적시간
    const float kIFrameDuration = 0.8f;      // 무적시간 길이(원하는 값)
    float m_worldTime = 0.f;                 // 누적 시간
    std::unordered_map<uintptr_t, float> m_lastHitAt; // 가해자별 마지막 히트 시간
    const float kPerSourceCooldown = 0.35f;  // 같은 무기/화살에게 재히트 허용 간격

    struct WorldSnapshot {
        DirectX::XMFLOAT4 right, up, look, pos;
    };

    inline WorldSnapshot CaptureWorld(Engine::CTransform* t) {
        WorldSnapshot s{};
        XMStoreFloat4(&s.right, t->Get_State(Engine::STATE::RIGHT));
        XMStoreFloat4(&s.up, t->Get_State(Engine::STATE::UP));
        XMStoreFloat4(&s.look, t->Get_State(Engine::STATE::LOOK));
        XMStoreFloat4(&s.pos, t->Get_State(Engine::STATE::POSITION));
        return s;
    }

    inline void RestoreWorld(Engine::CTransform* t, const WorldSnapshot& s) {
        t->Set_State(Engine::STATE::RIGHT, XMLoadFloat4(&s.right));
        t->Set_State(Engine::STATE::UP, XMLoadFloat4(&s.up));
        t->Set_State(Engine::STATE::LOOK, XMLoadFloat4(&s.look));
        t->Set_State(Engine::STATE::POSITION, XMLoadFloat4(&s.pos));
    }

private:
    // 이동/조준 배율
    static constexpr float kRunHoldThreshold = 0.23f;
    static constexpr float kDashImpulseMul = 20.0f;
    static constexpr float kRunMul = 1.3f;
    static constexpr float kAimWalkMul = 0.3f;
    static constexpr float kAimRunMul = 0.6f;

public:
    static CHero_Bridge* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void         Free() override;
};

NS_END
