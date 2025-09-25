#include "Terrain_Intro.h"
#include "GameInstance.h"

CTerrain_Intro::CTerrain_Intro(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{
}

CTerrain_Intro::CTerrain_Intro(const CTerrain_Intro& Prototype)
    : CGameObject{ Prototype }
{
}

HRESULT CTerrain_Intro::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CTerrain_Intro::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CTerrain_Intro::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CTerrain_Intro::Update(_float fTimeDelta)
{
    m_pNavigationCom->Update(m_pTransformCom->Get_WorldMatrix());

}

void CTerrain_Intro::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;

#ifdef _DEBUG
    if (FAILED(m_pGameInstance->Add_DebugComponent(m_pNavigationCom)))
        return;


#endif
}

HRESULT CTerrain_Intro::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    m_pShaderCom->Begin(0);

    m_pVIBufferCom->Bind_Resources();

    m_pVIBufferCom->Render();



    return S_OK;
}

HRESULT CTerrain_Intro::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::INTRO), TEXT("Prototype_Component_Shader_VtxNorTex_Blood"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    /*if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::INTRO), TEXT("Prototype_Component_VIBuffer_Terrain"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;*/

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::INTRO), TEXT("Prototype_Component_VIBuffer_Terrain_Flat"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::INTRO), TEXT("Prototype_Component_Texture_Terrain"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom[TEXTURE_DIFFUSE]), nullptr)))
        return E_FAIL;

    /*if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Texture_Mask_Terrain"),
        TEXT("Com_Texture_Mask"), reinterpret_cast<CComponent**>(&m_pTextureCom[TEXTURE_MASK]), nullptr)))
        return E_FAIL;*/

    /*if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Texture_Brush"),
        TEXT("Com_Texture_Brush"), reinterpret_cast<CComponent**>(&m_pTextureCom[TEXTURE_BRUSH]), nullptr)))
        return E_FAIL;*/

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::INTRO), TEXT("Prototype_Component_Navigation_Intro"),
        TEXT("Com_Navigation"), reinterpret_cast<CComponent**>(&m_pNavigationCom), nullptr)))
        return E_FAIL;

    return S_OK;
}

HRESULT CTerrain_Intro::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;

    if (FAILED(m_pTextureCom[TEXTURE_DIFFUSE]->Bind_Shader_Resources(m_pShaderCom, "g_DiffuseTexture")))
        return E_FAIL;

    return S_OK;
}

CTerrain_Intro* CTerrain_Intro::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTerrain_Intro* pInstance = new CTerrain_Intro(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CTerrain_Intro"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CTerrain_Intro::Clone(void* pArg)
{
    CTerrain_Intro* pInstance = new CTerrain_Intro(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CTerrain_Intro"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTerrain_Intro::Free()
{
    __super::Free();

    Safe_Release(m_pNavigationCom);
    Safe_Release(m_pVIBufferCom);

    for (auto& pTexture : m_pTextureCom)
        Safe_Release(pTexture);

    Safe_Release(m_pShaderCom);
}
