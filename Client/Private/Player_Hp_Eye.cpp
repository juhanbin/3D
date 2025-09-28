#include "Player_Hp_Eye.h"
#include "GameInstance.h"

CPlayer_Hp_Eye::CPlayer_Hp_Eye(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext }
{
}

CPlayer_Hp_Eye::CPlayer_Hp_Eye(const CPlayer_Hp_Eye& Prototype)
    : CUIObject{ Prototype }
{
}

HRESULT CPlayer_Hp_Eye::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CPlayer_Hp_Eye::Initialize(void* pArg)
{
    UIOBJECT_DESC desc{};
    desc.fSizeX = 30.f;     // 지름(픽셀처럼 쓰는 UI 크기)
    desc.fSizeY = 30.f;
    desc.fX = 140.f;    // 화면 좌표(이미 맞춰둔 값)
    desc.fY = 630.f;

    if (pArg)
        desc = *reinterpret_cast<UIOBJECT_DESC*>(pArg);

    if (FAILED(__super::Initialize(&desc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CPlayer_Hp_Eye::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CPlayer_Hp_Eye::Update(_float fTimeDelta)
{
    m_Time += fTimeDelta;

    const int played = (int)(m_Time * m_FPS);
    const int frame = m_Start + (m_Count > 0 ? (played % m_Count) : 0);

    const int col = frame % m_Cols;
    const int row = frame / m_Cols;

    // 셀 스케일/오프셋
    const _float2 cell = _float2(1.f / m_Cols, 1.f / m_Rows);
    const _float2 uvOffset = _float2(col * cell.x, row * cell.y);

    // 반 텍셀 패딩(512 기준) ? 경계 블리딩 방지
    const float   pad = 0.5f / 512.f;
    const _float2 scale = _float2(cell.x - 2.f * pad, cell.y - 2.f * pad);
    const _float2 offs = _float2(uvOffset.x + pad, uvOffset.y + pad);

    // 저장만 해두고, Render에서 바인딩
    m_UVScale = scale;
    m_UVOffset = offs;
}


void CPlayer_Hp_Eye::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this)))
        return;
}

HRESULT CPlayer_Hp_Eye::Render()
{
    __super::Begin();

    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix"))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix)))            return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix)))            return E_FAIL;

    // ★ UVScale/UVOffset 바인딩을 Render에서 수행
    if (FAILED(m_pShaderCom->Bind_RawValue("g_UVScale", &m_UVScale, sizeof(_float2)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_UVOffset", &m_UVOffset, sizeof(_float2)))) return E_FAIL;

    if (FAILED(m_pTextureCom->Bind_Shader_Resource(m_pShaderCom, "g_Texture", 0)))   return E_FAIL;

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Bind_Resources();
    m_pVIBufferCom->Render();

    return S_OK;
}


HRESULT CPlayer_Hp_Eye::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_Eye"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Texture_HP_Bar_Eye"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom), nullptr)))
        return E_FAIL;




    return S_OK;
}

CPlayer_Hp_Eye* CPlayer_Hp_Eye::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CPlayer_Hp_Eye* pInstance = new CPlayer_Hp_Eye(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CPlayer_Hp_Eye"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CPlayer_Hp_Eye::Clone(void* pArg)
{
    CPlayer_Hp_Eye* pInstance = new CPlayer_Hp_Eye(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CPlayer_Hp_Eye"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CPlayer_Hp_Eye::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}
