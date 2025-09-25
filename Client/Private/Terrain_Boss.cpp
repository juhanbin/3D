#include "Terrain_Boss.h"
#include "GameInstance.h"

CTerrain_Boss::CTerrain_Boss(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext } {
}

CTerrain_Boss::CTerrain_Boss(const CTerrain_Boss& Prototype)
    : CGameObject{ Prototype } {
}

HRESULT CTerrain_Boss::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CTerrain_Boss::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))         return E_FAIL;
    if (FAILED(Ready_Components()))                return E_FAIL;

    // 필요시 약간 아래로
    m_pTransformCom->Translate(XMVectorSet(0.f, -0.1f, 0.f, 0.f));
    return S_OK;
}

void CTerrain_Boss::Priority_Update(_float /*fTimeDelta*/) {}

void CTerrain_Boss::Update(_float /*fTimeDelta*/)
{
    // 네비는 Terrain 기준 월드 행렬로만 업데이트한다(월드 고정!)
    if (m_pNavigationCom)
        m_pNavigationCom->Update(m_pTransformCom->Get_WorldMatrix());
}

void CTerrain_Boss::Late_Update(_float /*fTimeDelta*/)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;

#ifdef _DEBUG
    if (m_pNavigationCom)
        m_pGameInstance->Add_DebugComponent(m_pNavigationCom);
#endif
}

HRESULT CTerrain_Boss::Render()
{
    if (FAILED(Bind_ShaderResources()))            return E_FAIL;

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Bind_Resources();
    m_pVIBufferCom->Render();
    return S_OK;
}

HRESULT CTerrain_Boss::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_Component_Shader_VtxNorTex_Blood"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_Component_VIBuffer_Terrain_Flat"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_Component_Texture_Terrain"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom[TEXTURE_DIFFUSE]), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_Component_Navigation_Boss"),
        TEXT("Com_Navigation"), reinterpret_cast<CComponent**>(&m_pNavigationCom), nullptr)))
        return E_FAIL;

    return S_OK;
}

HRESULT CTerrain_Boss::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))   return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))  return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))  return E_FAIL;
    if (FAILED(m_pTextureCom[TEXTURE_DIFFUSE]->Bind_Shader_Resources(m_pShaderCom, "g_DiffuseTexture")))          return E_FAIL;
    return S_OK;
}

CTerrain_Boss* CTerrain_Boss::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTerrain_Boss* pInstance = new CTerrain_Boss(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CTerrain_Boss"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CTerrain_Boss::Clone(void* pArg)
{
    CTerrain_Boss* pInstance = new CTerrain_Boss(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CTerrain_Boss"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CTerrain_Boss::Free()
{
    __super::Free();

    Safe_Release(m_pNavigationCom);   // Terrain이 소유하므로 정상 해제
    Safe_Release(m_pVIBufferCom);
    for (auto& pTexture : m_pTextureCom) Safe_Release(pTexture);
    Safe_Release(m_pShaderCom);
}
