#include "Fade.h"
#include "GameInstance.h"

CFade::CFade(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext }
{
}

CFade::CFade(const CFade& Prototype)
    : CUIObject{ Prototype }
{
}

HRESULT CFade::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CFade::Initialize(void* pArg)
{
    UIOBJECT_DESC Desc{};
    Desc.fX = g_iWinSizeX * 0.5f;
    Desc.fY = g_iWinSizeY * 0.5f;
    Desc.fSizeX = static_cast<_float>(g_iWinSizeX);
    Desc.fSizeY = static_cast<_float>(g_iWinSizeY);

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CFade::Priority_Update(_float fTimeDelta)
{

}

void CFade::Update(_float fTimeDelta)
{
    m_fAlpha -= fTimeDelta * 0.5;
    if (m_fAlpha < 0.f)
        m_fAlpha = 0.f;
}

void CFade::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::FADE, this)))
        return;
}

HRESULT CFade::Render()
{
    __super::Begin();

    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix)))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix)))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_fAlpha", &m_fAlpha, sizeof(float))))
        return E_FAIL;

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Bind_Resources();
    m_pVIBufferCom->Render();

    return S_OK;
}

HRESULT CFade::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_Fade"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    return S_OK;
}

CFade* CFade::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CFade* pInstance = new CFade(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CFade"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CFade::Clone(void* pArg)
{
    CFade* pInstance = new CFade(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CFade"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CFade::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pShaderCom);
}
