#include "Logo_StartButton.h"
#include "GameInstance.h"
#include <windowsx.h>

CLogo_StartButton::CLogo_StartButton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext }
{
}

CLogo_StartButton::CLogo_StartButton(const CLogo_StartButton& Prototype)
    : CUIObject{ Prototype }
{
}

HRESULT CLogo_StartButton::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CLogo_StartButton::Initialize(void* pArg)
{
    UIOBJECT_DESC Desc{};
    Desc.fX = g_iWinSizeX - 150.f;
    Desc.fY = g_iWinSizeY - 100.f;
    Desc.fSizeX = 200.f;
    Desc.fSizeY = 80.f;

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CLogo_StartButton::Priority_Update(_float fTimeDelta)
{
}

void CLogo_StartButton::Update(_float fTimeDelta)
{
    POINT pt{ 0, 0 };
    GetCursorPos(&pt);
    ScreenToClient(g_hWnd, &pt);

    bool bInRect = (pt.x >= m_fX - m_fSizeX * 0.5f && pt.x <= m_fX + m_fSizeX * 0.5f &&
        pt.y >= m_fY - m_fSizeY * 0.5f && pt.y <= m_fY + m_fSizeY * 0.5f);

    if (GetKeyState(VK_LBUTTON) & 0x8000)
    {
        if (!m_bPrevClick && bInRect)
        {
            m_pGameInstance->Dispatch_Event(static_cast<_uint>(LEVEL::LOGO), TEXT("StartButton"));
            m_bPrevClick = true;
        }
    }
    else
    {
        m_bPrevClick = false;
    }
}

void CLogo_StartButton::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this)))
        return;
}

HRESULT CLogo_StartButton::Render()
{
    __super::Begin();

    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix)))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix)))
        return E_FAIL;

    if (FAILED(m_pTextureCom->Bind_Shader_Resource(m_pShaderCom, "g_Texture",0 )))
        return E_FAIL;

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Bind_Resources();
    m_pVIBufferCom->Render();

    return S_OK;
}

HRESULT CLogo_StartButton::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_Logo"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_Component_Texture_Logo_Logo"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom), nullptr)))
        return E_FAIL;

    return S_OK;
}

CLogo_StartButton* CLogo_StartButton::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CLogo_StartButton* pInstance = new CLogo_StartButton(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CLogo_StartButton"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CLogo_StartButton::Clone(void* pArg)
{
    CLogo_StartButton* pInstance = new CLogo_StartButton(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CLogo_StartButton"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CLogo_StartButton::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}