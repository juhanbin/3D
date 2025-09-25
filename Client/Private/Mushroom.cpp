#include "Mushroom.h"
#include "GameInstance.h"

using namespace Client;
using namespace DirectX;

CMushroom::CMushroom(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext } {
}

CMushroom::CMushroom(const CMushroom& Prototype)
    : CGameObject{ Prototype } {
}

HRESULT CMushroom::Initialize_Prototype() { return S_OK; }

HRESULT CMushroom::Initialize(void* pArg)
{
    GAMEOBJECT_DESC Desc{};
    Desc.fSpeedPerSec = 0.f;
    Desc.fRotationPerSec = 0.f;
    if (FAILED(__super::Initialize(&Desc))) return E_FAIL;

    if (pArg)
    {
        auto* pDesc = static_cast<Mushroom_DESC*>(pArg);
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

    m_pModelCom->Set_Animation(ANIM_IDLE, true);
    return S_OK;
}

void CMushroom::Priority_Update(_float) {}

void CMushroom::Update(_float dt)
{
    // 애니: 살아있을 때만 시간 진행, 죽으면 멈춰둠(연출 바꾸고 싶으면 조정)
    if (m_eState == STATE::ALIVE) m_pModelCom->Play_Animation(dt);
    else                          m_pModelCom->Play_Animation(0.f);

    const _matrix W = m_pTransformCom->Get_WorldMatrix();

    // ★ 상태별로 필요한 콜라이더만 갱신
    if (m_eState == STATE::ALIVE)
    {
        if (m_pCollider_Block)   m_pCollider_Block->Update(W);
    }
    else // DEAD
    {
        if (m_pCollider_Trigger) m_pCollider_Trigger->Update(W);

        // 트리거 수명 관리(기본 3초)
        m_deadElapsed += dt;
        if (m_deadElapsed >= m_triggerLife)
        {
            Safe_Release(m_pCollider_Trigger);
            m_pCollider_Trigger = nullptr;
        }
    }
}

void CMushroom::Late_Update(_float)
{
    m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this);

#ifdef _DEBUG
    if (m_eState == STATE::ALIVE) {
        if (m_pCollider_Block)
            m_pGameInstance->Add_DebugComponent(m_pCollider_Block);
    }
    else {
        if (m_pCollider_Trigger)
            m_pGameInstance->Add_DebugComponent(m_pCollider_Trigger);
    }
#endif

}

HRESULT CMushroom::Render()
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

//#ifdef _DEBUG
//    // ★ 상태별로 필요한 콜라이더만 렌더
//    if (m_eState == STATE::ALIVE) {
//        if (m_pCollider_Block)   m_pCollider_Block->Render();
//    }
//    else {
//        if (m_pCollider_Trigger) m_pCollider_Trigger->Render();
//    }
//#endif
    return S_OK;
}

HRESULT CMushroom::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Mushroom"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;

    // 본체(막힘) ? 살아있을 때만 사용
    CBounding_Sphere::BOUNDING_SPHERE_DESC body{};
    body.fRadius = 0.45f;
    body.vCenter = _float3(0.f, body.fRadius, 0.f);
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider_Block"), reinterpret_cast<CComponent**>(&m_pCollider_Block), &body)))
        return E_FAIL;

    // 사망 트리거 ? 죽은 뒤 일정 시간만 사용
    CBounding_Sphere::BOUNDING_SPHERE_DESC trg{};
    trg.fRadius = 0.6f;
    trg.vCenter = _float3(0.f, trg.fRadius, 0.f);
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider_Trigger"), reinterpret_cast<CComponent**>(&m_pCollider_Trigger), &trg)))
        return E_FAIL;

    return S_OK;
}

HRESULT CMushroom::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix"))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ)))) return E_FAIL;

   /* const LIGHT_DESC* L = m_pGameInstance->Get_LightDesc(0);
    if (!L) return E_FAIL;
    m_pShaderCom->Bind_RawValue("g_vLightDir", &L->vDirection, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &L->vDiffuse, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vLightAmbient", &L->vAmbient, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vLightSpecular", &L->vSpecular, sizeof(_float4));
    m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4));*/
    return S_OK;
}

void CMushroom::TakeDamage(int dmg, Engine::CGameObject*)
{
    if (m_eState == STATE::DEAD) return;

    m_hp -= dmg;
    if (m_hp <= 0)
    {
        m_hp = 0;
        m_eState = STATE::DEAD;
        m_deadElapsed = 0.f; // 트리거 타이머 시작

        // 필요하면 여기서 죽는 애니로 전환 가능:
        // m_pModelCom->Set_Animation(ANIM_DIE, false, 0.1f, true);
    }
}

CMushroom* CMushroom::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    auto* p = new CMushroom(pDevice, pContext);
    if (FAILED(p->Initialize_Prototype())) { MSG_BOX(TEXT("Failed to Created : CMushroom")); Safe_Release(p); }
    return p;
}

CGameObject* CMushroom::Clone(void* pArg)
{
    auto* p = new CMushroom(*this);
    if (FAILED(p->Initialize(pArg))) { MSG_BOX(TEXT("Failed to Created : CMushroom")); Safe_Release(p); }
    return p;
}

void CMushroom::Free()
{
    __super::Free();
    Safe_Release(m_pCollider_Block);
    Safe_Release(m_pCollider_Trigger);
    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
