#include "Player_Hp_Frame.h"
#include "GameInstance.h"

CPlayer_Hp_Frame::CPlayer_Hp_Frame(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext }
{
}

CPlayer_Hp_Frame::CPlayer_Hp_Frame(const CPlayer_Hp_Frame& Prototype)
    : CUIObject{ Prototype }
{
}

HRESULT CPlayer_Hp_Frame::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CPlayer_Hp_Frame::Initialize(void* pArg)
{
    const _float screenW = (_float)g_iWinSizeX; // 1280
    const _float screenH = (_float)g_iWinSizeY; // 720

    // 현재 너비/높이는 네가 쓰던 값 유지
    const _float width = 520.f;
    const _float height = 18.f;

    // 기존 배치값
    const _float left = 90.f;
    const _float bottom = 70.f;

    // --- 여기만 손대면 위치만 바뀜 ---
    const _float offsetLeft = -30.f;  // 왼쪽으로 30px
    const _float offsetUp = 12.f;  // 위로 12px
    // ----------------------------------

    UIOBJECT_DESC Desc{};
    Desc.fSizeX = width - 200.f;   // 네가 쓰던 스케일 그대로
    Desc.fSizeY = height + 250.f;

    const _float cx = (left + offsetLeft) + width * 0.5f;
    const _float cy = screenH - ((bottom + offsetUp) + height * 0.5f);

    // 픽셀 스냅(살짝 흐려지는 것 방지용, 선택)
    Desc.fX = roundf(cx);
    Desc.fY = roundf(cy);

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;
    /*D3D11_SHADER_DESC*/
    return S_OK;
}

void CPlayer_Hp_Frame::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CPlayer_Hp_Frame::Update(_float fTimeDelta)
{

}

void CPlayer_Hp_Frame::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this)))
        return;
}

HRESULT CPlayer_Hp_Frame::Render()
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

HRESULT CPlayer_Hp_Frame::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_HP_Bar"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Texture_HP_Bar_Frame"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom), nullptr)))
        return E_FAIL;




    return S_OK;
}

CPlayer_Hp_Frame* CPlayer_Hp_Frame::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CPlayer_Hp_Frame* pInstance = new CPlayer_Hp_Frame(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CPlayer_Hp_Frame"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CPlayer_Hp_Frame::Clone(void* pArg)
{
    CPlayer_Hp_Frame* pInstance = new CPlayer_Hp_Frame(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CPlayer_Hp_Frame"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CPlayer_Hp_Frame::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}
