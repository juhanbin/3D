#include "Sky.h"
#include "GameInstance.h"

CSky::CSky(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{
    /*m_pGraphic_Device->SetRenderState(, );*/

    /*ID3D11RasterizerState* pRSState = { nullptr };
    ID3D11DepthStencilState* pRSState = { nullptr };
    ID3D11BlendState**/
    // D3D11_DEPTH_STENCIL_DESC
  //  D3D11_RASTERIZER_DESC

     /*D3D11_RASTERIZER_DESC   RSDesc{};
     m_pDevice->CreateRasterizerState(RSDesc, &pRSState);

     m_pContext->RSSetState(pRSState);
     m_pContext->OMSetBlendState();
     */
     // m_pContext->OMSetDepthStencilState();

      //D3D11_BLEND_DESC

      // m_pContext->OMSetBlendState(, , )

}

CSky::CSky(const CSky& Prototype)
    : CGameObject{ Prototype }
{
}

HRESULT CSky::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CSky::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CSky::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CSky::Update(_float fTimeDelta)
{
    m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat4(m_pGameInstance->Get_CamPosition()));

}

void CSky::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::PRIORITY, this)))
        return;
}

HRESULT CSky::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    m_pShaderCom->Begin(0);

    m_pVIBufferCom->Bind_Resources();

    m_pVIBufferCom->Render();

    return S_OK;
}

HRESULT CSky::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::BRIDGE), TEXT("Prototype_Component_Shader_VtxCube"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::BRIDGE), TEXT("Prototype_Component_VIBuffer_Cube"),
        TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::BRIDGE), TEXT("Prototype_Component_Texture_Sky_Bridge"),
        TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom), nullptr)))
        return E_FAIL;


    return S_OK;
}

HRESULT CSky::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;

    if (FAILED(m_pTextureCom->Bind_Shader_Resource(m_pShaderCom, "g_Texture", 3)))
        return E_FAIL;

    return S_OK;
}

CSky* CSky::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CSky* pInstance = new CSky(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CSky"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CSky::Clone(void* pArg)
{
    CSky* pInstance = new CSky(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CSky"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CSky::Free()
{
    __super::Free();

    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pTextureCom);
    Safe_Release(m_pShaderCom);
}
