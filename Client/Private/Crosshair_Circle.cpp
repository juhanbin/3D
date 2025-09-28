#include "Crosshair_Circle.h"
#include "GameInstance.h"

CCrosshair_Circle::CCrosshair_Circle(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext }
{
}

CCrosshair_Circle::CCrosshair_Circle(const CCrosshair_Circle& Prototype)
    : CUIObject{ Prototype }
{
}

HRESULT CCrosshair_Circle::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CCrosshair_Circle::Initialize(void* pArg)
{
    
    UIOBJECT_DESC Desc{};
    Desc.fSizeX = 5.f;
    Desc.fSizeY = 5.f;

       // 픽셀 스냅(살짝 흐려지는 것 방지용, 선택)
    Desc.fX = g_iWinSizeX *0.5f ;
    Desc.fY = g_iWinSizeY * 0.5f;

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;
    /*D3D11_SHADER_DESC*/
    return S_OK;
}

void CCrosshair_Circle::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CCrosshair_Circle::Update(_float fTimeDelta)
{

}

void CCrosshair_Circle::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this)))
        return;
}

HRESULT CCrosshair_Circle::Render()
{
    if (!(m_pGameInstance->MousePressing(MOUSEKEYSTATE::LB) || m_pGameInstance->MousePressing(MOUSEKEYSTATE::RB)))
        return S_OK;

    __super::Begin();

    m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix");
    m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix);
    m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix);

    // ★ HLSL 상수 바인딩 (없으면 까만 화면)
    float alphaCut = 0.01f;                    // 필요하면 0.005~0.03 사이 튜닝
    _float4 tint = { 1.f, 1.f, 1.f, 1.f };     // 색 보정 없음

    m_pShaderCom->Bind_RawValue("g_AlphaCut", &alphaCut, sizeof(float));
    m_pShaderCom->Bind_RawValue("g_Tint", &tint, sizeof(_float4));

    m_pTextureCom->Bind_Shader_Resource(m_pShaderCom, "g_Texture", 0);

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Bind_Resources();
    m_pVIBufferCom->Render();
    return S_OK;
}


HRESULT CCrosshair_Circle::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_Logo"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Texture_Crosshair_Circle"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom), nullptr)))
        return E_FAIL;




    return S_OK;
}

CCrosshair_Circle* CCrosshair_Circle::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CCrosshair_Circle* pInstance = new CCrosshair_Circle(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CCrosshair_Circle"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CCrosshair_Circle::Clone(void* pArg)
{
    CCrosshair_Circle* pInstance = new CCrosshair_Circle(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CCrosshair_Circle"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CCrosshair_Circle::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}
