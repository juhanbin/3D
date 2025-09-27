#include "Boss_Hand_R.h"
#include "GameInstance.h"
#include "PlayerManager.h"

using namespace Client;
using namespace DirectX;

CBoss_Hand_R::CBoss_Hand_R(ID3D11Device* d, ID3D11DeviceContext* c) : CGameObject(d, c) {}
CBoss_Hand_R::CBoss_Hand_R(const CBoss_Hand_R& r) : CGameObject(r) {}

HRESULT CBoss_Hand_R::Initialize_Prototype() { return S_OK; }

HRESULT CBoss_Hand_R::Initialize(void* pArg)
{
    GAMEOBJECT_DESC Desc{};
    if (FAILED(__super::Initialize(&Desc))) return E_FAIL;

    if (pArg)
    {
        auto* p = static_cast<Boss_Hand_R_DESC*>(pArg);
        m_eType = (int)p->type;

        const XMMATRIX S = XMMatrixScaling(p->vScale.x, p->vScale.y, p->vScale.z);
        const XMMATRIX R = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(p->vRot.x), XMConvertToRadians(p->vRot.y), XMConvertToRadians(p->vRot.z));
        const XMMATRIX T = XMMatrixTranslation(p->vPos.x, p->vPos.y, p->vPos.z);
        XMFLOAT4X4 W; XMStoreFloat4x4(&W, S * R * T);

        m_pTransformCom->Set_State(STATE::RIGHT, XMLoadFloat4((XMFLOAT4*)&W.m[0]));
        m_pTransformCom->Set_State(STATE::UP, XMLoadFloat4((XMFLOAT4*)&W.m[1]));
        m_pTransformCom->Set_State(STATE::LOOK, XMLoadFloat4((XMFLOAT4*)&W.m[2]));
        m_pTransformCom->Set_State(STATE::POSITION, XMVectorSet(W._41, W._42, W._43, 1.f));
    }

    if (FAILED(Ready_Components())) return E_FAIL;
    if (m_pModelCom) m_pModelCom->Set_Animation(1, true);
    return S_OK;
}

void CBoss_Hand_R::Update(_float dt)
{
    if (m_pModelCom) m_pModelCom->Play_Animation(dt);
    TickSlam(dt);
}

void CBoss_Hand_R::Late_Update(_float)
{
    m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this);
#ifdef _DEBUG
    if (m_pCollider) m_pGameInstance->Add_DebugComponent(m_pCollider);
#endif
}

HRESULT CBoss_Hand_R::Render()
{
    if (FAILED(Bind_ShaderResources())) return E_FAIL;
    const _uint n = m_pModelCom->Get_NumMeshes();
    for (_uint i = 0; i < n; ++i)
    {
        m_pShaderCom->Begin(1);
        m_pModelCom->Render(i);
    }
    return S_OK;
}

HRESULT CBoss_Hand_R::Ready_Components()
{
    if (FAILED(Add_Component(ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr))) return E_FAIL;
    if (FAILED(Add_Component(ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_Component_Model_Boss_hand_R"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr))) return E_FAIL;
    return S_OK;
}

HRESULT CBoss_Hand_R::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix"))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ)))) return E_FAIL;
    return S_OK;
}

// ========== 패턴 ==========

void CBoss_Hand_R::StartSlam(const _float3& targetWorld)
{
    if (m_slam != SLAM::NONE) return;

    XMVECTOR P = m_pTransformCom->Get_State(STATE::POSITION);
    XMStoreFloat3(&m_slamStart, P);

    m_slamTarget = targetWorld;
    m_slam = SLAM::RISE;
    m_slamT = 0.f;
}

void CBoss_Hand_R::TickSlam(_float dt)
{
    if (m_slam == SLAM::NONE) return;

    auto* tr = m_pTransformCom;
    XMVECTOR P = tr->Get_State(STATE::POSITION);

    switch (m_slam)
    {
    case SLAM::RISE:
        m_slamT += dt;
        P = P + XMVectorSet(0, 1, 0, 0) * (m_slamRiseH * dt * 2.f);
        tr->Set_State(STATE::POSITION, AsPos(P));
        if (m_slamT >= 0.3f) m_slam = SLAM::DOWN;
        break;

    case SLAM::DOWN:
    {
        const XMVECTOR T = AsPos(m_slamTarget);
        const float step = m_slamSpeed * dt;

        const XMVECTOR toT = T - P;
        const float    dist = XMVectorGetX(XMVector3Length(toT));

        if (dist <= step)
        {
            tr->Set_State(STATE::POSITION, T);
            m_slam = SLAM::RECOVER;
            m_slamT = 0.f;
        }
        else
        {
            const XMVECTOR dir = XMVector3Normalize(toT);
            P = P + dir * step;
            tr->Set_State(STATE::POSITION, AsPos(P));
        }
        break;
    }

    case SLAM::RECOVER:
    {
        const XMVECTOR Home = AsPos(m_slamStart);
        const float step = m_recoverSpeed * dt;

        const XMVECTOR toH = Home - P;
        const float    dist = XMVectorGetX(XMVector3Length(toH));

        if (dist <= step)
        {
            tr->Set_State(STATE::POSITION, Home);
            m_slam = SLAM::NONE;
            m_slamT = 0.f;
        }
        else
        {
            const XMVECTOR dir = XMVector3Normalize(toH);
            P = P + dir * step;
            tr->Set_State(STATE::POSITION, AsPos(P));
        }
        break;
    }
    }
}

_float4x4 CBoss_Hand_R::GetWorld() const
{
    _float4x4 W{}; XMStoreFloat4x4(&W, m_pTransformCom->Get_WorldMatrix()); return W;
}

// ===== 생성/클론 =====
CBoss_Hand_R* CBoss_Hand_R::Create(ID3D11Device* d, ID3D11DeviceContext* c)
{
    auto* p = new CBoss_Hand_R(d, c);
    if (FAILED(p->Initialize_Prototype())) { MSG_BOX(L"Failed to Created : CBoss_Hand_R"); Safe_Release(p); }
    return p;
}
CGameObject* CBoss_Hand_R::Clone(void* pArg)
{
    auto* p = new CBoss_Hand_R(*this);
    if (FAILED(p->Initialize(pArg))) { MSG_BOX(L"Failed to Clone : CBoss_Hand_R"); Safe_Release(p); }
    return p;
}
