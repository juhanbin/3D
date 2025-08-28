#include "Monster_Skeleton.h"
#include "GameInstance.h"
#include "PlayerManager.h"
#include "Body_Monster_Skeleton.h"
#include "Weapon_Skeleton_Spear.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

template<typename T>
static inline T clamp_compat(T v, T lo, T hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}
namespace {
    constexpr float kAttackRange = 2.2f;      
    constexpr float kChaseRange = 12.0f;      
    constexpr float kMoveSpeed = 2.8f;        
    constexpr float kTurnSpeed = XM_PIDIV2;   

    inline float WrapPi(float a) {
        while (a > XM_PI)  a -= XM_2PI;
        while (a < -XM_PI) a += XM_2PI;
        return a;
    }
    inline float ApproachYaw(float curYaw, float targetYaw, float dt, float turnSpeed) {
        const float delta = WrapPi(targetYaw - curYaw);
        const float maxStep = turnSpeed * dt;
        const float step = clamp_compat(delta, -maxStep, +maxStep);
        return WrapPi(curYaw + step);
    }
}


CMonster_Skeleton::CMonster_Skeleton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CContainerObject{ pDevice,pContext } {
}

CMonster_Skeleton::CMonster_Skeleton(const CMonster_Skeleton& Prototype)
    : CContainerObject{ Prototype }
    , m_eType(Prototype.m_eType) {
}

HRESULT CMonster_Skeleton::Initialize_Prototype() { return S_OK; }

HRESULT CMonster_Skeleton::Initialize(void* pArg)
{
    GAMEOBJECT_DESC Desc{};
    Desc.fSpeedPerSec = kMoveSpeed;                     
    Desc.fRotationPerSec = XMConvertToRadians(180.f);   

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (pArg)
    {
        Monster_Skeleton_DESC* pDesc = static_cast<Monster_Skeleton_DESC*>(pArg);
        m_eType = pDesc->type;

        const XMMATRIX S = XMMatrixScaling(pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z);
        const XMMATRIX R = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(pDesc->vRot.x),
            XMConvertToRadians(pDesc->vRot.y),
            XMConvertToRadians(pDesc->vRot.z));
        const XMMATRIX T = XMMatrixTranslation(pDesc->vPos.x, pDesc->vPos.y, pDesc->vPos.z);
        const XMMATRIX W = S * R * T;

        XMFLOAT4X4 W4; XMStoreFloat4x4(&W4, W);
        m_pTransformCom->Set_State(STATE::RIGHT, XMLoadFloat4((XMFLOAT4*)&W4.m[0]));
        m_pTransformCom->Set_State(STATE::UP, XMLoadFloat4((XMFLOAT4*)&W4.m[1]));
        m_pTransformCom->Set_State(STATE::LOOK, XMLoadFloat4((XMFLOAT4*)&W4.m[2]));
        m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat4((XMFLOAT4*)&W4.m[3]));
    }

    if (FAILED(Ready_Components()))  return E_FAIL;
    if (FAILED(Ready_PartObjects())) return E_FAIL;

    // 시작 상태
    m_iState = MONSTER::SPEARE_IDLE;

    return S_OK;
}

void CMonster_Skeleton::Priority_Update(_float fTimeDelta)
{
    __super::Priority_Update(fTimeDelta);
}

void CMonster_Skeleton::Update(_float fTimeDelta)
{
    if (m_bisHit)
    {
        if (m_iState != MONSTER::HIT)
            m_iState = MONSTER::HIT;
        __super::Update(fTimeDelta);
        return; 
    }
    // 플레이어 위치
    _float3 playerPos{ 0,0,0 };
    if (auto* pm = Client::CPlayerManager::GetInstance())
        XMStoreFloat3(&playerPos, pm->GetPos());
    else {
        const _float4x4 V = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW);
        const XMMATRIX invV = XMMatrixInverse(nullptr, XMLoadFloat4x4(&V));
        XMFLOAT4X4 invV4{}; XMStoreFloat4x4(&invV4, invV);
        playerPos = _float3(invV4._41, invV4._42, invV4._43);
    }

    // 내 위치/거리
    XMVECTOR myPosV = m_pTransformCom->Get_State(STATE::POSITION);
    _float3  myPos; XMStoreFloat3(&myPos, myPosV);
    const float dx = playerPos.x - myPos.x;
    const float dz = playerPos.z - myPos.z;
    const float distXZ = std::sqrt(dx * dx + dz * dz);

    if (m_eType == EObjectType::SKELETON_SPEAR)
    {
        if (distXZ <= kAttackRange) {
            m_iState = MONSTER::SPEARE_ATTACK;
        }
        else if (distXZ <= kChaseRange) {
            m_iState = MONSTER::WALK;

            // 부드러운 회전(yaw) + 전진
            XMVECTOR look = m_pTransformCom->Get_State(STATE::LOOK);
            const float curYaw = std::atan2(XMVectorGetX(look), XMVectorGetZ(look));
            const float tgtYaw = std::atan2(dx, dz);
            const float newYaw = ApproachYaw(curYaw, tgtYaw, fTimeDelta, kTurnSpeed);
            m_pTransformCom->Rotation(0.f, newYaw, 0.f);

            m_pTransformCom->Go_Straight(fTimeDelta, 1.0f, m_pNavigationCom);
        }
        else {
            m_iState = MONSTER::SPEARE_IDLE;
        }
    }
    else if (m_eType == EObjectType::SKELETON_BOW)
    {
        if (distXZ <= kChaseRange) {
            m_iState = MONSTER::BOW_ATTACK;

            XMVECTOR look = m_pTransformCom->Get_State(STATE::LOOK);
            const float curYaw = std::atan2(XMVectorGetX(look), XMVectorGetZ(look));
            const float tgtYaw = std::atan2(dx, dz);
            const float newYaw = ApproachYaw(curYaw, tgtYaw, fTimeDelta, kTurnSpeed);
            m_pTransformCom->Rotation(0.f, newYaw, 0.f);
        }
        else {
            m_iState = MONSTER::Bow_IDLE;
        }
    }

    for (auto& pCollider : m_pColliderCom)
        if (pCollider) pCollider->Update(m_pTransformCom->Get_WorldMatrix());

    if (Collision_ToPlayer())
    {
        m_bisHit = true;
        m_iState = MONSTER::HIT;
    }

    __super::Update(fTimeDelta);
}



void CMonster_Skeleton::Late_Update(_float fTimeDelta)
{
    // 네비 위로 위치 보정
    m_pTransformCom->Set_State(Engine::STATE::POSITION,
        m_pNavigationCom->Compute_OnCell(m_pTransformCom->Get_State(Engine::STATE::POSITION)));

    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;

    __super::Late_Update(fTimeDelta);
}

HRESULT CMonster_Skeleton::Render()
{
#ifdef _DEBUG
    for (auto& pCollider : m_pColliderCom)
        pCollider->Render();
    if (m_pNavigationCom) m_pNavigationCom->Render();
#endif
    return S_OK;
}

HRESULT CMonster_Skeleton::Ready_Components()
{
    CNavigation::NAVIGATION_DESC NaviDesc{};
    NaviDesc.iCurrentCellIndex = 0;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Navigation"),
        TEXT("Com_Navigation"), reinterpret_cast<CComponent**>(&m_pNavigationCom), &NaviDesc)))
        return E_FAIL;

    CBounding_AABB::BOUNDING_AABB_DESC  AABBDesc{};
    AABBDesc.vExtents = _float3(0.4f, 0.7f, 0.4f);
    AABBDesc.vCenter = _float3(0.f, AABBDesc.vExtents.y, 0.f);

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_AABB"),
        TEXT("Com_Collider_AABB"), reinterpret_cast<CComponent**>(&m_pColliderCom[ENUM_CLASS(COLLIDER::AABB)]), &AABBDesc)))
        return E_FAIL;

    CBounding_OBB::BOUNDING_OBB_DESC  OBBDesc{};
    OBBDesc.vAngles = _float3(XMConvertToRadians(45.0f), XMConvertToRadians(45.0f), XMConvertToRadians(45.0f));
    OBBDesc.vExtents = _float3(0.6f, 0.6f, 0.6f);
    OBBDesc.vCenter = _float3(0.f, OBBDesc.vExtents.y, 0.f);

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_OBB"),
        TEXT("Com_Collider_OBB"), reinterpret_cast<CComponent**>(&m_pColliderCom[ENUM_CLASS(COLLIDER::OBB)]), &OBBDesc)))
        return E_FAIL;

    CBounding_Sphere::BOUNDING_SPHERE_DESC  SphereDesc{};
    SphereDesc.fRadius = 0.7f;
    SphereDesc.vCenter = _float3(0.f, SphereDesc.fRadius, 0.f);

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider_Sphere"), reinterpret_cast<CComponent**>(&m_pColliderCom[ENUM_CLASS(COLLIDER::SPHERE)]), &SphereDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMonster_Skeleton::Ready_PartObjects()
{
    CBody_Monster_Skeleton::BODY_MONSTER_SKELETON_DESC BodyDesc{};
    BodyDesc.pState = &m_iState;
    BodyDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();

    if (FAILED(__super::Add_PartObject(TEXT("Part_Body_Skeleton"),
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Body_Skeleton"), &BodyDesc)))
        return E_FAIL;

    CPartObject* pBody = Find_PartObject(TEXT("Part_Body_Skeleton"));
    if (!pBody) return E_FAIL;


    CWeapon_Skeleton_Spear::WEAPON_DESC WeaponDesc{};

    WeaponDesc.pState = &m_iState;
    WeaponDesc.pSocketMatrix = dynamic_cast<CBody_Monster_Skeleton*>(pBody)->Get_BoneMatrix("jnt_index_01_SKN_right");
    WeaponDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();

    if (m_eType == EObjectType::SKELETON_SPEAR)
    {
        if (FAILED(__super::Add_PartObject(TEXT("Part_Weapon_Monster_Spear"),
            ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Skeleton_Spear"), &WeaponDesc)))
            return E_FAIL;
    }
    else if (m_eType == EObjectType::SKELETON_BOW)
    {
        if (FAILED(__super::Add_PartObject(TEXT("Part_Weapon_Monster_Bow"),
            ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Skeleton_Bow"), &WeaponDesc)))
            return E_FAIL;
    }

    return S_OK;
}

_bool CMonster_Skeleton::Collision_ToPlayer()
{
    CContainerObject* pPlayer = dynamic_cast<CContainerObject*>(m_pGameInstance->Find_GameObject(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Layer_Player")));
    if (nullptr == pPlayer)
        return false;

    CCollider* pTargetCollider = static_cast<CCollider*>(pPlayer->Get_Component(TEXT("Part_Weapon"), TEXT("Com_Collider")));
    if (nullptr == pTargetCollider)
        return false;

    return m_pColliderCom[ENUM_CLASS(COLLIDER::OBB)]->Intersect(pTargetCollider);
}

CMonster_Skeleton* CMonster_Skeleton::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMonster_Skeleton* pInstance = new CMonster_Skeleton(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CMonster_Skeleton"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CMonster_Skeleton::Clone(void* pArg)
{
    CMonster_Skeleton* pInstance = new CMonster_Skeleton(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CMonster_Skeleton"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CMonster_Skeleton::Free()
{
    for (auto& pCollider : m_pColliderCom)
        Safe_Release(pCollider);
    __super::Free();
}
