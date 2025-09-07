#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"   // CContainerObject
//#include "Transform.h"         // _vector helpers

NS_BEGIN(Engine)
class CCollider;
class CNavigation;
NS_END

NS_BEGIN(Client)
class CBody_Player;
class CWeapon;

class CPlayer final : public CContainerObject
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
    CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CPlayer(const CPlayer& Prototype);
    virtual ~CPlayer() = default;

public:
    // CGameObject
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

public:
    _vector Get_TransformState(Engine::STATE s) const;
    _vector GetPos() const;
    _vector GetForward(bool flattenY) const;
    _vector GetRight() const;
    _vector GetUp() const;

    void Throw_Spear();

private:
    HRESULT Ready_Components();
    HRESULT Ready_PartObjects();

private:
    Engine::CNavigation* m_pNavigationCom = nullptr;
    CCollider* m_pColliderCom = { nullptr };

    //ป๓ลย
    _uint   m_iState = 0;
    EObjectType    m_eType = EObjectType::HERO;

    MOVING  m_eMoving = MOVING::IDLE;
    ATTACK  m_eAttack = ATTACK::NONE;

    bool    m_bShiftPressed = false;
    float   m_fShiftHeldSec = 0.f;

    int     m_iDashFlagFrames = 0;

private:

    static constexpr float kRunHoldThreshold = 0.23f; 
    static constexpr float kDashImpulseMul = 20.0f;
    static constexpr float kRunMul = 1.3f;
    static constexpr float kAimWalkMul = 0.3f;
    static constexpr float kAimRunMul = 0.6f;

public:
    static CPlayer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void         Free() override;
};

NS_END
