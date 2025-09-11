#include "Boss_Fire.h"
#include "GameInstance.h"
#include "Object_Pool_Manager.h"
#include "PlayerManager.h"

using namespace DirectX;
using namespace Client;

CBoss_Fire::CBoss_Fire(ID3D11Device* dev, ID3D11DeviceContext* ctx)
    : CGameObject(dev, ctx) {
}

CBoss_Fire::CBoss_Fire(const CBoss_Fire& rhs)
    : CGameObject(rhs) {
}

HRESULT CBoss_Fire::Initialize(void* pArg)
{
    GAMEOBJECT_DESC desc{};
    desc.fSpeedPerSec = 0.f;
    desc.fRotationPerSec = 0.f;

    if (FAILED(__super::Initialize(&desc)))   return E_FAIL;
    if (FAILED(Ready_Components()))           return E_FAIL;

    m_pTransformCom->Scaling((_float3(2.f, 2.f, 2.f)));
    return S_OK;
}

void CBoss_Fire::Priority_Update(_float dt)
{
    __super::Priority_Update(dt);
    if (!Is_Active()) return;
}

void CBoss_Fire::Update(_float dt)
{
    __super::Update(dt);
    if (!Is_Active()) return;

    Tick_Move(dt);

    // 수명 감소 및 종료
    m_life -= dt;
    if (m_life <= 0.f) { ReturnToPool(); return; }

    if (m_pColliderCom) m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
}

void CBoss_Fire::Late_Update(_float dt)
{
    __super::Late_Update(dt);
    if (!Is_Active()) return;

    // 디버그 렌더링 그룹
    m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this);

#ifdef _DEBUG
    m_pGameInstance->Add_DebugComponent(m_pColliderCom);
#endif
}

HRESULT CBoss_Fire::Render()
{
    if (!Is_Active()) return S_OK;

    if (FAILED(Bind_ShaderResources())) return E_FAIL;

    const _uint n = m_pModelCom->Get_NumMeshes();
    for (_uint i = 0; i < n; ++i)
    {
        m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, 0, 0);
        //m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_NormalTexture", i, 1, 0);
        m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix");
        m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW));
        m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ));
        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }

    return S_OK;
}

void CBoss_Fire::Reuse_Begin(void* pArg)
{
    Set_Active(true);
    if (pArg) m_desc = *reinterpret_cast<DESC*>(pArg);

    m_life = (m_desc.maxLife > 0.f ? m_desc.maxLife : 3.f);
    if (m_desc.gravity == 0.f) m_desc.gravity = -9.8f;

    // 1) 위치 세팅
    if (fabsf(m_desc.pos.x) + fabsf(m_desc.pos.y) + fabsf(m_desc.pos.z) > 1e-6f)
        m_pTransformCom->Set_State(Engine::STATE::POSITION, XMLoadFloat3(&m_desc.pos));

    // 2) dir이 0이면 → 플레이어 쪽으로 자동 조준
    XMVECTOR dir;
    if (fabsf(m_desc.dir.x) + fabsf(m_desc.dir.y) + fabsf(m_desc.dir.z) > 1e-6f) {
        dir = XMVector3Normalize(XMLoadFloat3(&m_desc.dir));
    }
    else {
        _float3 player{};
        if (auto* pm = CPlayerManager::GetInstance())
            XMStoreFloat3(&player, pm->GetPos());
        else
            player = _float3{ 0,0,1 };

        XMVECTOR P = m_pTransformCom->Get_State(Engine::STATE::POSITION);
        XMVECTOR T = XMLoadFloat3(&player);
        // 살짝 위(플레이어 가슴/머리 높이)
        T = T + XMVectorSet(0.f, 1.2f, 0.f, 0.f);
        dir = XMVector3Normalize(T - P);
    }

    // 3) 속도 벡터
    const float speed = (m_desc.speed > 0.f ? m_desc.speed : 35.f);
    XMStoreFloat3(&m_vel, dir * speed);

    // 4) 정렬 & 콜라이더 초기화
    AlignToVelocity();
    if (m_pColliderCom) m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
}

void CBoss_Fire::Reuse_End()
{
    // 풀 반납 직전 정리할 게 있으면 여기서
}

void CBoss_Fire::Tick_Move(float dt)
{
    // 중력 적용
    m_vel.y += m_desc.gravity * dt;

    const _vector pos = m_pTransformCom->Get_State(Engine::STATE::POSITION);
    const _vector dv = XMVectorSet(m_vel.x * dt, m_vel.y * dt, m_vel.z * dt, 0.f);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, pos + dv);

    AlignToVelocity();
}

bool CBoss_Fire::Check_Hit()
{
    // 지금은 Player 쪽( CPlayer::TickHostileHits )에서 레이어 스캔으로 처리.
    return false;
}

void CBoss_Fire::ReturnToPool()
{
    Set_Active(false);
    CObject_Pool_Manager::GetInstance()->Release(LEVEL::GAMEPLAY, L"Layer_Boss_Fire", this);
}

HRESULT CBoss_Fire::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    // 보스 파이어 모델 프리팹명은 프로젝트에 맞춰 조정
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Boss_Fire"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;

    // 콜라이더(조금 넉넉하게)
    CBounding_Sphere::BOUNDING_SPHERE_DESC S{};
    S.fRadius = 0.25f;
    S.vCenter = _float3(0.f, 0.25f, 0.f);

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &S)))
        return E_FAIL;

    return S_OK;
}

HRESULT CBoss_Fire::Bind_ShaderResources()
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

CBoss_Fire* CBoss_Fire::Create(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    auto* p = new CBoss_Fire(dev, ctx);
    if (FAILED(p->Initialize(nullptr))) { Safe_Release(p); return nullptr; }
    return p;
}

CGameObject* CBoss_Fire::Clone(void* pArg)
{
    auto* p = new CBoss_Fire(*this);
    if (FAILED(p->Initialize(pArg))) { MSG_BOX(TEXT("Failed to Clone : CBoss_Fire")); Safe_Release(p); }
    return p;
}

void CBoss_Fire::Free()
{
    __super::Free();
    Safe_Release(m_pModelCom);
    Safe_Release(m_pColliderCom);
    Safe_Release(m_pShaderCom);
}

DirectX::XMFLOAT3 CBoss_Fire::GetCurrentScale() const
{
    XMVECTOR R = m_pTransformCom->Get_State(Engine::STATE::RIGHT);
    XMVECTOR U = m_pTransformCom->Get_State(Engine::STATE::UP);
    XMVECTOR L = m_pTransformCom->Get_State(Engine::STATE::LOOK);
    float sx = XMVectorGetX(XMVector3Length(R));
    float sy = XMVectorGetX(XMVector3Length(U));
    float sz = XMVectorGetX(XMVector3Length(L));
    return XMFLOAT3(sx, sy, sz);
}

void CBoss_Fire::AlignToVelocity()
{
    XMVECTOR v = XMVectorSet(m_vel.x, m_vel.y, m_vel.z, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(v)) < 1e-6f) return;

    XMVECTOR look = XMVector3Normalize(v);
    XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    XMVECTOR prevRight = XMVector3Normalize(m_pTransformCom->Get_State(Engine::STATE::RIGHT));
    float parallel = fabsf(XMVectorGetX(XMVector3Dot(look, worldUp)));
    XMVECTOR upRef = (parallel > 0.99f) ? prevRight : worldUp;

    XMVECTOR right = XMVector3Normalize(XMVector3Cross(upRef, look));
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(look, right));

    XMFLOAT3 s = GetCurrentScale();
    _vector   p = m_pTransformCom->Get_State(Engine::STATE::POSITION);

    XMVECTOR R = right * s.x;
    XMVECTOR U = up * s.y;
    XMVECTOR L = look * s.z;

    m_pTransformCom->Set_State(Engine::STATE::RIGHT, R);
    m_pTransformCom->Set_State(Engine::STATE::UP, U);
    m_pTransformCom->Set_State(Engine::STATE::LOOK, L);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, p);
}

void CBoss_Fire::FaceDir(const XMFLOAT3& dir)
{
    XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&dir));
    float yaw = atan2f(XMVectorGetX(d), XMVectorGetZ(d));
    float pitch = -asinf(XMVectorGetY(d));
    m_pTransformCom->RotationKeepPos(pitch, yaw, 0.f);
}
