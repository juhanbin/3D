#include "MapObject.h"
#include "GameInstance.h"

using namespace DirectX;
NS_BEGIN(Edit)

std::vector<CMapObject*> CMapObject::s_All;

CMapObject::CMapObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice,pContext } {
}

CMapObject::CMapObject(const CMapObject& Prototype)
    : CGameObject{ Prototype }, m_eType(Prototype.m_eType), m_LocalBox(Prototype.m_LocalBox) {
}

HRESULT CMapObject::Initialize_Prototype() { return S_OK; }

HRESULT CMapObject::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg))) return E_FAIL;

    if (pArg)
    {
        auto* pDesc = static_cast<MAPOBJECT_DESC*>(pArg);
        m_eType = pDesc->type;

        XMMATRIX S = XMMatrixScaling(pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z);
        XMMATRIX R = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(pDesc->vRot.x),
            XMConvertToRadians(pDesc->vRot.y),
            XMConvertToRadians(pDesc->vRot.z));
        XMMATRIX T = XMMatrixTranslation(pDesc->vPos.x, pDesc->vPos.y, pDesc->vPos.z);

        XMMATRIX W = S * R * T;
        XMFLOAT4X4 W4; XMStoreFloat4x4(&W4, W);
        m_pTransformCom->Set_State(STATE::RIGHT, XMLoadFloat4((XMFLOAT4*)&W4.m[0]));
        m_pTransformCom->Set_State(STATE::UP, XMLoadFloat4((XMFLOAT4*)&W4.m[1]));
        m_pTransformCom->Set_State(STATE::LOOK, XMLoadFloat4((XMFLOAT4*)&W4.m[2]));
        m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat4((XMFLOAT4*)&W4.m[3]));
    }

    if (FAILED(Ready_Components())) return E_FAIL;

    InitBoundingBoxFromModel();
    if (m_LocalBox.Extents.x == 0 && m_LocalBox.Extents.y == 0 && m_LocalBox.Extents.z == 0) {
        // 임시로 큰 박스 넣어보기
        m_LocalBox = DirectX::BoundingBox({ 0,0,0 }, { 10,10,10 });
    }

    // 모델 기준 로컬 AABB 계산
    InitBoundingBoxFromModel();

    if (m_eType == EObjectType::MONSTER) m_pModelCom->Set_Animation(2, true);
    if (m_eType == EObjectType::HERO)    m_pModelCom->Set_Animation(3, true);
    if (m_eType == EObjectType::SPEAR)   m_pModelCom->Set_Animation(0, true);
    if (m_eType == EObjectType::SKELETON_SPEAR) m_pModelCom->Set_Animation(1, true);
    if (m_eType == EObjectType::SKELETON_BOW) m_pModelCom->Set_Animation(0, true);
    if (m_eType == EObjectType::BOSS_HAND_L) m_pModelCom->Set_Animation(0, true);
    if (m_eType == EObjectType::BOSS_HAND_R) m_pModelCom->Set_Animation(0, true);
    if (m_eType == EObjectType::EYESPAWNER) m_pModelCom->Set_Animation(0, true);
    if (m_eType == EObjectType::MONSTER_EYE) m_pModelCom->Set_Animation(0, true);
    if (m_eType == EObjectType::MUSHROOM) m_pModelCom->Set_Animation(0, true);
    // 전역 레지스트리에 등록
    s_All.push_back(this);
    return S_OK;
}

void CMapObject::Priority_Update(_float) {}
void CMapObject::Update(_float dt)
{
    if (m_eType == EObjectType::MONSTER)            m_pModelCom->Play_Animation(dt);
    if (m_eType == EObjectType::HERO)               m_pModelCom->Play_Animation(dt);
    if (m_eType == EObjectType::SPEAR)              m_pModelCom->Play_Animation(dt);
    if (m_eType == EObjectType::SKELETON_SPEAR)     m_pModelCom->Play_Animation(dt);
    if (m_eType == EObjectType::SKELETON_BOW)       m_pModelCom->Play_Animation(dt);
    if (m_eType == EObjectType::BOSS_HAND_L)        m_pModelCom->Play_Animation(dt);
    if (m_eType == EObjectType::BOSS_HAND_R)        m_pModelCom->Play_Animation(dt);
    if (m_eType == EObjectType::EYESPAWNER)         m_pModelCom->Play_Animation(dt);
    if (m_eType == EObjectType::MONSTER_EYE)        m_pModelCom->Play_Animation(dt);
    if (m_eType == EObjectType::MUSHROOM)           m_pModelCom->Play_Animation(dt);
}
void CMapObject::Late_Update(_float)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;
}

HRESULT CMapObject::Render()
{
    if (FAILED(Bind_ShaderResources())) return E_FAIL;

    _uint iNumMeshes = m_pModelCom->Get_NumMeshes();
    for (size_t i = 0; i < iNumMeshes; ++i)
    {
        if (FAILED(m_pModelCom->Bind_Materials(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE, 0)))
            return E_FAIL;

        if (m_eType == EObjectType::MONSTER || m_eType == EObjectType::HERO || m_eType == EObjectType::SPEAR || m_eType == EObjectType::SKELETON_SPEAR || m_eType == EObjectType::SKELETON_BOW ||
            m_eType == EObjectType::BOSS_HAND_L || m_eType == EObjectType::BOSS_HAND_R || m_eType == EObjectType::EYESPAWNER || m_eType == EObjectType::MONSTER_EYE || m_eType == EObjectType::MUSHROOM)
            if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i))) return E_FAIL;

        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }
    return S_OK;
}

HRESULT CMapObject::Ready_Components()
{
    const wchar_t* modelProto = nullptr;
    switch (m_eType)
    {
    case EObjectType::MONSTER:
        modelProto = TEXT("Prototype_Component_Model_Monster");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::ROCK_AA:
        modelProto = TEXT("Prototype_Component_Model_Rock_AA");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::HERO:
        modelProto = TEXT("Prototype_Component_Model_Hero");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::SPEAR:
        modelProto = TEXT("Prototype_Component_Model_Spear");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::SPEAR_STATIC:
        modelProto = TEXT("Prototype_Component_Model_Spear_Static");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::MONSTER_SPEAR:
        modelProto = TEXT("Prototype_Component_Model_Monster_Spear");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::MONSTER_BOW:
        modelProto = TEXT("Prototype_Component_Model_Monster_Bow");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::BRIDGE:
        modelProto = TEXT("Prototype_Component_Model_Bridge");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::CAVE:
        modelProto = TEXT("Prototype_Component_Model_Cave");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::SKELETON_SPEAR:
        modelProto = TEXT("Prototype_Component_Model_Monster");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::SKELETON_BOW:
        modelProto = TEXT("Prototype_Component_Model_Monster");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::BOSS_EYE_MID:
        modelProto = TEXT("Prototype_Component_Model_Boss_Eye_Mid");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::BOSS_EYE_TOP:
        modelProto = TEXT("Prototype_Component_Model_Boss_Eye_TOP");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::BOSS_FIRE:
        modelProto = TEXT("Prototype_Component_Model_Boss_Fire");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::BOSS_HAND_L:
        modelProto = TEXT("Prototype_Component_Model_Boss_Hand_L");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::BOSS_HAND_R:
        modelProto = TEXT("Prototype_Component_Model_Boss_Hand_R");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::BOSS_MASK:
        modelProto = TEXT("Prototype_Component_Model_Boss_Mask");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::EYESPAWNER:
        modelProto = TEXT("Prototype_Component_Model_EyeSpawner");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::MONSTER_EYE:
        modelProto = TEXT("Prototype_Component_Model_Monster_Eye");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::MUSHROOM:
        modelProto = TEXT("Prototype_Component_Model_Mushroom");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;

    case EObjectType::SMALLMUSHROOM:
        modelProto = TEXT("Prototype_Component_Model_Small_Mushroom");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    case EObjectType::PARASIT_EYE:
        modelProto = TEXT("Prototype_Component_Model_Parasit_Eye");
        if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_Component_Shader_VtxMesh"),
            TEXT("Com_Shader"), (CComponent**)&m_pShaderCom, nullptr))) return E_FAIL;
        break;
    default: return E_FAIL;
    }

    if (FAILED(Add_Component(ENUM_CLASS(LEVEL::EDIT), modelProto,
        TEXT("Com_Model"), (CComponent**)&m_pModelCom, nullptr)))
        return E_FAIL;
    return S_OK;
}

HRESULT CMapObject::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix"))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ)))) return E_FAIL;

    const LIGHT_DESC* pLight = m_pGameInstance->Get_LightDesc(0);
    if (!pLight) return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDir", &pLight->vDirection, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &pLight->vDiffuse, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightAmbient", &pLight->vAmbient, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightSpecular", &pLight->vSpecular, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4)))) return E_FAIL;
    return S_OK;
}

CMapObject* CMapObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMapObject* p = new CMapObject(pDevice, pContext);
    if (FAILED(p->Initialize_Prototype())) { MSG_BOX(TEXT("Failed to Created : CMapObject")); Safe_Release(p); }
    return p;
}

CGameObject* CMapObject::Clone(void* pArg)
{
    CMapObject* p = new CMapObject(*this);
    if (FAILED(p->Initialize(pArg))) { MSG_BOX(TEXT("Failed to Created : CMapObject")); Safe_Release(p); }
    return p;
}

void CMapObject::Free()
{
    __super::Free();
    // 전역 레지스트리에서 제거
    auto it = std::find(s_All.begin(), s_All.end(), this);
    if (it != s_All.end()) s_All.erase(it);

    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
NS_END
