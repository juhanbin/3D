#include "Weapon_Skeleton_Bow.h"
#include "GameInstance.h"

CWeapon_Skeleton_Bow::CWeapon_Skeleton_Bow(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CPartObject{ pDevice, pContext }
{

}

CWeapon_Skeleton_Bow::CWeapon_Skeleton_Bow(const CWeapon_Skeleton_Bow& Prototype)
    : CPartObject{ Prototype }
{

}

HRESULT CWeapon_Skeleton_Bow::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CWeapon_Skeleton_Bow::Initialize(void* pArg)
{
    WEAPON_DESC* pDesc = static_cast<WEAPON_DESC*>(pArg);
    m_pParentState = pDesc->pState;
    m_pSocketMatrix = pDesc->pSocketMatrix;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    //m_pTransformCom->Scaling(_float3(0.1f, 0.1f, 0.1f));
    //m_pTransformCom->Set_State(STATE::POSITION, XMVectorSet(0.f, 0.f, 0.f, 1.f));

    //m_pTransformCom->Set_State(STATE::POSITION, XMVectorSet(0.f, -1.f, 0.2f, 1.f));
    m_pTransformCom->Rotation(120.f, -90.f, -90.f);
    return S_OK;
}

void CWeapon_Skeleton_Bow::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CWeapon_Skeleton_Bow::Update(_float fTimeDelta)
{



    _matrix     BoneMatrix = XMLoadFloat4x4(m_pSocketMatrix);

    for (size_t i = 0; i < 3; i++)
        BoneMatrix.r[i] = XMVector3Normalize(BoneMatrix.r[i]);

    XMStoreFloat4x4(&m_CombinedWorldMatrix,
        m_pTransformCom->Get_WorldMatrix() *
        BoneMatrix *
        XMLoadFloat4x4(m_pParentMatrix));
}


void CWeapon_Skeleton_Bow::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;
}

HRESULT CWeapon_Skeleton_Bow::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint           iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (size_t i = 0; i < iNumMeshes; i++)
    {
        if (FAILED(m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, ENUM_CLASS(TextureType::DIFFUSE), 0))) // 0: DIFFUSE
        {
            //OutputDebugStringA("Spear_머티리얼 바인딩 실패!\n");
            return E_FAIL;
        }

        if (FAILED(m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_NormalTexture", i, 1, 0))) // 1: NORMAL
        {
            OutputDebugStringA("n머티리얼 바인딩 실패!\n");
            return E_FAIL;
        }

        m_pShaderCom->Begin(0);

        m_pModelCom->Render(i);
    }

    return S_OK;
}

HRESULT CWeapon_Skeleton_Bow::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Monster_Bow"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;

    return S_OK;
}

HRESULT CWeapon_Skeleton_Bow::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_CombinedWorldMatrix)))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;

    const LIGHT_DESC* pLightDesc = m_pGameInstance->Get_LightDesc(0);
    if (nullptr == pLightDesc)
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDir", &pLightDesc->vDirection, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &pLightDesc->vDiffuse, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightAmbient", &pLightDesc->vAmbient, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightSpecular", &pLightDesc->vSpecular, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4))))
        return E_FAIL;

    return S_OK;
}

CWeapon_Skeleton_Bow* CWeapon_Skeleton_Bow::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CWeapon_Skeleton_Bow* pInstance = new CWeapon_Skeleton_Bow(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CWeapon_Skeleton_Bow"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CWeapon_Skeleton_Bow::Clone(void* pArg)
{
    CWeapon_Skeleton_Bow* pInstance = new CWeapon_Skeleton_Bow(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Clone : CWeapon_Skeleton_Bow"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CWeapon_Skeleton_Bow::Free()
{
    __super::Free();

    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
