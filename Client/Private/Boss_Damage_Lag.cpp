// Boss_Damage_Lag.cpp
#include "Boss_Damage_Lag.h"
#include "GameInstance.h"
#include <algorithm>

USING(Client)

CBoss_Damage_Lag::CBoss_Damage_Lag(ID3D11Device* d, ID3D11DeviceContext* c) : CUIObject(d, c) {}
CBoss_Damage_Lag::CBoss_Damage_Lag(const CBoss_Damage_Lag& rhs) : CUIObject(rhs) {}
HRESULT CBoss_Damage_Lag::Initialize_Prototype() { return S_OK; }

HRESULT CBoss_Damage_Lag::Initialize(void* pArg)
{
    UIOBJECT_DESC desc{};
    desc.fSizeX = (float)g_iWinSizeX / 3.f + 10.f;
    desc.fSizeY = 20.f;
    desc.fX = (float)g_iWinSizeX * 0.5f;
    desc.fY = 120.f;

    if (FAILED(__super::Initialize(&desc))) return E_FAIL;
    if (FAILED(Ready_Components()))         return E_FAIL;

    if (pArg) {
        auto* d = (DESC*)pArg;
        m_fnGetActual = d->fnGetActualRatio;
        m_edge = d->edgeSoft;
        m_catchPerSec = d->catchUpPerSec;
    }
    return S_OK;
}

void CBoss_Damage_Lag::Update(_float dt)
{
    const float dtSec = (dt > 5.f) ? (dt * 0.001f) : dt;

    // 실제(목표) 값
    float actual = 1.f;
    if (m_fnGetActual) actual = Clamp(m_fnGetActual(), 0.f, 1.f);
    m_target = actual;

    // 힐은 즉시, 데미지는 서서히
    if (m_visual < m_target) {
        m_visual = m_target;
    }
    else {
        m_visual = max(m_target, m_visual - m_catchPerSec * dtSec);
    }
}

void CBoss_Damage_Lag::Late_Update(_float) {
    m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this);
}

HRESULT CBoss_Damage_Lag::Render()
{
    __super::Begin();

    m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix");
    m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix);
    m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix);

    m_pMaskTex->Bind_Shader_Resource(m_pShaderCom, "g_Tex", 0);

    const int   mode = 2; // Delay
    const float fill = Clamp(m_visual, 0.f, 1.f);

    m_pShaderCom->Bind_RawValue("g_Mode", &mode, sizeof(int));
    m_pShaderCom->Bind_RawValue("g_Fill", &fill, sizeof(float));
    m_pShaderCom->Bind_RawValue("g_EdgeSoft", &m_edge, sizeof(float));
    m_pShaderCom->Bind_RawValue("g_Color", &m_LagColor, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_EmptyColor", &m_EmptyColor, sizeof(_float4));

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Bind_Resources();
    m_pVIBufferCom->Render();
    return S_OK;
}

HRESULT CBoss_Damage_Lag::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_Boss_HP"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Texture_HP_Bar"),
        TEXT("Com_Tex"), reinterpret_cast<CComponent**>(&m_pMaskTex), nullptr)))
        return E_FAIL;

    return S_OK;
}

CBoss_Damage_Lag* CBoss_Damage_Lag::Create(ID3D11Device* d, ID3D11DeviceContext* c)
{
    auto* p = new CBoss_Damage_Lag(d, c);
    if (FAILED(p->Initialize_Prototype())) { Safe_Release(p); return nullptr; }
    return p;
}
CGameObject* CBoss_Damage_Lag::Clone(void* pArg)
{
    auto* p = new CBoss_Damage_Lag(*this);
    if (FAILED(p->Initialize(pArg))) { Safe_Release(p); return nullptr; }
    return p;
}
void CBoss_Damage_Lag::Free()
{
    __super::Free();
    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pMaskTex);
    Safe_Release(m_pShaderCom);
}
