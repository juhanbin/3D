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
    m_pTransformCom->Scaling(_float3(5.f, 5.f, 5.f));

    return S_OK;
}

void CPlayer_Speare::Priority_Update(_float dt)
{
    __super::Priority_Update(dt);
    //if (!Is_Active()) return;
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
    //if (!Is_Active()) return;
#ifdef _DEBUG
    if (Is_Active())
    {
        XMFLOAT4X4 m; XMStoreFloat4x4(&m, m_pTransformCom->Get_WorldMatrix());
        char b[128]; sprintf_s(b, "[SPEAR] Late pos=(%.2f,%.2f,%.2f)\n", m._41, m._42, m._43);
        OutputDebugStringA(b);
    }
#endif
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;
}
HRESULT CPlayer_Speare::Render()
{
    if (!Is_Active()) return S_OK;
    if (FAILED(Bind_ShaderResources())) return E_FAIL;

    // --- 상태 백업
    ID3D11RasterizerState* oldRS = nullptr;
    ID3D11DepthStencilState* oldDS = nullptr;
    UINT oldStencilRef = 0;
    m_pContext->RSGetState(&oldRS);
    m_pContext->OMGetDepthStencilState(&oldDS, &oldStencilRef);

    // --- Cull None + Depth Enable
    ID3D11RasterizerState* rsCullNone = nullptr;
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;        // 뒤집힘/와인딩 이슈 회피
    rd.DepthClipEnable = TRUE;
    m_pDevice->CreateRasterizerState(&rd, &rsCullNone);
    m_pContext->RSSetState(rsCullNone);

    ID3D11DepthStencilState* dsDefault = nullptr;
    D3D11_DEPTH_STENCIL_DESC dd{};
    dd.DepthEnable = TRUE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    m_pDevice->CreateDepthStencilState(&dd, &dsDefault);
    m_pContext->OMSetDepthStencilState(dsDefault, 0);

    const _uint n = m_pModelCom->Get_NumMeshes();
    for (_uint i = 0; i < n; ++i)
    {
        m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, 0, 0);
        m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_NormalTexture", i, 1, 0);

        m_pTransformCom->Bind_Shader_Resource(m_pShaderCom, "g_WorldMatrix");
        m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW));
        m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ));
        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }

#ifdef _DEBUG
    m_pColliderCom->Render();
#endif

    // --- 상태 복원
    m_pContext->OMSetDepthStencilState(oldDS, oldStencilRef);
    m_pContext->RSSetState(oldRS);
    Safe_Release(dsDefault);
    Safe_Release(rsCullNone);
    Safe_Release(oldDS);
    Safe_Release(oldRS);
    return S_OK;
}



void CPlayer_Speare::Reuse_Begin(void* pArg)
{
    Set_Active(true);
    if (pArg) m_desc = *reinterpret_cast<DESC*>(pArg);

    using namespace DirectX;
    XMStoreFloat3(&m_desc.dir, XMVector3Normalize(XMLoadFloat3(&m_desc.dir)));

    m_life = m_desc.maxLife;
    m_vel = { m_desc.dir.x * m_desc.speed, m_desc.dir.y * m_desc.speed, m_desc.dir.z * m_desc.speed };

    // 0) 스케일 값
    const _float3 S = _float3(5.f, 5.f, 5.f);

    // 1) ★ 월드 기저 완전 리셋(스케일된 단위축)
    m_pTransformCom->Set_State(Engine::STATE::RIGHT, XMVectorSet(1.f, 0.f, 0.f, 0.f) * S.x);
    m_pTransformCom->Set_State(Engine::STATE::UP, XMVectorSet(0.f, 1.f, 0.f, 0.f) * S.y);
    m_pTransformCom->Set_State(Engine::STATE::LOOK, XMVectorSet(0.f, 0.f, 1.f, 0.f) * S.z);

    // 2) 위치
    m_pTransformCom->Set_State(Engine::STATE::POSITION, XMLoadFloat3(&m_desc.pos));

    // 3) 회전(위치 보존)
    FaceDir(m_desc.dir); // 내부에서 RotationKeepPos 사용

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
    m_vel.y += m_desc.gravity * dt;
    const _vector pos = m_pTransformCom->Get_State(Engine::STATE::POSITION);
    const _vector dv = XMVectorSet(m_vel.x * dt, m_vel.y * dt, m_vel.z * dt, 0.f);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, pos + dv);
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