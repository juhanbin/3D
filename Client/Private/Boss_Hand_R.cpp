#include "Boss_Hand_R.h"
#include "GameInstance.h"

using namespace Client;
using namespace DirectX;

CBoss_Hand_R::CBoss_Hand_R(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext } {
}

CBoss_Hand_R::CBoss_Hand_R(const CBoss_Hand_R& Prototype)
    : CGameObject{ Prototype } {
}

HRESULT CBoss_Hand_R::Initialize_Prototype() { return S_OK; }

HRESULT CBoss_Hand_R::Initialize(void* pArg)
{
    GAMEOBJECT_DESC Desc{};
    Desc.fSpeedPerSec = 0.f;
    Desc.fRotationPerSec = 0.f;
    if (FAILED(__super::Initialize(&Desc))) return E_FAIL;

    if (pArg)
    {
        auto* pDesc = static_cast<Boss_Hand_R_DESC*>(pArg);
        m_eType = pDesc->type;

        const XMMATRIX S = XMMatrixScaling(pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z);
        const XMMATRIX R = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(pDesc->vRot.x),
            XMConvertToRadians(pDesc->vRot.y),
            XMConvertToRadians(pDesc->vRot.z));
        const XMMATRIX T = XMMatrixTranslation(pDesc->vPos.x, pDesc->vPos.y, pDesc->vPos.z);
        XMFLOAT4X4 W; XMStoreFloat4x4(&W, S * R * T);

        m_pTransformCom->Set_State(Engine::STATE::RIGHT, XMLoadFloat4((XMFLOAT4*)&W.m[0]));
        m_pTransformCom->Set_State(Engine::STATE::UP, XMLoadFloat4((XMFLOAT4*)&W.m[1]));
        m_pTransformCom->Set_State(Engine::STATE::LOOK, XMLoadFloat4((XMFLOAT4*)&W.m[2]));
        m_pTransformCom->Set_State(Engine::STATE::POSITION, XMLoadFloat4((XMFLOAT4*)&W.m[3]));
    }

    if (FAILED(Ready_Components())) return E_FAIL;

    m_pModelCom->Set_Animation(10, true);
    return S_OK;
}

void CBoss_Hand_R::Priority_Update(_float) {}

void CBoss_Hand_R::Update(_float dt)
{
    m_pModelCom->Play_Animation(dt);
}

void CBoss_Hand_R::Late_Update(_float)
{
    m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this);
}

HRESULT CBoss_Hand_R::Render()
{
    if (FAILED(Bind_ShaderResources())) return E_FAIL;

    const _uint n = m_pModelCom->Get_NumMeshes();
    for (_uint i = 0; i < n; ++i)
    {
        if (FAILED(m_pModelCom->Bind_Materials_Bin(
            m_pShaderCom, "g_DiffuseTexture", i, ENUM_CLASS(TextureType::DIFFUSE), 0)))
            return E_FAIL;

        if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
            return E_FAIL;

        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }

    return S_OK;
}

HRESULT CBoss_Hand_R::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Boss_hand_R"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;

    return S_OK;
}

HRESULT CBoss_Hand_R::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix"))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ)))) return E_FAIL;

    const LIGHT_DESC* L = m_pGameInstance->Get_LightDesc(0);
    if (!L) return E_FAIL;
    m_pShaderCom->Bind_RawValue("g_vLightDir", &L->vDirection, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &L->vDiffuse, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vLightAmbient", &L->vAmbient, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vLightSpecular", &L->vSpecular, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4));
    return S_OK;
}

CBoss_Hand_R* CBoss_Hand_R::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    auto* p = new CBoss_Hand_R(pDevice, pContext);
    if (FAILED(p->Initialize_Prototype())) { MSG_BOX(TEXT("Failed to Created : CBoss_Hand_R")); Safe_Release(p); }
    return p;
}

CGameObject* CBoss_Hand_R::Clone(void* pArg)
{
    auto* p = new CBoss_Hand_R(*this);
    if (FAILED(p->Initialize(pArg))) { MSG_BOX(TEXT("Failed to Clone : CBoss_Hand_R")); Safe_Release(p); }
    return p;
}

void CBoss_Hand_R::Free()
{
    __super::Free();
    Safe_Release(m_pCollider);
    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
