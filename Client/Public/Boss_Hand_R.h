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

class CBoss_Hand_R final : public CGameObject
{
public:
    struct Boss_Hand_R_DESC
    {
        EObjectType type{ EObjectType::MUSHROOM };
        _float3 vScale{ 1.f,1.f,1.f };
        _float3 vRot{ 0.f,0.f,0.f };
        _float3 vPos{ 0.f,0.f,0.f };
    };

private:
    CBoss_Hand_R(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CBoss_Hand_R(const CBoss_Hand_R& Prototype);
    virtual ~CBoss_Hand_R() = default;

public:
    // CGameObject
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT                 Ready_Components();
    HRESULT                 Bind_ShaderResources();

private:
    // 렌더/애니
    Engine::CShader* m_pShaderCom = nullptr;
    Engine::CModel* m_pModelCom = nullptr;

    // 네비/콜리전
    Engine::CNavigation* m_pNavigationCom = nullptr;
    Engine::CCollider* m_pCollider = nullptr; // 생존 중 플레이어 이동 방해

private:
    EObjectType             m_eType = EObjectType::BOSS_HAND_R;

public:
    static CBoss_Hand_R* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void            Free() override;
};

NS_END
