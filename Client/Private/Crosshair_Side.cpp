#include "Crosshair_Side.h"
#include "GameInstance.h"

CCrosshair_Side::CCrosshair_Side(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext }
{
}

CCrosshair_Side::CCrosshair_Side(const CCrosshair_Side& Prototype)
    : CUIObject{ Prototype }
{
}

HRESULT CCrosshair_Side::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CCrosshair_Side::Initialize(void* pArg)
{

    UIOBJECT_DESC Desc{};
    Desc.fSizeX = 20.f;
    Desc.fSizeY = 20.f;

    // 픽셀 스냅(살짝 흐려지는 것 방지용, 선택)
    Desc.fX = g_iWinSizeX * 0.5;
    Desc.fY = g_iWinSizeY * 0.5;

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;
    /*D3D11_SHADER_DESC*/
    return S_OK;
}

void CCrosshair_Side::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CCrosshair_Side::Update(_float fTimeDelta)
{

}

void CCrosshair_Side::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this)))
        return;
}

HRESULT CCrosshair_Side::Render()
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

HRESULT CCrosshair_Side::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_Logo"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Texture_Crosshair_Side"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom), nullptr)))
        return E_FAIL;




    return S_OK;
}

CCrosshair_Side* CCrosshair_Side::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CCrosshair_Side* pInstance = new CCrosshair_Side(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CCrosshair_Side"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CCrosshair_Side::Clone(void* pArg)
{
    CCrosshair_Side* pInstance = new CCrosshair_Side(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CCrosshair_Side"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CCrosshair_Side::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}
