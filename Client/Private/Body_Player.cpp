#include "Body_Player.h"
#include "GameInstance.h"
#include <cmath>

CBody_Player::CBody_Player(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CPartObject{ pDevice, pContext } {
}

CBody_Player::CBody_Player(const CBody_Player& Prototype)
    : CPartObject{ Prototype } {
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
    if (!pArg) return E_FAIL;

    BODY_DESC* pDesc = static_cast<BODY_DESC*>(pArg);
    m_pParentState = pDesc->pState;
    m_pMoving = pDesc->pMoving;
    m_pAttack = pDesc->pAttack;

    if (FAILED(__super::Initialize(pArg))) return E_FAIL;
    if (FAILED(Ready_Components()))       return E_FAIL;

    // 초기 상태
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

void CBody_Player::Priority_Update(_float /*fTimeDelta*/)
{
    // 필요 시 선행 처리
}

void CBody_Player::Update(_float fTimeDelta)
{
    // --- 대쉬 재트리거 쿨다운 감소 ---
    if (m_fDashFinishBlock > 0.f)
        m_fDashFinishBlock -= fTimeDelta;

    float playDt = fTimeDelta;
    if (m_pAttack && *m_pAttack == ATTACK::THROW && m_iCurAnim == 20)
        playDt = fTimeDelta * 2.f; // 투척만 가속

    const bool finished = m_pModelCom->Play_Animation(playDt);

    // one-shot 종료 후 정리(현재 재생중 클립 기준)
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

    // -------------------------------------------------
    // 1) 다음 클립 선택
    // -------------------------------------------------
    int   nextAnim = -1;
    bool  nextLoop = true;
    bool  forceStart = false;

    const bool dashRequested = (m_pMoving && *m_pMoving == MOVING::DASH);

    // 대쉬: 진행중이면 유지, 아니면 요청이 있고 쿨다운이 0일 때 1회 시작
    if (m_bDashPlaying) {
        nextAnim = 2; nextLoop = false;
    }
    else if (dashRequested && m_fDashFinishBlock <= 0.f) {
        forceStart = true;
        m_bDashPlaying = true;
        nextAnim = 2; nextLoop = false;
    }
    else {
        // 지면찍기(one-shot)
        if (m_pAttack && *m_pAttack == ATTACK::GROUND) {
            if (!m_bGroundPlaying) { forceStart = true; m_bGroundPlaying = true; }
            nextAnim = 7; nextLoop = false;
        }

        // 공격/조준
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

        // 로코모션
        if (nextAnim < 0 && m_pMoving) {
            switch (*m_pMoving) {
            case MOVING::RUN:  nextAnim = 15; nextLoop = true; break;
            case MOVING::JOG:  nextAnim = 11; nextLoop = true; break;
            default:           nextAnim = 10; nextLoop = true; break; // IDLE
            }
        }
    }

    // 안전가드
    if (nextAnim < 0) { nextAnim = (m_iCurAnim >= 0) ? m_iCurAnim : 10; nextLoop = true; }

    // 로코모션 히스테리시스(깜빡임 방지)
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

    // -------------------------------------------------
    // 2) 선택 결과 적용(이 프레임에는 추가 재생 X)
    // -------------------------------------------------
    SetClipSmart(nextAnim, nextLoop, 0.25f, forceStart);

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

        m_pShaderCom->Begin(0);
        m_pModelCom->Render(i);
    }
    return S_OK;
}

void CBody_Player::SetClipSmart(int animIndex, bool loop, _float blendDur, bool forceRestart)
{
    // 1) 다른 인덱스면 무조건 세팅(재시작)
    if (animIndex != m_iCurAnim) {
        m_pModelCom->Set_Animation(animIndex, loop, blendDur, true);
        m_iCurAnim = animIndex; m_bCurLoop = loop; m_fCurBlend = blendDur;
        return;
    }

    // 2) 같은 인덱스지만 강제 재시작이면 리스타트
    if (forceRestart) {
        m_pModelCom->Set_Animation(animIndex, loop, blendDur, true);
        m_bCurLoop = loop; m_fCurBlend = blendDur;
        return;
    }

    // 3) 같은 인덱스 + loop/fade 변경만 → 리스타트 없이 반영
    if (loop != m_bCurLoop || std::fabs(blendDur - m_fCurBlend) > 1e-3f) {
        m_pModelCom->Set_Animation(animIndex, loop, blendDur, false);
        m_bCurLoop = loop; m_fCurBlend = blendDur;
        return;
    }
}

HRESULT CBody_Player::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
        TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
        return E_FAIL;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Model_Hero"),
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

    const LIGHT_DESC* pLightDesc = m_pGameInstance->Get_LightDesc(0);
    if (!pLightDesc) return E_FAIL;

    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDir", &pLightDesc->vDirection, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &pLightDesc->vDiffuse, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightAmbient", &pLightDesc->vAmbient, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightSpecular", &pLightDesc->vSpecular, sizeof(_float4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4)))) return E_FAIL;

    return S_OK;
}

CBody_Player* CBody_Player::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBody_Player* pInstance = new CBody_Player(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype())) {
        MSG_BOX(TEXT("Failed to Created : CBody_Player"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CBody_Player::Clone(void* pArg)
{
    CBody_Player* pInstance = new CBody_Player(*this);
    if (FAILED(pInstance->Initialize(pArg))) {
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
