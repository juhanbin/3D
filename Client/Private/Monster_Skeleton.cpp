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
    constexpr float kAttackRange = 2.2f;        // 근접 공격 반경
    constexpr float kChaseRange = 12.0f;       // 추적 시작 반경
    constexpr float kMoveSpeed = 2.8f;        // m/s (Transform의 fSpeedPerSec와 같게 써도 됨)
    constexpr float kTurnSpeed = XM_PIDIV2;   // 초당 90도 회전

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

/* ============================ 본체 ============================ */

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
    // 플레이어와 동일한 방식으로 기본 이동/회전 속도 지정
    GAMEOBJECT_DESC Desc{};
    Desc.fSpeedPerSec = kMoveSpeed;                      // 이동 기본속도
    Desc.fRotationPerSec = XMConvertToRadians(180.f);       // Transform::Turn용(여기서는 직접 yaw세팅 사용)

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
    // ===== 플레이어 위치 가져오기(인라인, 간단하게) =====
    _float3 playerPos{ 0,0,0 };
    if (auto* pm = Client::CPlayerManager::GetInstance())
        XMStoreFloat3(&playerPos, pm->GetPos());
    else {
        // 폴백: 카메라 위치(뷰 역행렬)
        const _float4x4 V = *m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW);
        const XMMATRIX invV = XMMatrixInverse(nullptr, XMLoadFloat4x4(&V));
        XMFLOAT4X4 invV4{}; XMStoreFloat4x4(&invV4, invV);
        playerPos = _float3(invV4._41, invV4._42, invV4._43);
    }

    // 내 위치
    XMVECTOR myPosV = m_pTransformCom->Get_State(STATE::POSITION);
    _float3  myPos; XMStoreFloat3(&myPos, myPosV);

    // XZ 거리
    const float dx = playerPos.x - myPos.x;
    const float dz = playerPos.z - myPos.z;
    const float distXZ = std::sqrt(dx * dx + dz * dz);

    if (m_eType == EObjectType::SKELETON_SPEAR)
    {
        // ===== 상태 전이 =====
        if (distXZ <= kAttackRange) {
            m_iState = MONSTER::SPEARE_ATTACK;
        }
        else if (distXZ <= kChaseRange) {
            m_iState = MONSTER::WALK;

            // 1) 부드러운 방향전환 (yaw만)
            XMVECTOR look = m_pTransformCom->Get_State(STATE::LOOK);
            const float curYaw = std::atan2(XMVectorGetX(look), XMVectorGetZ(look));
            const float tgtYaw = std::atan2(dx, dz);
            const float newYaw = ApproachYaw(curYaw, tgtYaw, fTimeDelta, kTurnSpeed);
            m_pTransformCom->Rotation(0.f, newYaw, 0.f);

            // 2) 앞으로 전진 (Navigation 통과)
            m_pTransformCom->Go_Straight(fTimeDelta, 1.0f /*speedMul*/, m_pNavigationCom);
        }
        else {
            m_iState = MONSTER::SPEARE_IDLE;
        }
    }
    else if (m_eType == EObjectType::SKELETON_BOW)
    {
        if (distXZ <= kChaseRange)
        {
            m_iState = MONSTER::BOW_ATTACK;

            XMVECTOR look = m_pTransformCom->Get_State(STATE::LOOK);
            const float curYaw = std::atan2(XMVectorGetX(look), XMVectorGetZ(look));
            const float tgtYaw = std::atan2(dx, dz);
            const float newYaw = ApproachYaw(curYaw, tgtYaw, fTimeDelta, kTurnSpeed);
            m_pTransformCom->Rotation(0.f, newYaw, 0.f);
        }
            
        else
            m_iState = MONSTER::Bow_IDLE;
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
    __super::Free();
}
