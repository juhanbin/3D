#include "MapObject_Boss.h"
#include "GameInstance.h"

USING(Client)

CMapObject_Boss::CMapObject_Boss(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{
}

CMapObject_Boss::CMapObject_Boss(const CMapObject_Boss& Prototype)
    : CGameObject{ Prototype }
    , m_eType(Prototype.m_eType)
{
}

HRESULT CMapObject_Boss::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CMapObject_Boss::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (pArg)
    {
        MAPOBJECT_DESC* pDesc = static_cast<MAPOBJECT_DESC*>(pArg);
        m_eType = pDesc->type;

        // 행렬 변환
        XMMATRIX matScale = XMMatrixScaling(pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z);
        XMMATRIX matRot = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(pDesc->vRot.x),
            XMConvertToRadians(pDesc->vRot.y),
            XMConvertToRadians(pDesc->vRot.z));

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

    return S_OK;
}

void CMapObject_Boss::Priority_Update(_float fTimeDelta) {}
void CMapObject_Boss::Update(_float fTimeDelta) {}
void CMapObject_Boss::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;


}

HRESULT CMapObject_Boss::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (size_t i = 0; i < iNumMeshes; i++)
    {
        // BIN 데이터니까 직접 type 인덱스(0=Diffuse, 1=Normal)로 바인딩
        if (FAILED(m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, 0, 0))) // 0: DIFFUSE
        {
            OutputDebugStringA("d머티리얼 바인딩 실패!\n");
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

HRESULT CMapObject_Boss::Ready_Components()
{
    // 쉐이더 추가
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::BOSS),
        TEXT("Prototype_Component_Shader_VtxMesh"),
        TEXT("Com_Shader"),
        reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
    {
        OutputDebugStringA("MapObject: 쉐이더 Add_Component 실패!\n");
        return E_FAIL;
    }

    // 타입 분기별로 모델 할당
    const wchar_t* modelProto = nullptr;
    switch (m_eType)
    {
    case EObjectType::MOD_BOSSROOM_CEILING_AA:
        modelProto = TEXT("Prototype_Component_Model_MOD_BossRoom_Ceiling_AA");
        break;
    case EObjectType::MOD_BOSSROOM_GROUND_AA:
        modelProto = TEXT("Prototype_Component_Model_MOD_BossRoom_Ground_AA");
        break;
    case EObjectType::MOD_BOSSROOM_GROUNDFENCE_AA:
        modelProto = TEXT("Prototype_Component_Model_MOD_BossRoom_GroundFence_AA");
        break;
    case EObjectType::MOD_BOSSROOM_GROUNDFENCE_AB:
        modelProto = TEXT("Prototype_Component_Model_MOD_BossRoom_GroundFence_AB");
        break;
    case EObjectType::MOD_BOSSROOM_PILLAR_AA:
        modelProto = TEXT("Prototype_Component_Model_MOD_BossRoom_Pillar_AA");
        break;
    case EObjectType::MOD_BOSSROOM_PILLAR_AB:
        modelProto = TEXT("Prototype_Component_Model_MOD_BossRoom_Pillar_AB");
        break;
    case EObjectType::MOD_BOSSROOM_WALL_AA:
        modelProto = TEXT("Prototype_Component_Model_MOD_BossRoom_Wall_AA");
        break;
    case EObjectType::MOD_BOSSROOM_WALL_AB:
        modelProto = TEXT("Prototype_Component_Model_MOD_BossRoom_Wall_AB");
        break;
    case EObjectType::MOD_BOSSROOM_WALL_AC:
        modelProto = TEXT("Prototype_Component_Model_MOD_BossRoom_Wall_AC");
        break;
    case EObjectType::MOD_GUARDRAIL_AB:
        modelProto = TEXT("Prototype_Component_Model_MOD_guardrail_AB");
        break;
    default:
        OutputDebugStringA("Unknown EObjectType in Ready_Components!\n");
        return E_FAIL;
    }

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::BOSS),
        modelProto,
        TEXT("Com_Model"),
        reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
    {
        OutputDebugStringA("MapObject: 모델 Add_Component 실패!\n");
        return E_FAIL;
    }

    OutputDebugStringA("MapObject: Ready_Components 정상 종료\n");
    return S_OK;
}


HRESULT CMapObject_Boss::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;

    /*const LIGHT_DESC* pLightDesc = m_pGameInstance->Get_LightDesc(0);
    if (!pLightDesc)
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
        return E_FAIL;*/

    return S_OK;
}

CMapObject_Boss* CMapObject_Boss::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMapObject_Boss* pInstance = new CMapObject_Boss(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CMapObject_Boss"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CMapObject_Boss::Clone(void* pArg)
{
    CMapObject_Boss* pInstance = new CMapObject_Boss(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CMapObject_Boss"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMapObject_Boss::Free()
{
    __super::Free();
    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
