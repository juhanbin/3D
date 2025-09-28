// Boss_Fill.cpp
#include "Boss_Fill.h"
#include "GameInstance.h"
#include <algorithm> // std::clamp

USING(Client)

// ---------- G/H 디버그 (콜백 없을 때만) ----------
namespace BossUI_Debug
{
    inline float& HP() { static float v = 100.f; return v; }
    inline float& MaxHP() { static float v = 100.f; return v; }

    inline float Ratio() { return (MaxHP() > 0.f) ? (HP() / MaxHP()) : 0.f; }

    inline void Tick(Engine::CGameInstance* gi, float dtSec)
    {
        if (!gi) return;
        const float step = 0.05f * MaxHP();          // tap 5%
        const float flow = 0.40f * MaxHP() * dtSec;  // hold 40%/s

        if (gi->KeyDown(DIK_E)) HP() = max(0.f, HP() - step);
        if (gi->KeyDown(DIK_H)) HP() = std::min(MaxHP(), HP() + step);

        if (gi->KeyPressing(DIK_E)) HP() = max(0.f, HP() - flow);
        if (gi->KeyPressing(DIK_H)) HP() = std::min(MaxHP(), HP() + flow);
    }
}
// ---------------------------------------------------

CBoss_Fill::CBoss_Fill(ID3D11Device* d, ID3D11DeviceContext* c) : CUIObject(d, c) {}
CBoss_Fill::CBoss_Fill(const CBoss_Fill& rhs) : CUIObject(rhs) {}
HRESULT CBoss_Fill::Initialize_Prototype() { return S_OK; }

HRESULT CBoss_Fill::Initialize(void* pArg)
{
    // 원작 느낌: 상단 중앙, 얇고 짧게
    UIOBJECT_DESC desc{};
    desc.fSizeX = (float)g_iWinSizeX / 3.f + 10.f;
    desc.fSizeY = 20.f;
    desc.fX = (float)g_iWinSizeX * 0.5f;
    desc.fY = 120.f;

    if (FAILED(__super::Initialize(&desc))) return E_FAIL;
    if (FAILED(Ready_Components()))         return E_FAIL;

    if (pArg) {
        auto* d = (DESC*)pArg;
        m_fnGetRatio = d->fnGetRatio;
        m_Edge = d->edgeSoft;
        m_MainColor = d->mainColor;
        m_EmptyColor = d->emptyColor;
    }
    return S_OK;
}

void CBoss_Fill::Update(_float dt)
{
    const float dtSec = (dt > 5.f) ? (dt * 0.001f) : dt;

    float ratio = 1.f;
    if (m_fnGetRatio)  ratio = Clamp(m_fnGetRatio(), 0.f, 1.f);
    else { // 디버그 입력
        BossUI_Debug::Tick(m_pGameInstance, dtSec);
        ratio = Clamp(BossUI_Debug::Ratio(), 0.f, 1.f);
    }
    m_Fill = ratio;
}

void CBoss_Fill::Late_Update(_float) {
    m_pGameInstance->Add_RenderGroup(RENDERGROUP::UI, this);
}

HRESULT CBoss_Fill::Render()
{
    __super::Begin();

    // 행렬
    m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix");
    m_pShaderCom->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix);
    m_pShaderCom->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix);

    // 텍스처(t0)
    m_pMaskTex->Bind_Shader_Resource(m_pShaderCom, "g_Tex", 0);

    // 파라미터 (메인=1)
    int   mode = 1;
    float fill = Clamp(m_Fill, 0.f, 1.f);

    m_pShaderCom->Bind_RawValue("g_Mode", &mode, sizeof(int));
    m_pShaderCom->Bind_RawValue("g_Fill", &fill, sizeof(float));
    m_pShaderCom->Bind_RawValue("g_EdgeSoft", &m_Edge, sizeof(float));
    m_pShaderCom->Bind_RawValue("g_Color", &m_MainColor, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_EmptyColor", &m_EmptyColor, sizeof(_float4));

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Bind_Resources();
    m_pVIBufferCom->Render();
    return S_OK;
}

HRESULT CBoss_Fill::Ready_Components()
{
    // HPBar_OneTex.hlsl 연결된 셰이더 키 사용
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC),
        TEXT("Prototype_Component_Shader_Boss_HP"),
        TEXT("Com_Shader"),
        reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC),
        TEXT("Prototype_Component_VIBuffer_Rect"),
        TEXT("Com_VIBuffer"),
        reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    // 현재 보유한 단일 텍스처 키(예: MyHeight.bmp가 매핑된 키)
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC),
        TEXT("Prototype_Component_Texture_HP_Bar"),
        TEXT("Com_Tex"),
        reinterpret_cast<CComponent**>(&m_pMaskTex), nullptr)))
        return E_FAIL;

    return S_OK;
}

CBoss_Fill* CBoss_Fill::Create(ID3D11Device* d, ID3D11DeviceContext* c)
{
    auto* p = new CBoss_Fill(d, c);
    if (FAILED(p->Initialize_Prototype())) { Safe_Release(p); return nullptr; }
    return p;
}
CGameObject* CBoss_Fill::Clone(void* pArg)
{
    auto* p = new CBoss_Fill(*this);
    if (FAILED(p->Initialize(pArg))) { Safe_Release(p); return nullptr; }
    return p;
}
void CBoss_Fill::Free()
{
    __super::Free();
    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pMaskTex);
    Safe_Release(m_pShaderCom);
}
