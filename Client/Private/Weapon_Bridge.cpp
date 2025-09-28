#include "Weapon_Bridge.h"
#include "GameInstance.h"

CWeapon_Bridge::CWeapon_Bridge(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CPartObject{ pDevice, pContext }
{

}

CWeapon_Bridge::CWeapon_Bridge(const CWeapon_Bridge& Prototype)
    : CPartObject{ Prototype }
{

}

HRESULT CWeapon_Bridge::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CWeapon_Bridge::Initialize(void* pArg)
{
    WEAPON_DESC* pDesc = static_cast<WEAPON_DESC*>(pArg);
    m_pParentState = pDesc->pState;
    m_pSocketMatrix = pDesc->pSocketMatrix;
    m_pSocketMatrix_Hand = pDesc->pSocketMatrix_Hand;
    m_pMoving = pDesc->pMoving;
    m_pAttack = pDesc->pAttack;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    m_pTransformCom->Set_State(STATE::POSITION, XMVectorSet(0.f, 100.f, 0.2f, 1.f));
    m_pModelCom->Set_Animation(2, true);

    wchar_t wbuf[128];
    swprintf(wbuf, 128, L"[ADDR] Weapon model=%p shader=%p\n", m_pModelCom, m_pShaderCom);
    OutputDebugStringW(wbuf);

    XMStoreFloat4x4(&m_GripLocal, XMMatrixIdentity());
    return S_OK;
}

void CWeapon_Bridge::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CWeapon_Bridge::Update(_float dt)
{
    const bool aiming =
        (*m_pAttack == ATTACK::ENTER || *m_pAttack == ATTACK::IDLE ||
            *m_pAttack == ATTACK::FRONT || *m_pAttack == ATTACK::BACK ||
            *m_pAttack == ATTACK::LEFT || *m_pAttack == ATTACK::RIGHT ||
            *m_pAttack == ATTACK::GROUND);

    const _matrix socketW = aiming
        ? XMLoadFloat4x4(m_pSocketMatrix_Hand)
        : XMLoadFloat4x4(m_pSocketMatrix);

    const _matrix parentW = XMLoadFloat4x4(m_pParentMatrix);
    const _matrix gripLocal = XMLoadFloat4x4(&m_GripLocal);

    // 1) 모델-손 축 정렬(고정값) : 모델 기본자세에 맞춰 한 번만 세팅
    const _matrix kAlign =
        XMMatrixRotationRollPitchYaw(m_alignEuler.x, m_alignEuler.y, m_alignEuler.z);

    // 2) 상태별 미세 보정
    const XMFLOAT3& eul = aiming ? m_aimEuler : m_equipEuler;
    const _float3& offs = aiming ? m_aimOffset : m_equipOffset;

    const _matrix stateTweak =
        XMMatrixRotationRollPitchYaw(eul.x, eul.y, eul.z) *
        XMMatrixTranslation(offs.x, offs.y, offs.z);

    // 3) 최종 월드 = Grip^-1 * kAlign * stateTweak * socket * parent
    const _matrix weaponW =
        XMMatrixInverse(nullptr, gripLocal) *
        kAlign *
        stateTweak *
        socketW *
        parentW;

    XMStoreFloat4x4(&m_CombinedWorldMatrix, weaponW);
    m_pModelCom->Play_Animation(dt);
}


void CWeapon_Bridge::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;

    //#ifdef _DEBUG
    //    m_pGameInstance->Add_DebugComponent(m_pColliderCom);
    //#endif
}

HRESULT CWeapon_Bridge::Render()
{
    if (*m_pAttack == ATTACK::THROW)
        return S_OK;

    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint           iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (size_t i = 0; i < iNumMeshes; i++)
    {
        if (FAILED(m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, ENUM_CLASS(TextureType::DIFFUSE), 0))) // 0: DIFFUSE
        {
            //OutputDebugStringA("Weapon_머티리얼 바인딩 실패!\n");
            return E_FAIL;
        }
        else
        {
            //OutputDebugStringA("Spear_머티리얼 바인딩 성공!\n");
        }

        if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
            return E_FAIL;

        m_pShaderCom->Begin(0);

        m_pModelCom->Render(i);
    }



    return S_OK;
}

HRESULT CWeapon_Bridge::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::BRIDGE), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::BRIDGE), TEXT("Prototype_Component_Model_Spear"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;

    /*CBounding_OBB::BOUNDING_OBB_DESC  OBBDesc{};
    OBBDesc.vAngles = _float3(0.f, 0.f, 0.f);
    OBBDesc.vExtents = _float3(1.0f, 1.5f, 2.f);
    OBBDesc.vCenter = _float3(0.f, OBBDesc.vExtents.y, 0.f);

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_Component_Collider_OBB"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &OBBDesc)))
        return E_FAIL;*/

    return S_OK;
}

HRESULT CWeapon_Bridge::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_CombinedWorldMatrix)))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;

    return S_OK;
}

CWeapon_Bridge* CWeapon_Bridge::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CWeapon_Bridge* pInstance = new CWeapon_Bridge(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CWeapon_Bridge"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CWeapon_Bridge::Clone(void* pArg)
{
    CWeapon_Bridge* pInstance = new CWeapon_Bridge(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Clone : CWeapon_Bridge"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CWeapon_Bridge::Free()
{
    __super::Free();

    Safe_Release(m_pModelCom);
    Safe_Release(m_pColliderCom);
    Safe_Release(m_pShaderCom);
}
