#pragma once
#include "Client_Defines.h"
#include "PartObject.h"
#include "Monster_Skeleton.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CWeapon_Skeleton_Bow final : public CPartObject
{
public:
    struct WEAPON_DESC : public CPartObject::PARTOBJECT_DESC
    {
        const _float4x4* pSocketMatrix = nullptr;
        MONSTER* pState = nullptr;
        Engine::CModel* pAnimModel = nullptr; // 부모(바디) 모델
    };

private:
    CWeapon_Skeleton_Bow(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CWeapon_Skeleton_Bow(const CWeapon_Skeleton_Bow& Prototype);
    virtual ~CWeapon_Skeleton_Bow() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    void      TickAimSync(_float dt);
    void      FireOnce();
    _float4x4 Get_SocketWorldMatrix() const;
    _float3   Get_SocketWorldPos() const;
    bool      IsAimedEnough(float degThreshold = 10.f) const;

private:
    Engine::CShader* m_pShaderCom = nullptr;
    Engine::CModel* m_pModelCom = nullptr;

    const _float4x4* m_pSocketMatrix = nullptr;
    MONSTER* m_pParentState = nullptr;
    Engine::CModel* m_pAnimModel = nullptr;

    // 애니메이션 동기화
    float m_FireAtNormalized = 60.f / 76.f;
    float m_AimDegThreshold = 10.f;

    bool  m_InAttackState = false;
    bool  m_FiredThisCycle = false;

    // ★ 루프 감지용(로컬) 진행도 샘플
    float m_prevAnim01Local = -1.f;

    // 물리/오프셋
    float m_MuzzleFwd = 0.25f;
    float m_MuzzleUp = 0.05f;
    float m_InitialSpeed = 18.f;
    float m_Gravity = -9.0f;

    _float4x4 m_CombinedWorldMatrix{};

public:
    static CWeapon_Skeleton_Bow* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void         Free() override;
};

NS_END
