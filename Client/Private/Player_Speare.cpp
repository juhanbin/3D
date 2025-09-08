#include "Player_Speare.h"
#include "GameInstance.h"
#include "Object_Pool_Manager.h"

CPlayer_Speare::CPlayer_Speare(ID3D11Device* dev, ID3D11DeviceContext* ctx)
    : CGameObject(dev, ctx) {
}

CPlayer_Speare::CPlayer_Speare(const CPlayer_Speare& rhs)
    : CGameObject(rhs) 
{
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

    // 초기 스케일 (위치 보존)
    //m_pTransformCom->Scaling(_float3(5.f, 5.f, 5.f));
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

   /* m_life -= dt;
    if (m_life <= 0.f) { ReturnToPool(); return; }*/

    Tick_Move(dt);


    //if (Check_Hit()) {
    //    // 데미지/이펙트
    //    ReturnToPool();
    //}

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

        if (FAILED(m_pShaderCom->Begin(0))) OutputDebugStringA("SPEAR: Begin(0) fail\n");
        if (FAILED(m_pModelCom->Render(i))) OutputDebugStringA("SPEAR: Model->Render fail\n");
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

    // 카메라 앞 3m 스폰 (지금 잘 보이는 버전 유지)
    auto V = XMLoadFloat4x4(m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW));
    auto Vinv = XMMatrixInverse(nullptr, V);
    XMVECTOR eye = Vinv.r[3];                 // 카메라 월드 위치
    XMVECTOR fwd = XMVector3Normalize(Vinv.r[2]); // LH: +Z가 전방
    XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    XMVECTOR spawn = eye + fwd * 3.0f;
    m_pTransformCom->Set_State(Engine::STATE::POSITION, spawn);

    // --- 초기 속도 설정 (조금 위로 각도 주기)
    const float speed = (m_desc.speed > 0.f ? m_desc.speed : 30.f);
    const float pitchDeg = 10.f;                           // 위로 10도 쏘기
    const float pitchRad = XMConvertToRadians(pitchDeg);

    XMVECTOR aimDir = XMVector3Normalize(fwd + up * tanf(pitchRad));

    m_vel = { XMVectorGetX(aimDir) * speed,
              XMVectorGetY(aimDir) * speed,
              XMVectorGetZ(aimDir) * speed };

    // 수명
    m_life = (m_desc.maxLife > 0.f ? m_desc.maxLife : 4.f);

    // 처음 방향 정렬(보기 좋게)
    XMFLOAT3 d; XMStoreFloat3(&d, aimDir);
    FaceDir(d);

    m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
}



void CPlayer_Speare::Reuse_End()
{
}

void CPlayer_Speare::FaceDir(const XMFLOAT3& dir)
{
    using namespace DirectX;
    XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&dir));
    float yaw = atan2f(XMVectorGetX(d), XMVectorGetZ(d));
    float pitch = -asinf(XMVectorGetY(d));

    // 위치 보존 회전
    m_pTransformCom->RotationKeepPos(pitch, yaw, 0.f);
}

void CPlayer_Speare::Tick_Move(float dt)
{
    // 중력 먼저 적용
    m_vel.y += m_desc.gravity * dt;

    // 속도 적분
    const _vector pos = m_pTransformCom->Get_State(Engine::STATE::POSITION);
    const _vector dv = XMVectorSet(m_vel.x * dt, m_vel.y * dt, m_vel.z * dt, 0.f);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, pos + dv);

    // 진행방향으로 모델 돌려주면 더 자연스러움(선택)
    XMVECTOR v = XMVector3Normalize(dv);
    if (XMVectorGetX(XMVector3LengthSq(v)) > 1e-6f) {
        XMFLOAT3 face; XMStoreFloat3(&face, v);
        FaceDir(face);
    }
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

    CBounding_AABB::BOUNDING_AABB_DESC  AABBDesc{};
    AABBDesc.vExtents = _float3(0.4f, 0.7f, 0.4f);
    AABBDesc.vCenter = _float3(0.f, AABBDesc.vExtents.y, 0.f);


    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_AABB"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &AABBDesc)))
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

    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDir", &pLightDesc->vDirection, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &pLightDesc->vDiffuse, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightAmbient", &pLightDesc->vAmbient, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightSpecular", &pLightDesc->vSpecular, sizeof(_float4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4))))
        return E_FAIL;

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