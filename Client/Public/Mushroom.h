#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CCollider;
class CShader;
class CModel;
class CNavigation;
NS_END

NS_BEGIN(Client)

class CMushroom final : public CGameObject
{
public:
    struct Mushroom_DESC
    {
        EObjectType type{ EObjectType::MUSHROOM };
        _float3 vScale{ 1.f,1.f,1.f };
        _float3 vRot{ 0.f,0.f,0.f };
        _float3 vPos{ 0.f,0.f,0.f };
    };

    enum AnimIndex : int {
        ANIM_IDLE = 0,
        ANIM_DIE = 1,
    };

private:
    // 간단 상태: 살아있음 → 즉시 DEAD(요구대로 사망 애니 생략/고정 가능)
    enum class STATE { ALIVE, DEAD };

private:
    CMushroom(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CMushroom(const CMushroom& Prototype);
    virtual ~CMushroom() = default;

public:
    // CGameObject
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

    // -------- 외부 조회 --------
    bool                    IsAlive() const { return m_eState == STATE::ALIVE; }
    bool                    IsDead()  const { return m_eState == STATE::DEAD; }

    // 살아있을 때만 "막는" 본체 콜라이더 유효
    Engine::CCollider* GetCollider_Block() const {
        return (m_eState == STATE::ALIVE) ? m_pCollider_Block : nullptr;
    }

    // 죽은 뒤 3초 동안만 트리거 유효(데미지 적용은 플레이어에서)
    Engine::CCollider* GetCollider_TriggerIfActive() const {
        return (m_eState == STATE::DEAD && m_deadElapsed < m_triggerLife) ? m_pCollider_Trigger : nullptr;
    }

    // 투사체/공격에서 호출
    void                    TakeDamage(int dmg, Engine::CGameObject* src = nullptr);

private:
    HRESULT                 Ready_Components();
    HRESULT                 Bind_ShaderResources();

private:
    // 렌더/애니
    Engine::CShader* m_pShaderCom = nullptr;
    Engine::CModel* m_pModelCom = nullptr;

    // 네비/콜리전
    Engine::CNavigation* m_pNavigationCom = nullptr;
    Engine::CCollider* m_pCollider_Block = nullptr; // 생존 중 플레이어 이동 방해
    Engine::CCollider* m_pCollider_Trigger = nullptr; // 사망 후 트리거(3초)

    // 상태/수치
    EObjectType             m_eType = EObjectType::MUSHROOM;
    STATE                   m_eState = STATE::ALIVE;
    int                     m_hp = 30;

    // 사망 트리거 관리
    float                   m_deadElapsed = 0.f; // DEAD 경과시간
    float                   m_triggerLife = 3.f; // 트리거 유지시간(초)

public:
    static CMushroom* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void            Free() override;
};

NS_END
