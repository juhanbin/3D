#include "Monster.h"
#include "GameInstance.h"

CMonster::CMonster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    :CGameObject{ pDevice,pContext }
{
}

CMonster::CMonster(const CMonster& Prototype)
    :CGameObject{ Prototype }
    , m_eType(Prototype.m_eType)
{
}

HRESULT CMonster::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CMonster::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (pArg)
    {
        MONSTER_DESC* pDesc = static_cast<MONSTER_DESC*>(pArg);
        m_eType = pDesc->type;
        //크기
        XMMATRIX matScale = XMMatrixScaling(pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z);

        //회전
        XMMATRIX matRot = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(pDesc->vRot.x),
            XMConvertToRadians(pDesc->vRot.y),
            XMConvertToRadians(pDesc->vRot.z));

        //위치
        XMMATRIX matTrans = XMMatrixTranslation(pDesc->vPos.x, pDesc->vPos.y, pDesc->vPos.z);

        XMMATRIX matWorld = matScale * matRot * matTrans;

        XMFLOAT4X4 matWorld4x4;
        XMStoreFloat4x4(&matWorld4x4, matWorld);

        m_pTransformCom->Set_State(STATE::RIGHT, XMLoadFloat4((XMFLOAT4*)&matWorld4x4.m[0]));
        m_pTransformCom->Set_State(STATE::UP, XMLoadFloat4((XMFLOAT4*)&matWorld4x4.m[1]));
        m_pTransformCom->Set_State(STATE::LOOK, XMLoadFloat4((XMFLOAT4*)&matWorld4x4.m[2]));
        m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat4((XMFLOAT4*)&matWorld4x4.m[3]));

        char szDbg[256];
        sprintf_s(szDbg, sizeof(szDbg), "type=%d scale=(%.2f,%.2f,%.2f) rot=(%.2f,%.2f,%.2f) pos=(%.2f,%.2f,%.2f)\n",
            (int)m_eType,
            pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z,
            pDesc->vRot.x, pDesc->vRot.y, pDesc->vRot.z,
            pDesc->vPos.x, pDesc->vPos.y, pDesc->vPos.z);
        OutputDebugStringA(szDbg);
    }


    if (FAILED(Ready_Components()))
        return E_FAIL;

    m_pModelCom->Set_Animation(2, true);

    //_float3 test = { 0.000000001f, 0.000000001f, 0.000000001f };
    _float3 test = { 1.f, 1.f, 1.f };
    m_pTransformCom->Scaling(test);
    return S_OK;
}

void CMonster::Priority_Update(_float fTimeDelta)
{
}

void CMonster::Update(_float fTimeDelta)
{
    m_pModelCom->Play_Animation(fTimeDelta);
}

void CMonster::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;
}

HRESULT CMonster::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint           iNumMeshes = m_pModelCom->Get_NumMeshes();

    if (iNumMeshes == 0)
    {
        OutputDebugStringA("Monster Model의 Mesh가 0개입니다!\n");
        return E_FAIL;
    }

    for (size_t i = 0; i < iNumMeshes; i++)
    {
        if (FAILED(m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, ENUM_CLASS(TextureType::DIFFUSE), 0))) // 0: DIFFUSE
        {
            OutputDebugStringA("dif머티리얼 바인딩 실패!\n");
            return E_FAIL;
        }

        //if (FAILED(m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_NormalTexture", i, ENUM_CLASS(TextureType::NORMAL), 0))) // 1: NORMAL
        //{
        //    OutputDebugStringA("n머티리얼 바인딩 실패!\n");
        //    return E_FAIL;
        //}

        if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
            return E_FAIL;

        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }

    return S_OK;
}

HRESULT CMonster::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Monster"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;

    OutputDebugStringA("CMonster: Ready_Components 정상 종료\n");
    return S_OK;
}

HRESULT CMonster::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
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

CMonster* CMonster::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMonster* pInstance = new CMonster(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CMonster"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CMonster::Clone(void* pArg)
{
    CMonster* pInstance = new CMonster(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CMonster"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMonster::Free()
{
    __super::Free();

    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
