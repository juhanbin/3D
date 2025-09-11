#include "Player_Speare.h"
#include "GameInstance.h"
#include "Object_Pool_Manager.h"
#include "Mushroom.h"

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
    if (FAILED(__super::Initialize(&desc))) return E_FAIL;
    if (FAILED(Ready_Components())) return E_FAIL;
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

    Tick_Move(dt);
    m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());

    // 창당 1회만 데미지
    if (Check_Hit()) return;

    // 수명 관리(선택)
    m_life -= dt;
    if (m_life <= 0.f) ReturnToPool();
}

void CPlayer_Speare::Late_Update(_float dt)
{
    __super::Late_Update(dt);
    if (!Is_Active()) return;

    m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this);

#ifdef _DEBUG
    m_pGameInstance->Add_DebugComponent(m_pColliderCom);
#endif
}

HRESULT CPlayer_Speare::Render()
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

void CPlayer_Speare::Reuse_Begin(void* pArg)
{
    using namespace DirectX;

    Set_Active(true);
    m_hasDealtDamage = false;           // 창 1개당 1회 데미지 리셋

    // 전달된 desc는 속도/중력/수명만 참고(위치/방향은 무시하고 카메라 기준으로 계산)
    if (pArg) m_desc = *reinterpret_cast<DESC*>(pArg);

    // --- 카메라 기준 스폰/방향(원래 방식) ---
    auto V = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW));
    auto Vinv = XMMatrixInverse(nullptr, V);
    XMVECTOR eye = Vinv.r[3];                          // 카메라 위치
    XMVECTOR fwd = XMVector3Normalize(Vinv.r[2]);      // 카메라 +Z (LH)
    XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    // 카메라 앞쪽으로 조금 떨어진 지점에서 생성
    XMVECTOR spawn = eye + fwd * 3.0f;
    m_pTransformCom->Set_State(Engine::STATE::POSITION, spawn);

    // 살짝 고각(피치) 주기: 기본 10도
    const float pitchDeg = 10.f;
    XMVECTOR aimDir = XMVector3Normalize(fwd + up * tanf(XMConvertToRadians(pitchDeg)));

    // --- 디버그/정지용: 속도/중력 둘 다 0이면 정지 모드 ---
    const bool freeze = (m_desc.speed == 0.f && m_desc.gravity == 0.f);
    if (freeze)
    {
        m_vel = { 0,0,0 };
        m_life = (m_desc.maxLife > 0.f ? m_desc.maxLife : 999.f);

        // 진행방향만 보도록 1회 정렬
        XMFLOAT3 d; XMStoreFloat3(&d, aimDir);
        FaceDir(d);

        m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
        return;
    }

    // --- 정상 발사: 속도/중력/수명 적용 ---
    const float speed = (m_desc.speed > 0.f ? m_desc.speed : 35.f);
    m_vel = { XMVectorGetX(aimDir) * speed,
              XMVectorGetY(aimDir) * speed,
              XMVectorGetZ(aimDir) * speed };

    m_life = (m_desc.maxLife > 0.f ? m_desc.maxLife : 4.f);

    AlignToVelocity();
    m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
}


void CPlayer_Speare::Reuse_End() {}

void CPlayer_Speare::FaceDir(const XMFLOAT3& dir)
{
    XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&dir));
    float yaw = atan2f(XMVectorGetX(d), XMVectorGetZ(d));
    float pitch = -asinf(XMVectorGetY(d));
    m_pTransformCom->RotationKeepPos(pitch, yaw, 0.f);
}

void CPlayer_Speare::Tick_Move(float dt)
{
    m_vel.y += m_desc.gravity * dt;

    const _vector pos = m_pTransformCom->Get_State(Engine::STATE::POSITION);
    const _vector dv = XMVectorSet(m_vel.x * dt, m_vel.y * dt, m_vel.z * dt, 0.f);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, pos + dv);

    AlignToVelocity();
}

bool CPlayer_Speare::Check_Hit()
{
    if (m_hasDealtDamage) return false;

    const _uint level = ENUM_CLASS(LEVEL::GAMEPLAY);
    const std::wstring layer = L"Layer_Mushroom";

    for (_uint i = 0;; ++i)
    {
        CGameObject* obj = m_pGameInstance->Find_GameObject(level, layer, i);
        if (!obj) break;

        auto* mush = dynamic_cast<CMushroom*>(obj);
        if (!mush || !mush->IsAlive()) continue;

        Engine::CCollider* targetCol = mush->GetCollider_Block();
        if (!targetCol) continue;

        if (m_pColliderCom->Intersect(targetCol))
        {
            mush->TakeDamage(m_damage, this); // 1회 데미지
            m_hasDealtDamage = true;
            ReturnToPool();                    // 관통 원하면 이 줄 주석
            return true;
        }
    }
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
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr))) return E_FAIL;

    Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Spear_Static"),
        TEXT("Com_Model"), (CComponent**)&m_pModelCom, nullptr);

    CBounding_Sphere::BOUNDING_SPHERE_DESC  SphereDesc{};
    SphereDesc.fRadius = 0.1f;
    SphereDesc.vCenter = _float3(0.f, SphereDesc.fRadius + 0.6f, 0.f);

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &SphereDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CPlayer_Speare::Bind_ShaderResources()
{
    if (FAILED(m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix")))  return E_FAIL;
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

CPlayer_Speare* CPlayer_Speare::Create(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
    auto* p = new CPlayer_Speare(dev, ctx);
    if (FAILED(p->Initialize(nullptr))) { Safe_Release(p); return nullptr; }
    return p;
}

CGameObject* CPlayer_Speare::Clone(void* pArg)
{
    auto* p = new CPlayer_Speare(*this);
    if (FAILED(p->Initialize(pArg))) { MSG_BOX(TEXT("Failed to Clone : CPlayer_Speare")); Safe_Release(p); }
    return p;
}

void CPlayer_Speare::Free()
{
    __super::Free();
    Safe_Release(m_pModelCom);
    Safe_Release(m_pColliderCom);
    Safe_Release(m_pShaderCom);
}

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

void CPlayer_Speare::AlignToVelocity()
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

#if SPEAR_TIP_IS_UP
    XMVECTOR R = right * s.x;
    XMVECTOR U = look * s.y;
    XMVECTOR L = XMVectorNegate(up) * s.z;
#else
    XMVECTOR R = right * s.x;
    XMVECTOR U = up * s.y;
    XMVECTOR L = look * s.z;
#endif

    m_pTransformCom->Set_State(Engine::STATE::RIGHT, R);
    m_pTransformCom->Set_State(Engine::STATE::UP, U);
    m_pTransformCom->Set_State(Engine::STATE::LOOK, L);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, p);
}
