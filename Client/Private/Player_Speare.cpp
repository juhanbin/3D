#include "Player_Speare.h"
#include "GameInstance.h"
#include "Object_Pool_Manager.h"

using namespace DirectX;
using namespace Client;

#define SPEAR_TIP_IS_UP 1

CPlayer_Speare::CPlayer_Speare(ID3D11Device* dev, ID3D11DeviceContext* ctx)
    : CGameObject(dev, ctx) {
}

CPlayer_Speare::CPlayer_Speare(const CPlayer_Speare& rhs)
    : CGameObject(rhs) {
}

HRESULT CPlayer_Speare::Initialize(void* pArg)
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

void CPlayer_Speare::Priority_Update(_float dt)
{
    __super::Priority_Update(dt);
    if (!Is_Active()) return;
}

void CPlayer_Speare::Update(_float dt)
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

void CPlayer_Speare::Late_Update(_float dt)
{
    __super::Late_Update(dt);
    if (!Is_Active()) return;

    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;
}

HRESULT CPlayer_Speare::Render()
{
    if (!Is_Active()) return S_OK;

    if (FAILED(Bind_ShaderResources())) return E_FAIL;

    const _uint n = m_pModelCom->Get_NumMeshes();
    for (_uint i = 0; i < n; ++i)
    {
        m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, 0, 0);
        m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_NormalTexture", i, 1, 0);

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

void CPlayer_Speare::Reuse_Begin(void* pArg)
{
    using namespace DirectX;

    Set_Active(true);
    if (pArg) m_desc = *reinterpret_cast<DESC*>(pArg);

    // 카메라 기준 스폰
    auto V = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW));
    auto Vinv = XMMatrixInverse(nullptr, V);
    XMVECTOR eye = Vinv.r[3];
    XMVECTOR fwd = XMVector3Normalize(Vinv.r[2]); // LH +Z
    XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    XMVECTOR spawn = eye + fwd * 3.0f;
    m_pTransformCom->Set_State(Engine::STATE::POSITION, spawn);

    // 기본 조준 방향(카메라 전방에 약간 위 각도)
    const float pitchDeg = 10.f;
    XMVECTOR aimDir = XMVector3Normalize(fwd + up * tanf(XMConvertToRadians(pitchDeg)));

    // --- 디버그 정지 모드 판단
    const bool freeze = (m_desc.speed == 0.f && m_desc.gravity == 0.f);

    if (freeze)
    {
        // 속도 0, 위치 고정. 회전만 방향에 맞춰 세팅
        m_vel = { 0,0,0 };
        // 수명 길게(원하면)
        if (m_desc.maxLife < 10.f) m_life = 999.f; else m_life = m_desc.maxLife;

        // 진행방향(aimDir) 바라보게 1회 정렬
        // AlignToVelocity는 속도 0이면 회전 안 하니, 같은 로직을 dir로 한 번 세팅
        // ─ 간단히 FaceDir 사용 (모델 축이 +Z/ +Y 다를 땐 기존 AlignToVelocity 매핑처럼 쓰면 됨)
        XMFLOAT3 d; XMStoreFloat3(&d, aimDir);
        FaceDir(d);

        m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
        return;
    }

    // --- 정상 발사 모드: 전달받은 speed/gravity 존중
    const float speed = (m_desc.speed > 0.f ? m_desc.speed : 35.f);
    m_vel = { XMVectorGetX(aimDir) * speed,
              XMVectorGetY(aimDir) * speed,
              XMVectorGetZ(aimDir) * speed };
    m_life = (m_desc.maxLife > 0.f ? m_desc.maxLife : 4.f);

    // 첫 프레임부터 진행방향 보도록
    AlignToVelocity();
    m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
}


void CPlayer_Speare::Reuse_End()
{
}

void CPlayer_Speare::FaceDir(const XMFLOAT3& dir)
{
    XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&dir));
    float yaw = atan2f(XMVectorGetX(d), XMVectorGetZ(d));
    float pitch = -asinf(XMVectorGetY(d));
    m_pTransformCom->RotationKeepPos(pitch, yaw, 0.f);
}

void CPlayer_Speare::Tick_Move(float dt)
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


bool CPlayer_Speare::Check_Hit()
{
    // TODO: CCD 스윕 충돌 or 콜라이더 체크
    return false;
}

void CPlayer_Speare::ReturnToPool()
{
    Set_Active(false);
    CObject_Pool_Manager::GetInstance()->Release(LEVEL::GAMEPLAY, L"Layer_Spear", this);
}

HRESULT CPlayer_Speare::Ready_Components()
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

    Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Spear_Static"),
        TEXT("Com_Model"), (CComponent**)&m_pModelCom, nullptr);

    CBounding_Sphere::BOUNDING_SPHERE_DESC  SphereDesc{};
    SphereDesc.fRadius = 0.1f;
    SphereDesc.vCenter = _float3(0.f, SphereDesc.fRadius + 0.6f, 0.f);

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &SphereDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CPlayer_Speare::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;

    const LIGHT_DESC* pLightDesc = m_pGameInstance->Get_LightDesc(0);
    if (nullptr == pLightDesc)
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDir", &pLightDesc->vDirection, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &pLightDesc->vDiffuse, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightAmbient", &pLightDesc->vAmbient, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightSpecular", &pLightDesc->vSpecular, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4)))) return E_FAIL;

    return S_OK;
}

CPlayer_Speare* CPlayer_Speare::Create(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    auto* p = new CPlayer_Speare(dev, ctx);
    if (FAILED(p->Initialize(nullptr)))
    {
        Safe_Release(p);
        return nullptr;
    }
    return p;
}

CGameObject* CPlayer_Speare::Clone(void* pArg)
{
    auto* p = new CPlayer_Speare(*this);
    if (FAILED(p->Initialize(pArg))) {
        MSG_BOX(TEXT("Failed to Clone : CPlayer_Speare"));
        Safe_Release(p);
    }
    return p;
}

void CPlayer_Speare::Free()
{
    __super::Free();
    Safe_Release(m_pModelCom);
    Safe_Release(m_pColliderCom);
    Safe_Release(m_pShaderCom);
}

// 현재 Transform의 스케일 추출(각 축 벡터 길이)
XMFLOAT3 CPlayer_Speare::GetCurrentScale() const
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
void CPlayer_Speare::AlignToVelocity()
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
