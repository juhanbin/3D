#include "Parasit_Eye.h"
#include "GameInstance.h"

// === 추가 include (발사/조준에 필요) ===
#include "PlayerManager.h"
#include "Boss_Fire.h"
#include "Object_Pool_Manager.h"

using namespace Client;
using namespace DirectX;

CParasit_Eye::CParasit_Eye(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext } {
}

CParasit_Eye::CParasit_Eye(const CParasit_Eye& Prototype)
    : CGameObject{ Prototype } {
}

HRESULT CParasit_Eye::Initialize_Prototype() { return S_OK; }

HRESULT CParasit_Eye::Initialize(void* pArg)
{
    GAMEOBJECT_DESC Desc{};
    Desc.fSpeedPerSec = 0.f;
    Desc.fRotationPerSec = 0.f;
    if (FAILED(__super::Initialize(&Desc))) return E_FAIL;

    if (pArg)
    {
        auto* pDesc = static_cast<Parasit_Eye_Desc*>(pArg);
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

    // 적절한 애니메이션 슬롯으로 재생
    /*m_pModelCom->Set_Animation(10, true);*/

    // 발사 타이머 초기화
    m_fireTimer = 0.f;

    return S_OK;
}

void CParasit_Eye::Priority_Update(_float) {}

void CParasit_Eye::Update(_float dt)
{
    // 애니메이션 진행
    /*if (m_pModelCom) m_pModelCom->Play_Animation(dt);*/

    TickFire(dt);
}

void CParasit_Eye::Late_Update(_float)
{
    m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this);
}

HRESULT CParasit_Eye::Render()
{
    if (FAILED(Bind_ShaderResources())) return E_FAIL;

    const _uint n = m_pModelCom->Get_NumMeshes();
    for (_uint i = 0; i < n; ++i)
    {
        m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, 0, 0);
        m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_NormalTexture", i, 1, 0);
        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }

#ifdef _DEBUG
    if (m_pCollider)
        m_pGameInstance->Add_DebugComponent(m_pCollider);
#endif

    return S_OK;
}

// ==================== 컴포넌트/셰이더 ====================

HRESULT CParasit_Eye::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Eye"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;

    // 필요 시 네비/콜라이더 추가

    return S_OK;
}

HRESULT CParasit_Eye::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix"))) return E_FAIL;
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

// ==================== 활 스타일 멤버 유틸 ====================

_float4x4 CParasit_Eye::GetWorld() const
{
    _float4x4 W{};
    XMStoreFloat4x4(&W, m_pTransformCom->Get_WorldMatrix());
    return W;
}

_float3 CParasit_Eye::GetMuzzleWorldPos() const
{
    const _float4x4 W = GetWorld();

    XMVECTOR U = XMVector3Normalize(XMVectorSet(W._21, W._22, W._23, 0.f));
    XMVECTOR L = XMVector3Normalize(XMVectorSet(W._31, W._32, W._33, 0.f));
    XMVECTOR P = XMVectorSet(W._41, W._42, W._43, 1.f);

    P = P + L * m_muzzleFwd + U * m_muzzleUp;

    _float3 out{};
    XMStoreFloat3(&out, P);
    return out;
}

_float3 CParasit_Eye::AimDirToPlayer() const
{
    const _float3 muzzle = GetMuzzleWorldPos();

    _float3 player{};
    if (auto* pm = CPlayerManager::GetInstance())
        XMStoreFloat3(&player, pm->GetPos());
    else
        return _float3{ 0.f, 0.f, 1.f };

    // 살짝 위(가슴/머리)로 조준
    player.y += 1.2f;

    XMVECTOR M = XMLoadFloat3(&muzzle);
    XMVECTOR T = XMLoadFloat3(&player);
    XMVECTOR D = XMVector3Normalize(T - M);

    _float3 out{};
    XMStoreFloat3(&out, D);
    return out;
}

void CParasit_Eye::FireOnce()
{
    auto* pool = CObject_Pool_Manager::GetInstance();
    if (!pool) return;

    CGameObject* obj = pool->Acquire(LEVEL::GAMEPLAY, L"Layer_Boss_Fire");
    auto* fire = dynamic_cast<CBoss_Fire*>(obj);
    if (!fire)
    {
        OutputDebugStringW(L"[Boss_Hand_L] Acquire Layer_Boss_Fire 실패 또는 타입 불일치\n");
        if (obj) obj->Set_Active(false);
        return;
    }

    CBoss_Fire::DESC d{};
    d.pos = GetMuzzleWorldPos();
    d.dir = AimDirToPlayer();
    d.speed = m_initialSpeed;
    d.gravity = m_gravity;
    d.maxLife = 3.0f;
    d.owner = this;

    fire->Reuse_Begin(&d);

#ifdef _DEBUG
    wchar_t buf[160];
    swprintf(buf, 160, L"[Boss_Hand_L] FireOnce  pos=(%.2f,%.2f,%.2f) dir=(%.2f,%.2f,%.2f)\n",
        d.pos.x, d.pos.y, d.pos.z, d.dir.x, d.dir.y, d.dir.z);
    //OutputDebugStringW(buf);
#endif
}

void CParasit_Eye::TickFire(_float dt)
{
    m_fireTimer += dt;
    if (m_fireTimer >= m_fireCooldown)
    {
        m_fireTimer = 0.f;
        FireOnce();
    }
}

// ==================== 생성/해제 ====================

CParasit_Eye* CParasit_Eye::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    auto* p = new CParasit_Eye(pDevice, pContext);
    if (FAILED(p->Initialize_Prototype())) { MSG_BOX(TEXT("Failed to Created : CParasit_Eye")); Safe_Release(p); }
    return p;
}

CGameObject* CParasit_Eye::Clone(void* pArg)
{
    auto* p = new CParasit_Eye(*this);
    if (FAILED(p->Initialize(pArg))) { MSG_BOX(TEXT("Failed to Clone : CParasit_Eye")); Safe_Release(p); }
    return p;
}

void CParasit_Eye::Free()
{
    __super::Free();
    Safe_Release(m_pCollider);
    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
