#include "Body_Bridge.h"
#include "GameInstance.h"
#include <cmath>

CBody_Bridge::CBody_Bridge(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CPartObject{ pDevice, pContext } {
}

CBody_Bridge::CBody_Bridge(const CBody_Bridge& Prototype)
    : CPartObject{ Prototype } {
}

_float4x4* CBody_Bridge::Get_BoneMatrix(const _char* pBoneName)
{
    return m_pModelCom->Get_BoneMatrix(pBoneName);
}

HRESULT CBody_Bridge::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CBody_Bridge::Initialize(void* pArg)
{
    if (!pArg) return E_FAIL;

    BODY_DESC* pDesc = static_cast<BODY_DESC*>(pArg);
    m_pParentState = pDesc->pState;
    m_pMoving = pDesc->pMoving;
    m_pAttack = pDesc->pAttack;

    if (FAILED(__super::Initialize(pArg))) return E_FAIL;
    if (FAILED(Ready_Components()))       return E_FAIL;

    m_iCurAnim = -1;
    m_bCurLoop = true;
    m_fCurBlend = 0.f;

    m_LastLocoAnim = -1;
    m_LocoHold = 0.f;

    m_bDashPlaying = false;
    m_bGroundPlaying = false;

    m_fDashFinishBlock = 0.f;

    return S_OK;
}

void CBody_Bridge::Priority_Update(_float /*fTimeDelta*/)
{

}

void CBody_Bridge::Update(_float fTimeDelta)
{
    if (m_fDashFinishBlock > 0.f)
        m_fDashFinishBlock -= fTimeDelta;

    float playDt = fTimeDelta;
    if (m_pAttack && *m_pAttack == ATTACK::THROW && m_iCurAnim == 20)
        playDt = fTimeDelta * 2.f; // 투척만 가속

    const bool finished = m_pModelCom->Play_Animation(playDt);

    if (m_bDashPlaying && m_iCurAnim == 2 && finished) {
        m_bDashPlaying = false;
        m_fDashFinishBlock = 0.03f; // 같은 프레임 재트리거 방지
    }
    if (m_pAttack && *m_pAttack == ATTACK::ENTER && m_iCurAnim == 0 && finished)
        *m_pAttack = ATTACK::IDLE;
    if (m_pAttack && *m_pAttack == ATTACK::THROW && m_iCurAnim == 20 && finished)
        *m_pAttack = ATTACK::NONE;
    if (m_bGroundPlaying && m_iCurAnim == 7 && finished) {
        m_bGroundPlaying = false;
        if (m_pAttack) *m_pAttack = ATTACK::NONE;
    }

    int   nextAnim = -1;
    bool  nextLoop = true;
    bool  forceStart = false;

    const bool dashRequested = (m_pMoving && *m_pMoving == MOVING::DASH);

    if (m_bDashPlaying) {
        nextAnim = 2; nextLoop = false;
    }
    else if (dashRequested && m_fDashFinishBlock <= 0.f) {
        forceStart = true;
        m_bDashPlaying = true;
        nextAnim = 2; nextLoop = false;
    }
    else {
        if (m_pAttack && *m_pAttack == ATTACK::GROUND) {
            if (!m_bGroundPlaying) { forceStart = true; m_bGroundPlaying = true; }
            nextAnim = 7; nextLoop = false;
        }

        if (nextAnim < 0 && m_pAttack && *m_pAttack != ATTACK::NONE) {
            switch (*m_pAttack) {
            case ATTACK::ENTER:  nextAnim = 0;  nextLoop = false; forceStart = true; break;
            case ATTACK::IDLE:   nextAnim = 1;  nextLoop = true;  break;
            case ATTACK::FRONT:  nextAnim = 17; nextLoop = true;  break;
            case ATTACK::BACK:   nextAnim = 16; nextLoop = true;  break;
            case ATTACK::LEFT:   nextAnim = 18; nextLoop = true;  break;
            case ATTACK::RIGHT:  nextAnim = 19; nextLoop = true;  break;
            case ATTACK::THROW:  nextAnim = 20; nextLoop = false; forceStart = true; break;
            default: break;
            }
        }

        if (nextAnim < 0 && m_pMoving) {
            switch (*m_pMoving) {
            case MOVING::RUN:  nextAnim = 15; nextLoop = true; break;
            case MOVING::JOG:  nextAnim = 11; nextLoop = true; break;
            default:           nextAnim = 10; nextLoop = true; break; // IDLE
            }
        }
    }

    if (nextAnim < 0) { nextAnim = (m_iCurAnim >= 0) ? m_iCurAnim : 10; nextLoop = true; }

    auto isLoco = [](int a) { return (a == 10 || a == 11 || a == 15); };
    if (isLoco(nextAnim)) {
        if (m_LastLocoAnim < 0) {
            m_LastLocoAnim = nextAnim;
            m_LocoHold = 0.f;
        }
        else if (nextAnim != m_LastLocoAnim) {
            if (m_LocoHold < LOCO_MIN_HOLD)
                nextAnim = m_LastLocoAnim;
            else {
                m_LastLocoAnim = nextAnim;
                m_LocoHold = 0.f;
            }
        }
        else {
            m_LocoHold += fTimeDelta;
        }
    }
    else {
        m_LastLocoAnim = -1;
        m_LocoHold = 0.f;
    }

    SetClipSmart(nextAnim, nextLoop, 0.25f, forceStart);

    Update_CombinedMatrix();

    m_pColliderCom->Update(XMLoadFloat4x4(&m_CombinedWorldMatrix));
}

void CBody_Bridge::Late_Update(_float fTimeDelta)
{
    //if (true == m_pGameInstance->isIn_Frustum_WorldSpace(m_pTransformCom->Get_State(STATE::POSITION), 25.f))
    //{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;

    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::SHADOW, this)))
        return;
    //}


#ifdef _DEBUG
    m_pGameInstance->Add_DebugComponent(m_pColliderCom);
#endif
}

HRESULT CBody_Bridge::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint iNumMeshes = m_pModelCom->Get_NumMeshes();
    for (size_t i = 0; i < iNumMeshes; ++i)
    {
        if (FAILED(m_pModelCom->Bind_Materials_Bin(
            m_pShaderCom, "g_DiffuseTexture", i, ENUM_CLASS(TextureType::DIFFUSE), 0)))
        {
            OutputDebugStringA("Hero_body_머티리얼 바인딩 실패!\n");
            return E_FAIL;
        }

        if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
            return E_FAIL;

        m_pShaderCom->Begin(1);
        m_pModelCom->Render(i);
    }

    return S_OK;
}

HRESULT CBody_Bridge::Render_Shadow()
{
    if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_CombinedWorldMatrix)))
        return E_FAIL;

    /* 그림자를 표현하고하는 특수한 광원을 정의하고 그 광원이 바라본 장면응로서 플레이어를 그려준다. */
    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_ShadowLight_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_ShadowLight_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;

    _uint           iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (size_t i = 0; i < iNumMeshes; i++)
    {
        if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
            return E_FAIL;

        m_pShaderCom->Begin(2);

        m_pModelCom->Render(i);
    }


    return S_OK;
}

void CBody_Bridge::SetClipSmart(int animIndex, bool loop, _float blendDur, bool forceRestart)
{

    if (animIndex != m_iCurAnim) {
        m_pModelCom->Set_Animation(animIndex, loop, blendDur, true);
        m_iCurAnim = animIndex; m_bCurLoop = loop; m_fCurBlend = blendDur;
        return;
    }


    if (forceRestart) {
        m_pModelCom->Set_Animation(animIndex, loop, blendDur, true);
        m_bCurLoop = loop; m_fCurBlend = blendDur;
        return;
    }


    if (loop != m_bCurLoop || std::fabs(blendDur - m_fCurBlend) > 1e-3f) {
        m_pModelCom->Set_Animation(animIndex, loop, blendDur, false);
        m_bCurLoop = loop; m_fCurBlend = blendDur;
        return;
    }
}

HRESULT CBody_Bridge::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::BRIDGE), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::BRIDGE), TEXT("Prototype_Component_Model_Hero"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;

    CBounding_Sphere::BOUNDING_SPHERE_DESC  SphereDesc{};
    SphereDesc.fRadius = 0.7f;
    SphereDesc.vCenter = _float3(0.f, SphereDesc.fRadius, 0.f);

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &SphereDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CBody_Bridge::Bind_ShaderResources()
{
    if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_CombinedWorldMatrix)))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;

    return S_OK;
}

_bool CBody_Bridge::Collision_ToMushroom()
{
    /*CContainerObject* pPlayer = dynamic_cast<CContainerObject*>(m_pGameInstance->Find_GameObject(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Layer_Player")));
    if (nullptr == pPlayer)
        return false;

    CCollider* pTargetCollider = static_cast<CCollider*>(pPlayer->Get_Component(TEXT("Layer_Mushroom"), TEXT("Com_Collider")));
    if (nullptr == pTargetCollider)
        return false;

    return m_pColliderCom->Intersect(pTargetCollider);*/
    return S_OK;
}


CBody_Bridge* CBody_Bridge::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBody_Bridge* pInstance = new CBody_Bridge(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype())) {
        MSG_BOX(TEXT("Failed to Created : CBody_Bridge"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CBody_Bridge::Clone(void* pArg)
{
    CBody_Bridge* pInstance = new CBody_Bridge(*this);
    if (FAILED(pInstance->Initialize(pArg))) {
        MSG_BOX(TEXT("Failed to Clone : CBody_Bridge"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CBody_Bridge::Free()
{
    __super::Free();
    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
