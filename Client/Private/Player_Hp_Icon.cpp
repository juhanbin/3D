#include "Player_Hp_Icon.h"
#include "GameInstance.h"

CPlayer_Hp_Icon::CPlayer_Hp_Icon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext }
{
}

CPlayer_Hp_Icon::CPlayer_Hp_Icon(const CPlayer_Hp_Icon& Prototype)
    : CUIObject{ Prototype }
{
}

HRESULT CPlayer_Hp_Icon::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CPlayer_Hp_Icon::Initialize(void* pArg)
{
    UIOBJECT_DESC bar{};
    bar.fSizeX = 50.f;  // 네가 쓰던 크기 유지
    bar.fSizeY = 50.f; // 네가 쓰던 크기 유지
    bar.fX = 140.f;
    bar.fY = 630.f;

    if (FAILED(__super::Initialize(&bar)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CPlayer_Hp_Icon::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CPlayer_Hp_Icon::Update(_float fTimeDelta)
{

}

void CPlayer_Hp_Icon::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this)))
        return;
}

HRESULT CPlayer_Hp_Icon::Render()
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

    if (FAILED(m_pTextureCom->Bind_Shader_Resource(m_pShaderCom, "g_Texture", 0))) return E_FAIL;

    m_pShaderCom->Begin(0);

    m_pVIBufferCom->Bind_Resources();


    m_pVIBufferCom->Render();

    return S_OK;
}

HRESULT CPlayer_Hp_Icon::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_HP_Bar"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Texture_HP_Bar_Icon"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom), nullptr)))
        return E_FAIL;




    return S_OK;
}

CPlayer_Hp_Icon* CPlayer_Hp_Icon::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CPlayer_Hp_Icon* pInstance = new CPlayer_Hp_Icon(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CPlayer_Hp_Icon"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CPlayer_Hp_Icon::Clone(void* pArg)
{
    CPlayer_Hp_Icon* pInstance = new CPlayer_Hp_Icon(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CPlayer_Hp_Icon"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CPlayer_Hp_Icon::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}
