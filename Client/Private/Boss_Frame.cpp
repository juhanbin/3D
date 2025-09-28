// Boss_Frame.cpp
#include "Boss_Frame.h"
#include "GameInstance.h"

USING(Client)

CBoss_Frame::CBoss_Frame(ID3D11Device* d, ID3D11DeviceContext* c) : CUIObject(d, c) {}
CBoss_Frame::CBoss_Frame(const CBoss_Frame& rhs) : CUIObject(rhs) {}
HRESULT CBoss_Frame::Initialize_Prototype() { return S_OK; }

HRESULT CBoss_Frame::Initialize(void*)
{
    UIOBJECT_DESC desc{};
    desc.fSizeX = (float)g_iWinSizeX / 3.f + 10.f;
    desc.fSizeY = 20.f;
    desc.fX = (float)g_iWinSizeX * 0.5f;
    desc.fY = 120.f;

    if (FAILED(__super::Initialize(&desc))) return E_FAIL;
    return Ready_Components();
}

void CBoss_Frame::Late_Update(_float) {
    m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this);
}

HRESULT CBoss_Frame::Render()
{
    __super::Begin();

    m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix");
    m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix);
    m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix);

    m_pMaskTex->Bind_Shader_Resource(m_pShaderCom, "g_Tex", 0);

    int   mode = 0;                          // Frame
    float fill = 1.0f;                       // unused
    _float4 empty = { 0,0,0,0 };

    m_pShaderCom->Bind_RawValue("g_Mode", &mode, sizeof(int));
    m_pShaderCom->Bind_RawValue("g_Fill", &fill, sizeof(float));
    m_pShaderCom->Bind_RawValue("g_EdgeSoft", &m_EdgeSoft, sizeof(float));
    m_pShaderCom->Bind_RawValue("g_Color", &m_FrameColor, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_EmptyColor", &empty, sizeof(_float4));

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Bind_Resources();
    m_pVIBufferCom->Render();
    return S_OK;
}

HRESULT CBoss_Frame::Ready_Components()
{
    if (FAILED(Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_Boss_HP"),
        TEXT("Com_Shader"), (CComponent**)&m_pShaderCom))) return E_FAIL;

    if (FAILED(Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), (CComponent**)&m_pVIBufferCom))) return E_FAIL;

    if (FAILED(Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Texture_HP_Bar"),
        TEXT("Com_Tex"), (CComponent**)&m_pMaskTex))) return E_FAIL;

    return S_OK;
}

CBoss_Frame* CBoss_Frame::Create(ID3D11Device* d, ID3D11DeviceContext* c)
{
    auto* p = new CBoss_Frame(d, c);
    if (FAILED(p->Initialize_Prototype())) { Safe_Release(p); return nullptr; }
    return p;
}
CGameObject* CBoss_Frame::Clone(void* pArg)
{
    auto* p = new CBoss_Frame(*this);
    if (FAILED(p->Initialize(pArg))) { Safe_Release(p); return nullptr; }
    return p;
}
void CBoss_Frame::Free()
{
    __super::Free();
    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pMaskTex);
    Safe_Release(m_pShaderCom);
}
