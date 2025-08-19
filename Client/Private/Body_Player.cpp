#include "Body_Player.h"
#include "GameInstance.h"

CBody_Player::CBody_Player(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CPartObject{ pDevice, pContext }
{

}

CBody_Player::CBody_Player(const CBody_Player& Prototype)
    : CPartObject{ Prototype }
{

}

_float4x4* CBody_Player::Get_BoneMatrix(const _char* pBoneName)
{
    return m_pModelCom->Get_BoneMatrix(pBoneName);
}
    


HRESULT CBody_Player::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CBody_Player::Initialize(void* pArg)
{
    BODY_DESC* pDesc = static_cast<BODY_DESC*>(pArg);
    m_pParentState = pDesc->pState;
    m_pMoving = pDesc->pMoving;
    m_pAttack = pDesc->pAttack;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    //m_pModelCom->Set_Animation(0, true);    
    m_iCurAnim = -1;

    return S_OK;
}

void CBody_Player::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CBody_Player::Update(_float fTimeDelta)
{
    // --- 어떤 애니를 재생할지 결정 ---
    int   nextAnim = -1;
    bool  nextLoop = true;
    bool  forceStart = false;

    // 1) 대쉬(최우선, one-shot)
    if (*m_pMoving == MOVING::DASH) {
        if (!m_bDashPlaying) {            // 처음 진입 시 강제 리스타트
            forceStart = true;
            m_bDashPlaying = true;
        }
        nextAnim = 2;   // ANIM_HERO_dash_foward
        nextLoop = false;
    }
    else {
        m_bDashPlaying = false;

        if (m_pAttack && *m_pAttack == ATTACK::GROUND) {
            if (!m_bGroundPlaying) { forceStart = true; m_bGroundPlaying = true; }
            nextAnim = 7;                // ANIM_HERO_hit_ground_with_spear
            nextLoop = false;            // one-shot
        }

        // 2) 공격 레이어(조준/투척/스트레이프)
        if (m_pAttack && *m_pAttack != ATTACK::NONE) {
            switch (*m_pAttack) {
            case ATTACK::ENTER:     nextAnim = 0;  nextLoop = false; forceStart = true; break; // one-shot
            case ATTACK::IDLE:      nextAnim = 1;  nextLoop = true;  break;                    // loop
            case ATTACK::FRONT:     nextAnim = 17; nextLoop = true;  break;                    // loop
            case ATTACK::BACK:      nextAnim = 16; nextLoop = true;  break;                    // loop
            case ATTACK::LEFT:      nextAnim = 18; nextLoop = true;  break;                    // loop
            case ATTACK::RIGHT:     nextAnim = 19; nextLoop = true;  break;                    // loop
            case ATTACK::THROW:     nextAnim = 20; nextLoop = false; forceStart = true; break; // one-shot
            default: break;
            }
        }

        // 3) 공격 없음 → 로코모션
        if (nextAnim < 0) {
            switch (*m_pMoving) {
            case MOVING::RUN:  nextAnim = 15; nextLoop = true; break;
            case MOVING::JOG:  nextAnim = 11; nextLoop = true; break;
            default:           nextAnim = 10; nextLoop = true; break; // IDLE
            }
        }
    }

    // --- 실제 적용 & 재생 ---

    bool finished = {};
    SetClipOnce(nextAnim, nextLoop, forceStart);
    if(*m_pAttack == ATTACK::THROW)
        finished = m_pModelCom->Play_Animation(fTimeDelta * 2);
    else
    finished = m_pModelCom->Play_Animation(fTimeDelta);

    // --- 원샷 종료 후 상태 정리 ---
    // 대쉬 끝
    if (m_bDashPlaying && finished && m_iCurAnim == 2) {
        m_bDashPlaying = false;
        *m_pMoving = MOVING::IDLE;
    }

    // 조준 진입(AIM_ENTER) 끝 → 조준 유지(IDLE)로
    if (m_pAttack && *m_pAttack == ATTACK::ENTER && finished && m_iCurAnim == 0) {
        *m_pAttack = ATTACK::IDLE;
    }

    // 투척(THROW) 끝 → 공격 상태 해제
    if (m_pAttack && *m_pAttack == ATTACK::THROW && finished && m_iCurAnim == 20) {
        *m_pAttack = ATTACK::NONE;
    }
    if (m_bGroundPlaying && finished && m_iCurAnim == 7) {
        m_bGroundPlaying = false;
        if (m_pAttack) *m_pAttack = ATTACK::NONE;
    }

    Update_CombinedMatrix();
}



void CBody_Player::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;
}

HRESULT CBody_Player::Render()
{
    if (FAILED(Bind_ShaderResources()))
        return E_FAIL;

    _uint           iNumMeshes = m_pModelCom->Get_NumMeshes();

    for (size_t i = 0; i < iNumMeshes; i++)
    {
        if (FAILED(m_pModelCom->Bind_Materials_Bin(m_pShaderCom, "g_DiffuseTexture", i, ENUM_CLASS(TextureType::DIFFUSE), 0))) // 0: DIFFUSE
        {
            OutputDebugStringA("Hero_body_머티리얼 바인딩 실패!\n");
            return E_FAIL;
        }
        /*if (FAILED(m_pModelCom->Bind_Materials(m_pShaderCom, "g_NormalTexture", i, aiTextureType_DIFFUSE, 0)))
            return E_FAIL;        */

        if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
            return E_FAIL;

        m_pShaderCom->Begin(0);

        m_pModelCom->Render(i);
    }

    return S_OK;
}

void CBody_Player::SetClipOnce(int animIndex, bool loop, bool forceRestart)
{
    if (m_iCurAnim != animIndex) {
        m_pModelCom->Set_Animation(animIndex, loop, forceRestart);
        m_iCurAnim = animIndex;
    }
}

HRESULT CBody_Player::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Hero"),
        TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
        return E_FAIL;    

    return S_OK;
}

HRESULT CBody_Player::Bind_ShaderResources()
{
    if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_CombinedWorldMatrix)))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
        return E_FAIL;    

    const LIGHT_DESC*       pLightDesc = m_pGameInstance->Get_LightDesc(0);
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

CBody_Player* CBody_Player::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBody_Player* pInstance = new CBody_Player(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CBody_Player"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CBody_Player::Clone(void* pArg)
{
    CBody_Player* pInstance = new CBody_Player(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Clone : CBody_Player"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBody_Player::Free()
{
    __super::Free();

    Safe_Release(m_pModelCom);
    Safe_Release(m_pShaderCom);
}
