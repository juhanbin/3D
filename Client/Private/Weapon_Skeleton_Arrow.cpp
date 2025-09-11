#include "Weapon_Skeleton_Arrow.h"
#include "GameInstance.h"
#include "Object_Pool_Manager.h"

using namespace DirectX;
using namespace Client;

#define SPEAR_TIP_IS_UP 1

CWeapon_Skeleton_Arrow::CWeapon_Skeleton_Arrow(ID3D11Device* dev, ID3D11DeviceContext* ctx)
    : CGameObject(dev, ctx) {
}

CWeapon_Skeleton_Arrow::CWeapon_Skeleton_Arrow(const CWeapon_Skeleton_Arrow& rhs)
    : CGameObject(rhs) {
}

HRESULT CWeapon_Skeleton_Arrow::Initialize(void* pArg)
{
    GAMEOBJECT_DESC desc{};
    desc.fSpeedPerSec = 0.f;
    desc.fRotationPerSec = 0.f;

    if (FAILED(__super::Initialize(&desc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    // 스케일은 외부/모델 기본값 유지
    // m_pTransformCom->Scaling(_float3(5.f, 5.f, 5.f));
    return S_OK;
}

void CWeapon_Skeleton_Arrow::Priority_Update(_float dt)
{
    __super::Priority_Update(dt);
    if (!Is_Active()) return;
}

void CWeapon_Skeleton_Arrow::Update(_float dt)
{
    __super::Update(dt);
    if (!Is_Active()) return;

    // 남은 수명 관리가 필요하면 해제
    // m_life -= dt;
    // if (m_life <= 0.f) { ReturnToPool(); return; }

    Tick_Move(dt);

    // if (Check_Hit()) { ReturnToPool(); }

    m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
}

void CWeapon_Skeleton_Arrow::Late_Update(_float dt)
{
    __super::Late_Update(dt);
    if (!Is_Active()) return;

    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;

    m_pGameInstance->Add_DebugComponent(m_pColliderCom);
}

HRESULT CWeapon_Skeleton_Arrow::Render()
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

        if (FAILED(m_pShaderCom->Begin(0)))  OutputDebugStringA("SPEAR: Begin(0) fail\n");
        if (FAILED(m_pModelCom->Render(i)))   OutputDebugStringA("SPEAR: Model->Render fail\n");
    }

#ifdef _DEBUG
    m_pColliderCom->Render();
#endif
    return S_OK;
}

void CWeapon_Skeleton_Arrow::Reuse_Begin(void* pArg)
{
    Set_Active(true);
    if (pArg) m_desc = *reinterpret_cast<DESC*>(pArg);

    // 0) 기본 수명
    m_life = (m_desc.maxLife > 0.f ? m_desc.maxLife : 4.f);

    // 1) pos/dir이 유효하면 그것을 사용
    const bool hasDir =
        (fabsf(m_desc.dir.x) > 1e-6f) ||
        (fabsf(m_desc.dir.y) > 1e-6f) ||
        (fabsf(m_desc.dir.z) > 1e-6f);

    if (hasDir)
    {
        XMVECTOR spawn = XMLoadFloat3(&m_desc.pos);
        XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&m_desc.dir));

        m_pTransformCom->Set_State(Engine::STATE::POSITION, spawn);

        const float speed = (m_desc.speed > 0.f ? m_desc.speed : 35.f);
        XMFLOAT3 vel;
        XMStoreFloat3(&vel, dir * speed);
        m_vel = vel;
    }
    else
    {
        // 2) 폴백: 카메라 기준(지금 네 코드)
        auto V = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW));
        auto Vinv = XMMatrixInverse(nullptr, V);
        XMVECTOR eye = Vinv.r[3];
        XMVECTOR fwd = XMVector3Normalize(Vinv.r[2]); // LH +Z
        XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

        XMVECTOR spawn = eye + fwd * 3.0f;
        m_pTransformCom->Set_State(Engine::STATE::POSITION, spawn);

        const float pitchDeg = 10.f;
        XMVECTOR aimDir = XMVector3Normalize(fwd + up * tanf(XMConvertToRadians(pitchDeg)));

        const float speed = (m_desc.speed > 0.f ? m_desc.speed : 35.f);
        XMFLOAT3 vel; XMStoreFloat3(&vel, aimDir * speed);
        m_vel = vel;
    }

    // 3) 중력/수명 값 정리 (중력은 네 코드처럼 음수면 아래로)
    if (m_desc.gravity == 0.f) m_desc.gravity = -9.8f;

    // 4) 첫 프레임 정렬 + 콜라이더 갱신
    AlignToVelocity();
    if (m_pColliderCom) m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
}



void CWeapon_Skeleton_Arrow::Reuse_End()
{
}

void CWeapon_Skeleton_Arrow::FaceDir(const XMFLOAT3& dir)
{
    XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&dir));
    float yaw = atan2f(XMVectorGetX(d), XMVectorGetZ(d));
    float pitch = -asinf(XMVectorGetY(d));
    m_pTransformCom->RotationKeepPos(pitch, yaw, 0.f);
}

void CWeapon_Skeleton_Arrow::Tick_Move(float dt)
{
    // 정지 모드: 속도 0 + 중력 0 → 이동/회전 갱신 스킵
    if (m_desc.gravity == 0.f && m_vel.x == 0.f && m_vel.y == 0.f && m_vel.z == 0.f)
        return;

    // 정상 이동
    m_vel.y += m_desc.gravity * dt;

    const _vector pos = m_pTransformCom->Get_State(Engine::STATE::POSITION);
    const _vector dv = XMVectorSet(m_vel.x * dt, m_vel.y * dt, m_vel.z * dt, 0.f);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, pos + dv);

    AlignToVelocity();
}


bool CWeapon_Skeleton_Arrow::Check_Hit()
{
    // TODO: CCD 스윕 충돌 or 콜라이더 체크
    return false;
}

void CWeapon_Skeleton_Arrow::ReturnToPool()
{
    Set_Active(false);
    CObject_Pool_Manager::GetInstance()->Release(LEVEL::GAMEPLAY, L"Layer_Arrow", this);
}

HRESULT CWeapon_Skeleton_Arrow::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY),
        TEXT("Prototype_Component_Shader_VtxMesh"),
        TEXT("Com_Shader"),
        reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
    {
        OutputDebugStringA("Player_Spear: 쉐이더 Add_Component 실패!\n");
        return E_FAIL;
    }

    Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Monster_Arrow"),
        TEXT("Com_Model"), (CComponent**)&m_pModelCom, nullptr);

    CBounding_Sphere::BOUNDING_SPHERE_DESC  SphereDesc{};
    SphereDesc.fRadius = 0.1f;
    SphereDesc.vCenter = _float3(0.f, SphereDesc.fRadius + 0.6f, 0.f);

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &SphereDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CWeapon_Skeleton_Arrow::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;

    /*const LIGHT_DESC* pLightDesc = m_pGameInstance->Get_LightDesc(0);
    if (nullptr == pLightDesc)
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDir", &pLightDesc->vDirection, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &pLightDesc->vDiffuse, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightAmbient", &pLightDesc->vAmbient, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightSpecular", &pLightDesc->vSpecular, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4)))) return E_FAIL;*/

    return S_OK;
}

CWeapon_Skeleton_Arrow* CWeapon_Skeleton_Arrow::Create(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    auto* p = new CWeapon_Skeleton_Arrow(dev, ctx);
    if (FAILED(p->Initialize(nullptr)))
    {
        Safe_Release(p);
        return nullptr;
    }
    return p;
}

CGameObject* CWeapon_Skeleton_Arrow::Clone(void* pArg)
{
    auto* p = new CWeapon_Skeleton_Arrow(*this);
    if (FAILED(p->Initialize(pArg))) {
        MSG_BOX(TEXT("Failed to Clone : CWeapon_Skeleton_Arrow"));
        Safe_Release(p);
    }
    return p;
}

void CWeapon_Skeleton_Arrow::Free()
{
    __super::Free();
    Safe_Release(m_pModelCom);
    Safe_Release(m_pColliderCom);
    Safe_Release(m_pShaderCom);
}

// 현재 Transform의 스케일 추출(각 축 벡터 길이)
XMFLOAT3 CWeapon_Skeleton_Arrow::GetCurrentScale() const
{
    XMVECTOR R = m_pTransformCom->Get_State(Engine::STATE::RIGHT);
    XMVECTOR U = m_pTransformCom->Get_State(Engine::STATE::UP);
    XMVECTOR L = m_pTransformCom->Get_State(Engine::STATE::LOOK);

    float sx = XMVectorGetX(XMVector3Length(R));
    float sy = XMVectorGetX(XMVector3Length(U));
    float sz = XMVectorGetX(XMVector3Length(L));
    return XMFLOAT3(sx, sy, sz);
}

// 속도 벡터 방향으로 월드 회전 정렬 (스케일/위치 보존)
void CWeapon_Skeleton_Arrow::AlignToVelocity()
{
    XMVECTOR v = XMVectorSet(m_vel.x, m_vel.y, m_vel.z, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(v)) < 1e-6f) return;

    XMVECTOR look = XMVector3Normalize(v);                 // 우리가 원하는 '앞'
    XMVECTOR worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    // look이 worldUp과 거의 평행할 때 플립 방지
    XMVECTOR prevRight = XMVector3Normalize(m_pTransformCom->Get_State(Engine::STATE::RIGHT));
    float parallel = fabsf(XMVectorGetX(XMVector3Dot(look, worldUp)));
    XMVECTOR upRef = (parallel > 0.99f) ? prevRight : worldUp;

    XMVECTOR right = XMVector3Normalize(XMVector3Cross(upRef, look));
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(look, right));

    // 기존 스케일/포지션 보존
    XMFLOAT3 s = GetCurrentScale();
    _vector   p = m_pTransformCom->Get_State(Engine::STATE::POSITION);

#if SPEAR_TIP_IS_UP
    XMVECTOR R = right * s.x;
    XMVECTOR U = look * s.y;
    XMVECTOR L = XMVectorNegate(up) * s.z;
#else
    // 모델 +Z가 '앞'인 일반 케이스
    XMVECTOR R = right * s.x;
    XMVECTOR U = up * s.y;
    XMVECTOR L = look * s.z;
#endif

    m_pTransformCom->Set_State(Engine::STATE::RIGHT, R);
    m_pTransformCom->Set_State(Engine::STATE::UP, U);
    m_pTransformCom->Set_State(Engine::STATE::LOOK, L);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, p);
}
