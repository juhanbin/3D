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
    desc.fSpeedPerSec = 0.f;     // 필요 시
    desc.fRotationPerSec = 0.f;     // 필요 시

    if (FAILED(__super::Initialize(&desc)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    m_pTransformCom->Scaling(_float3(5.f, 5.f, 5.f));

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

    m_life -= dt;
    if (m_life <= 0.f) { ReturnToPool(); return; }

    Tick_Move(dt);


    if (Check_Hit()) {
        // 데미지/이펙트
        ReturnToPool();
    }

    m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
}

void CPlayer_Speare::Late_Update(_float dt)
{
    __super::Late_Update(dt);
    if (!Is_Active()) return;
#ifdef _DEBUG
    //OutputDebugStringW(L"[SPEAR] Late_Update -> RenderGroup 등록\n");
#endif
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;
}

HRESULT CPlayer_Speare::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (size_t i = 0; i < iNumMeshes; i++)
    {
        // BIN 데이터니까 직접 type 인덱스(0=Diffuse, 1=Normal)로 바인딩
        if (FAILED(m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, 0, 0))) // 0: DIFFUSE
        {
            OutputDebugStringA("d머티리얼 바인딩 실패!\n");
            return E_FAIL;
        }


        //if (FAILED(m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_NormalTexture", i, 1, 0))) // 1: NORMAL
        //{
        //    OutputDebugStringA("n머티리얼 바인딩 실패!\n");
        //    return E_FAIL;
        //}

        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }

#ifdef _DEBUG
    m_pColliderCom->Render();
#endif

    return S_OK;
}


void CPlayer_Speare::Reuse_Begin(void* pArg)
{
    if (pArg) m_desc = *reinterpret_cast<DESC*>(pArg);

    using namespace DirectX;
    XMVECTOR dnorm = XMVector3Normalize(XMLoadFloat3(&m_desc.dir));
    XMStoreFloat3(&m_desc.dir, dnorm);

    m_life = m_desc.maxLife;
    m_vel = { m_desc.dir.x * m_desc.speed, m_desc.dir.y * m_desc.speed, m_desc.dir.z * m_desc.speed };

    m_pTransformCom->Set_State(Engine::STATE::POSITION, XMLoadFloat3(&m_desc.pos));
    FaceDir(m_desc.dir); 


#ifdef _DEBUG
    wchar_t buf[128];
    swprintf(buf, 128, L"[SPEAR] Reuse_Begin pos=(%.2f,%.2f,%.2f)\n", m_desc.pos.x, m_desc.pos.y, m_desc.pos.z);
    OutputDebugStringW(buf);
#endif
}

void CPlayer_Speare::Reuse_End()
{
}

void CPlayer_Speare::FaceDir(const XMFLOAT3& dir)
{
    using namespace DirectX;
    XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&dir));
    float yaw = atan2f(XMVectorGetX(d), XMVectorGetZ(d)); // Y
    float pitch = -asinf(XMVectorGetY(d));                  // X
    m_pTransformCom->Rotation(pitch, yaw, 0.f);
}

void CPlayer_Speare::Tick_Move(float dt)
{
    m_desc.gravity; 
    m_vel.y += m_desc.gravity * dt;

    XMVECTOR pos = m_pTransformCom->Get_State(Engine::STATE::POSITION);
    XMVECTOR dv = XMVectorSet(m_vel.x * dt, m_vel.y * dt, m_vel.z * dt, 0);
    m_pTransformCom->Set_State(Engine::STATE::POSITION, pos + dv);
}

bool CPlayer_Speare::Check_Hit()
{
    // TODO: CCD 스윕 충돌 or 콜라이더 체크
    return false;
}

void CPlayer_Speare::ReturnToPool()
{
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