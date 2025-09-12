#include "Weapon_Skeleton_Bow.h"
#include "GameInstance.h"
#include "PlayerManager.h"
#include "Object_Pool_Manager.h"
#include "Weapon_Skeleton_Arrow.h"

using namespace DirectX;
using namespace Client;

CWeapon_Skeleton_Bow::CWeapon_Skeleton_Bow(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CPartObject{ pDevice, pContext } {
}

CWeapon_Skeleton_Bow::CWeapon_Skeleton_Bow(const CWeapon_Skeleton_Bow& Prototype)
    : CPartObject{ Prototype } {
}

HRESULT CWeapon_Skeleton_Bow::Initialize_Prototype() { return S_OK; }

HRESULT CWeapon_Skeleton_Bow::Initialize(void* pArg)
{
    WEAPON_DESC* pDesc = static_cast<WEAPON_DESC*>(pArg);
    m_pParentState = pDesc->pState;
    m_pSocketMatrix = pDesc->pSocketMatrix;
    m_pAnimModel = pDesc->pAnimModel;

    if (FAILED(__super::Initialize(pArg)))  return E_FAIL;
    if (FAILED(Ready_Components()))         return E_FAIL;

    m_pTransformCom->Rotation(120.f, -90.f, -90.f);

    m_InAttackState = false;
    m_FiredThisCycle = false;
    m_prevAnim01Local = -1.f; 
    return S_OK;
}

void CWeapon_Skeleton_Bow::Priority_Update(_float fTimeDelta)
{
    __super::Priority_Update(fTimeDelta);
}

void CWeapon_Skeleton_Bow::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);

    _matrix Bone = XMLoadFloat4x4(m_pSocketMatrix);
    for (int i = 0; i < 3; ++i) Bone.r[i] = XMVector3Normalize(Bone.r[i]);

    XMStoreFloat4x4(&m_CombinedWorldMatrix,
        m_pTransformCom->Get_WorldMatrix() *
        Bone *
        XMLoadFloat4x4(m_pParentMatrix));

    TickAimSync(fTimeDelta);
}

void CWeapon_Skeleton_Bow::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;
}

HRESULT CWeapon_Skeleton_Bow::Render()
{
    if (FAILED(Bind_ShaderResources())) return E_FAIL;

    _uint n = m_pModelCom->Get_NumMeshes();
    for (_uint i = 0; i < n; ++i)
    {
        m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, 0, 0);
        m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_NormalTexture", i, 1, 0);
        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }
    return S_OK;
}

HRESULT CWeapon_Skeleton_Bow::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Monster_Bow"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;

    return S_OK;
}

HRESULT CWeapon_Skeleton_Bow::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix"))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_CombinedWorldMatrix)))    return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ)))) return E_FAIL;

    /*const LIGHT_DESC* L = m_pGameInstance->Get_LightDesc(0);
    if (!L) return E_FAIL;

    m_pShaderCom->Bind_RawValue("g_vLightDir", &L->vDirection, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &L->vDiffuse, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vLightAmbient", &L->vAmbient, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vLightSpecular", &L->vSpecular, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4));*/
    return S_OK;
}

/* ----------------- 애니메이션 동기화 핵심 ----------------- */
void CWeapon_Skeleton_Bow::TickAimSync(_float /*dt*/)
{
    if (!m_pParentState || !m_pAnimModel) return;

    const bool attacking = (*m_pParentState == MONSTER::BOW_ATTACK);

    // 상태 진입/이탈
    if (attacking && !m_InAttackState) {
        m_InAttackState = true;
        m_FiredThisCycle = false;
        m_prevAnim01Local = -1.f;                   // ★ 로컬 진행도 초기화
        m_pAnimModel->ClearAnimEventWindow();        // 이벤트 창 초기화
    }
    else if (!attacking && m_InAttackState) {
        m_InAttackState = false;
        m_FiredThisCycle = false;
        m_prevAnim01Local = -1.f;                   // ★ 상태 이탈 시도 초기화
        return;
    }
    if (!attacking) return;

    const float cur01 = m_pAnimModel->GetAnimProgress01();
    if (m_prevAnim01Local >= 0.f && cur01 < m_prevAnim01Local) {
        m_FiredThisCycle = false; 
    }
    m_prevAnim01Local = cur01;

    const bool crossed = m_pAnimModel->AnimCrossedNormalized(m_FireAtNormalized);

    if (!m_FiredThisCycle && crossed) {
        if (IsAimedEnough(m_AimDegThreshold)) {
            FireOnce();
        }
        else {
            FireOnce();
        }
        m_FiredThisCycle = true;
    }
}

bool CWeapon_Skeleton_Bow::IsAimedEnough(float degThreshold) const
{
    _float3 muzzle = Get_SocketWorldPos();

    _float3 player{};
    if (auto* pm = Client::CPlayerManager::GetInstance())
        XMStoreFloat3(&player, pm->GetPos());
    else
        return true;

    player.y += 1.2f;

    XMVECTOR M = XMLoadFloat3(&muzzle);
    XMVECTOR T = XMLoadFloat3(&player);
    XMVECTOR toTarget = XMVector3Normalize(T - M);

    _float4x4 W = m_CombinedWorldMatrix;
    XMVECTOR look = XMVector3Normalize(XMVectorSet(W._31, W._32, W._33, 0.f));

    float dot = XMVectorGetX(XMVector3Dot(look, toTarget));
    dot = max(-1.f, min(1.f, dot));
    float angleDeg = XMConvertToDegrees(acosf(dot));
    return angleDeg <= degThreshold;
}

/* ----------------- 실제 발사 ----------------- */
_float4x4 CWeapon_Skeleton_Bow::Get_SocketWorldMatrix() const
{
    return m_CombinedWorldMatrix;
}

_float3 CWeapon_Skeleton_Bow::Get_SocketWorldPos() const
{
    _float4x4 W = Get_SocketWorldMatrix();
    _float3 pos{ W._41, W._42, W._43 };

    XMVECTOR U = XMVector3Normalize(XMVectorSet(W._21, W._22, W._23, 0.f));
    XMVECTOR L = XMVector3Normalize(XMVectorSet(W._31, W._32, W._33, 0.f));
    XMVECTOR P = XMVectorSet(pos.x, pos.y, pos.z, 1.f);
    P = P + L * m_MuzzleFwd + U * m_MuzzleUp;

    XMStoreFloat3(&pos, P);
    return pos;
}

void CWeapon_Skeleton_Bow::FireOnce()
{
    _float3 player{};
    if (auto* pm = Client::CPlayerManager::GetInstance())
        XMStoreFloat3(&player, pm->GetPos());
    else
        return;
    player.y += 1.2f;

    _float3 muzzle = Get_SocketWorldPos();

    const float v = m_InitialSpeed;
    const float g = m_Gravity;

    XMVECTOR M = XMLoadFloat3(&muzzle);
    XMVECTOR T = XMLoadFloat3(&player);
    XMVECTOR r = T - M;

    XMVECTOR rXZ = XMVectorSet(XMVectorGetX(r), 0.f, XMVectorGetZ(r), 0.f);
    float R = XMVectorGetX(XMVector3Length(rXZ));
    float y = XMVectorGetY(r);
    XMVECTOR dirXZ = (R > 1e-4f) ? XMVector3Normalize(rXZ) : XMVectorSet(0, 0, 1, 0);

    float v2 = v * v;
    float gabs = -g;
    float disc = v2 * v2 - gabs * (gabs * R * R + 2.f * y * v2);

    XMVECTOR dir;
    if (disc >= 0.f && R > 1e-4f) {
        float root = sqrtf(disc);
        float tanTheta = (v2 - root) / (gabs * R);
        float cosT = 1.f / sqrtf(1.f + tanTheta * tanTheta);
        float sinT = tanTheta * cosT;
        dir = XMVector3Normalize(dirXZ * cosT + XMVectorSet(0.f, 1.f, 0.f, 0.f) * sinT);
    }
    else {
        dir = XMVector3Normalize(r + XMVectorSet(0.f, 0.1f * R, 0.f, 0.f));
    }

    auto* pool = CObject_Pool_Manager::GetInstance();
    auto* obj = pool->Acquire(LEVEL::GAMEPLAY, L"Layer_Arrow");
    auto* arrow = dynamic_cast<CWeapon_Skeleton_Arrow*>(obj);
    if (!arrow) return;

    CWeapon_Skeleton_Arrow::DESC d{};
    XMStoreFloat3(&d.pos, M);
    XMStoreFloat3(&d.dir, dir);
    d.speed = v;
    d.gravity = g;
    d.maxLife = 4.0f;

    arrow->Reuse_Begin(&d);
}

/* ----------------- 생성/해제 ----------------- */
CWeapon_Skeleton_Bow* CWeapon_Skeleton_Bow::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    auto* p = new CWeapon_Skeleton_Bow(pDevice, pContext);
    if (FAILED(p->Initialize_Prototype())) { MSG_BOX(TEXT("Failed to Created : CWeapon_Skeleton_Bow")); Safe_Release(p); }
    return p;
}

CGameObject* CWeapon_Skeleton_Bow::Clone(void* pArg)
{
    auto* p = new CWeapon_Skeleton_Bow(*this);
    if (FAILED(p->Initialize(pArg))) { MSG_BOX(TEXT("Failed to Clone : CWeapon_Skeleton_Bow")); Safe_Release(p); }
    return p;
}

void CWeapon_Skeleton_Bow::Free()
{
    __super::Free();
    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
