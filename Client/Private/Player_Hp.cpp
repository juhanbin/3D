#include "Player_Hp.h"
#include "GameInstance.h"
#include "PlayerManager.h"

USING(Client)

CPlayer_Hp::CPlayer_Hp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext } {
}

CPlayer_Hp::CPlayer_Hp(const CPlayer_Hp& Prototype)
    : CUIObject{ Prototype } {
}

HRESULT CPlayer_Hp::Initialize_Prototype() { return S_OK; }

HRESULT CPlayer_Hp::Initialize(void* pArg)
{
    const _float screenW = (_float)g_iWinSizeX; // 1280
    const _float screenH = (_float)g_iWinSizeY; // 720

    // 고정 배치(네가 쓰던 값)
    const _float width = 520.f;
    const _float height = 18.f;
    const _float left = 90.f;
    const _float bottom = 70.f;

    // 미세 위치 보정
    const _float offsetLeft = -30.f;
    const _float offsetUp = 12.f;

    UIOBJECT_DESC Desc{};
    Desc.fSizeX = width - 210.f;
    Desc.fSizeY = height - 10.f;

    const _float cx = (left + offsetLeft) + width * 0.5f;
    const _float cy = screenH - ((bottom + offsetUp) + height * 0.5f);
    Desc.fX = roundf(cx);
    Desc.fY = roundf(cy);

    if (FAILED(__super::Initialize(&Desc)))   return E_FAIL;
    if (FAILED(Ready_Components()))           return E_FAIL;

    // 시작 시 PM에서 초기 채움비율 설정
    if (auto* pm = CPlayerManager::GetInstance())
    {
        const float maxhp = max(pm->GetActiveMaxHP(), 1e-3f);
        const float hp = pm->GetActiveHP();
        m_TargetFill = m_VisualFill = Clamp01(hp / maxhp);
    }

    return S_OK;
}

void CPlayer_Hp::Priority_Update(_float /*fTimeDelta*/)
{
    // 필요 시 우선 업데이트 로직
}

void CPlayer_Hp::Update(_float fTimeDelta)
{
    // ★ 입력/조작 없음 ? PM에서 읽기만
    if (auto* pm = CPlayerManager::GetInstance())
    {
        const float maxhp = max(pm->GetActiveMaxHP(), 1e-3f);
        const float hp = pm->GetActiveHP();
        m_TargetFill = Clamp01(hp / maxhp);
    }

    // 화면 보간(지수 감쇠)
    const float k = 10.f; // 커질수록 빠름
    m_VisualFill += (m_TargetFill - m_VisualFill) * (1.f - expf(-k * fTimeDelta));
}

void CPlayer_Hp::Late_Update(_float /*fTimeDelta*/)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this)))
        return;
}

HRESULT CPlayer_Hp::Render()
{
    __super::Begin();

    // 행렬 상수
    m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix");
    m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix);
    m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix);

    // 마스크 텍스처 바인딩 (임시로 프레임/바 텍스처 사용 중)
    if (FAILED(m_pTextureCom->Bind_Shader_Resource(m_pShaderCom, "g_Mask", 0)))
        return E_FAIL;

    // 셰이더 파라미터
    const float fill = m_VisualFill; // 0~1
    m_pShaderCom->Bind_RawValue("g_Fill", &fill, sizeof(float));
    m_pShaderCom->Bind_RawValue("g_EdgeSoft", &m_EdgeSoft, sizeof(float));
    m_pShaderCom->Bind_RawValue("g_FillL", &m_FillL, sizeof(m_FillL));
    m_pShaderCom->Bind_RawValue("g_FillR", &m_FillR, sizeof(m_FillR));
    m_pShaderCom->Bind_RawValue("g_EmptyL", &m_EmptyL, sizeof(m_EmptyL));
    m_pShaderCom->Bind_RawValue("g_EmptyR", &m_EmptyR, sizeof(m_EmptyR));

    // 드로우
    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Bind_Resources();
    m_pVIBufferCom->Render();

    return S_OK;
}

HRESULT CPlayer_Hp::Ready_Components()
{
    // HP바 전용 셰이더(마스크 기반 색 채움)
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_HP"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    // 임시 마스크 텍스처 (전용 마스크로 교체 권장)
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Texture_HP_Bar"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom), nullptr)))
        return E_FAIL;

    return S_OK;
}

CPlayer_Hp* CPlayer_Hp::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CPlayer_Hp* pInstance = new CPlayer_Hp(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CPlayer_Hp"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CPlayer_Hp::Clone(void* pArg)
{
    CPlayer_Hp* pInstance = new CPlayer_Hp(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CPlayer_Hp"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CPlayer_Hp::Free()
{
    __super::Free();
    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}
