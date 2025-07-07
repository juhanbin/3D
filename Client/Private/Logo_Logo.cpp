#include "Logo_Logo.h"
#include "GameInstance.h"

CLogo_Logo::CLogo_Logo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext }
{
}

CLogo_Logo::CLogo_Logo(const CLogo_Logo& Prototype)
    : CUIObject{ Prototype }
{
}

HRESULT CLogo_Logo::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CLogo_Logo::Initialize(void* pArg)
{
    UIOBJECT_DESC               Desc{};
    Desc.fX = g_iWinSizeX >> 1;
    Desc.fY = (g_iWinSizeY >> 1);
    Desc.fSizeX = (g_iWinSizeX >>1) - 60;
    Desc.fSizeY = (g_iWinSizeY >>1) + 190;

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;
    /*D3D11_SHADER_DESC*/
    return S_OK;
}

void CLogo_Logo::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CLogo_Logo::Update(_float fTimeDelta)
{
    int a = 10;
}

void CLogo_Logo::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this)))
        return;
}

HRESULT CLogo_Logo::Render()
{
    /*
    m_pShaderCom->Bind_Texture();*/

    __super::Begin();

    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix)))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix)))
        return E_FAIL;

    if (FAILED(m_pTextureCom->Bind_Shader_Resource(m_pShaderCom, "g_Texture", 0)))
        return E_FAIL;

    m_pShaderCom->Begin(0);

    m_pVIBufferCom->Bind_Resources();


    m_pVIBufferCom->Render();

    return S_OK;
}

HRESULT CLogo_Logo::Ready_Components()
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

CLogo_Logo* CLogo_Logo::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CLogo_Logo* pInstance = new CLogo_Logo(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CLogo_Logo"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CLogo_Logo::Clone(void* pArg)
{
    CLogo_Logo* pInstance = new CLogo_Logo(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CLogo_Logo"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CLogo_Logo::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}
