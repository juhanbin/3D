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

class CParasit_Eye final : public CGameObject
{
public:
    struct Parasit_Eye_Desc
    {
        EObjectType type{ EObjectType::PARASIT_EYE };
        _float3     vScale{ 1.f,1.f,1.f };
        _float3     vRot{ 0.f,0.f,0.f };
        _float3     vPos{ 0.f,0.f,0.f };
    };

private:
    CParasit_Eye(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CParasit_Eye(const CParasit_Eye& Prototype);
    virtual ~CParasit_Eye() = default;

public:
    // CGameObject
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

public: // === 활 패턴처럼 멤버 함수화 ===
    _float4x4 GetWorld() const;               // 손의 월드행렬
    _float3   GetMuzzleWorldPos() const;      // 손끝(총구) 위치
    _float3   AimDirToPlayer() const;         // 플레이어를 향한 단위 방향
    void      FireOnce();                     // 투사체 1발 발사
    void      TickFire(_float dt);            // 주기적으로 발사(쿨다운)

private:
    // 렌더/애니
    Engine::CShader* m_pShaderCom = nullptr;
    Engine::CModel* m_pModelCom = nullptr;

    // 네비/콜리전
    Engine::CNavigation* m_pNavigationCom = nullptr;
    Engine::CCollider* m_pCollider = nullptr; // (필요 시 사용)

    // 발사 파라미터
    _float m_fireCooldown = 1.2f;   // 발사 주기(초)
    _float m_initialSpeed = 16.f;   // 초기 속도
    _float m_gravity = -1.8f;  // 중력(음수=아래)
    _float m_muzzleFwd = 0.35f;  // 손 +Z 오프셋
    _float m_muzzleUp = 1.00f;  // 손 +Y 오프셋
    _float m_fireTimer = 0.f;    // 내부 타이머

private:
    EObjectType m_eType = EObjectType::PARASIT_EYE;

public:
    static CParasit_Eye* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void         Free() override;
};

NS_END
