#include "Player.h"
#include "GameInstance.h"
#include "PlayerManager.h"
#include "Body_Player.h"
#include "Weapon.h"

CPlayer::CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CContainerObject { pDevice, pContext }
{

}

CPlayer::CPlayer(const CPlayer& Prototype)
    : CContainerObject { Prototype }
{

}

HRESULT CPlayer::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CPlayer::Initialize(void* pArg)
{
    GAMEOBJECT_DESC         Desc{};
    Desc.fSpeedPerSec = 10.f;
    Desc.fRotationPerSec = XMConvertToRadians(180.0f);

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (pArg)
    {
        HERO_DESC* pDesc = static_cast<HERO_DESC*>(pArg);
        m_eType = pDesc->type;
        //크기
        XMMATRIX matScale = XMMatrixScaling(pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z);

        //회전
        XMMATRIX matRot = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(pDesc->vRot.x),
            XMConvertToRadians(pDesc->vRot.y),
            XMConvertToRadians(pDesc->vRot.z));

        //위치
        XMMATRIX matTrans = XMMatrixTranslation(pDesc->vPos.x, pDesc->vPos.y, pDesc->vPos.z);

        XMMATRIX matWorld = matScale * matRot * matTrans;

        XMFLOAT4X4 matWorld4x4;
        XMStoreFloat4x4(&matWorld4x4, matWorld);

        m_pTransformCom->Set_State(Engine::STATE::RIGHT, XMLoadFloat4((XMFLOAT4*)&matWorld4x4.m[0]));
        m_pTransformCom->Set_State(Engine::STATE::UP, XMLoadFloat4((XMFLOAT4*)&matWorld4x4.m[1]));
        m_pTransformCom->Set_State(Engine::STATE::LOOK, XMLoadFloat4((XMFLOAT4*)&matWorld4x4.m[2]));
        m_pTransformCom->Set_State(Engine::STATE::POSITION, XMLoadFloat4((XMFLOAT4*)&matWorld4x4.m[3]));

        char szDbg[256];
        sprintf_s(szDbg, sizeof(szDbg), "type=%d scale=(%.2f,%.2f,%.2f) rot=(%.2f,%.2f,%.2f) pos=(%.2f,%.2f,%.2f)\n",
            (int)m_eType,
            pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z,
            pDesc->vRot.x, pDesc->vRot.y, pDesc->vRot.z,
            pDesc->vPos.x, pDesc->vPos.y, pDesc->vPos.z);
        OutputDebugStringA(szDbg);
    }

    if (FAILED(Ready_Components()))
        return E_FAIL;  

    if (FAILED(Ready_PartObjects()))
        return E_FAIL;

    CPlayerManager::GetInstance()->Register(0, this, 100.f);
    CPlayerManager::GetInstance()->SetActive(0);

    return S_OK;
}

void CPlayer::Priority_Update(_float fTimeDelta)
{
    __super::Priority_Update(fTimeDelta);
}

void CPlayer::Update(_float fTimeDelta)
{
    // --- 키 입력 ---
    const bool w = m_pGameInstance->KeyPressing(DIK_W);
    const bool a = m_pGameInstance->KeyPressing(DIK_A);
    const bool s = m_pGameInstance->KeyPressing(DIK_S);
    const bool d = m_pGameInstance->KeyPressing(DIK_D);
    const bool anyMove = (w || a || s || d);

    const bool shiftDown = m_pGameInstance->KeyDown(DIK_LSHIFT);
    const bool shiftPress = m_pGameInstance->KeyPressing(DIK_LSHIFT);
    const bool shiftUp = m_pGameInstance->KeyUp(DIK_LSHIFT);

    const bool LMouseDown = m_pGameInstance->MouseDown(MOUSEKEYSTATE::LB);
    const bool LMousePress = m_pGameInstance->MousePressing(MOUSEKEYSTATE::LB);
    const bool LMouseUp = m_pGameInstance->MouseUp(MOUSEKEYSTATE::LB);

    const bool RMouseDown = m_pGameInstance->MouseDown(MOUSEKEYSTATE::RB);
    const bool RMousePress = m_pGameInstance->MousePressing(MOUSEKEYSTATE::RB);
    const bool RMouseUp = m_pGameInstance->MouseUp(MOUSEKEYSTATE::RB);

    const bool Space = m_pGameInstance->KeyDown(DIK_SPACE);

    const bool anyMouse = (LMouseDown || LMousePress || LMouseUp || RMouseDown || RMousePress || RMouseUp);


    if (anyMouse)
    {
        if (LMouseUp || (RMousePress && LMouseDown))
        {
            m_eAttack = ATTACK::THROW;
        }
        // 조준 종료
        else if (RMouseUp)
        {
            m_eAttack = ATTACK::NONE;
        }
        // 조준 진입
        else if ((LMouseDown) && m_eAttack == ATTACK::NONE)
        {
            m_eAttack = ATTACK::ENTER;
        }
        else if ((RMouseDown) && m_eAttack == ATTACK::NONE)
        {
            m_eAttack = ATTACK::IDLE;
        }

        //조준 유지(방향/정지). ENTER/GROUND 중에는 덮어쓰지 않음
        else if (LMousePress || RMousePress)
        {
            if (m_eAttack != ATTACK::ENTER &&
                m_eAttack != ATTACK::GROUND &&
                m_eAttack != ATTACK::THROW)
            {
                if (w) m_eAttack = ATTACK::FRONT;
                else if (s) m_eAttack = ATTACK::BACK;
                else if (a) m_eAttack = ATTACK::LEFT;
                else if (d) m_eAttack = ATTACK::RIGHT;
                else        m_eAttack = ATTACK::IDLE;
            }
        }
    }
    else
    {
        if (m_eAttack != ATTACK::THROW && m_eAttack != ATTACK::GROUND && m_eAttack != ATTACK::ENTER)
            m_eAttack = ATTACK::NONE;

        if (shiftDown) {
            m_bshiftPressed = true;
            m_fshiftHeldSec = 0.f;
        }
        if (m_bshiftPressed && shiftPress) {
            m_fshiftHeldSec += fTimeDelta;
            if (anyMove && m_fshiftHeldSec >= RUN_HOLD_THRESHOLD)
                m_eMoving = MOVING::RUN;
        }
        if (shiftUp) {
            if (m_fshiftHeldSec < RUN_HOLD_THRESHOLD) {
                // 수정해야함(네비깔면 터질수도)
                m_pTransformCom->Go_Straight(fTimeDelta * 20.0f);
                m_eMoving = MOVING::DASH;
            }
            m_bshiftPressed = false;
        }

        if (!shiftPress && m_eMoving != MOVING::DASH)
            m_eMoving = anyMove ? MOVING::JOG : MOVING::IDLE;
    }

    if (Space)
        m_eAttack = ATTACK::GROUND;

    const bool aimingHeldNow = (LMousePress || RMousePress);
    const bool aimingModeNow =
        aimingHeldNow ||
        m_eAttack == ATTACK::ENTER ||
        m_eAttack == ATTACK::IDLE ||
        m_eAttack == ATTACK::FRONT ||
        m_eAttack == ATTACK::BACK ||
        m_eAttack == ATTACK::LEFT ||
        m_eAttack == ATTACK::RIGHT ||
        m_eAttack == ATTACK::THROW;

    const float kRunMul = 1.3f; // 달리기
    const float kAimWalkMul = 0.3f; // 조준 느리게
    const float kAimRunMul = 0.6f; // 조준 빠르게(버튼 유지)

    float mul = 1.0f;
    if (aimingModeNow)               mul = (RMousePress || LMousePress) ? kAimRunMul : kAimWalkMul;
    else if (m_eMoving == MOVING::RUN) mul = kRunMul;

    if (aimingModeNow) {
        if (w) m_pTransformCom->Go_Straight(fTimeDelta * mul);
        if (s) m_pTransformCom->Go_Backward(fTimeDelta * mul);
        if (a) m_pTransformCom->Go_Left(fTimeDelta * mul);
        if (d) m_pTransformCom->Go_Right(fTimeDelta * mul);
    }
    else {
        if (w) m_pTransformCom->Go_Straight(fTimeDelta * mul, m_pNavigationCom);
        else if (s) m_pTransformCom->Go_Backward(fTimeDelta * mul);
        else if (a)
        {
            m_pTransformCom->Go_Straight(fTimeDelta * mul);
            m_pTransformCom->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), -fTimeDelta);
        }
        else if (d)
        {
            m_pTransformCom->Go_Straight(fTimeDelta * mul);
            m_pTransformCom->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), fTimeDelta);
        }
    }

    __super::Update(fTimeDelta);
}



void CPlayer::Late_Update(_float fTimeDelta)
{
    m_pTransformCom->Set_State(Engine::STATE::POSITION,
        m_pNavigationCom->Compute_OnCell(m_pTransformCom->Get_State(Engine::STATE::POSITION)));
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;

    __super::Late_Update(fTimeDelta);
}

HRESULT CPlayer::Render()
{
#ifdef _DEBUG
    m_pNavigationCom->Render();

#endif
    return S_OK;
}

_vector CPlayer::Get_TransformState(Engine::STATE s) const
{
    return m_pTransformCom ? m_pTransformCom->Get_State(s) : XMVectorZero();
}

_vector CPlayer::GetPos() const
{
    return Get_TransformState(Engine::STATE::POSITION);
}

_vector CPlayer::GetForward(bool flattenY) const
{
    _vector f = Get_TransformState(Engine::STATE::LOOK);
    return flattenY ? XMVector3Normalize(XMVectorSetY(f, 0.f))
        : XMVector3Normalize(f);
}

_vector CPlayer::GetRight() const
{
    return XMVector3Normalize(Get_TransformState(Engine::STATE::RIGHT));
}

_vector CPlayer::GetUp() const
{
    return XMVector3Normalize(Get_TransformState(Engine::STATE::UP));
}

HRESULT CPlayer::Ready_Components()
{
    CNavigation::NAVIGATION_DESC        NaviDesc{};
    NaviDesc.iCurrentCellIndex = 0;

    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Navigation"),
        TEXT("Com_Navigation"), reinterpret_cast<CComponent**>(&m_pNavigationCom), &NaviDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CPlayer::Ready_PartObjects()
{
    CBody_Player::BODY_DESC         BodyDesc{};
    BodyDesc.pState = &m_iState;
    BodyDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
    BodyDesc.pMoving = &m_eMoving;
    BodyDesc.pAttack = &m_eAttack;

    if (FAILED(__super::Add_PartObject(TEXT("Part_Body"), ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Body_Player"), &BodyDesc)))
        return E_FAIL;

    CPartObject*      pBody = Find_PartObject(TEXT("Part_Body"));
    if (nullptr == pBody)
        return E_FAIL;
    
    CWeapon::WEAPON_DESC                 WeaponDesc{};
    WeaponDesc.pState = &m_iState;
    WeaponDesc.pSocketMatrix = dynamic_cast<CBody_Player*>(pBody)->Get_BoneMatrix("jnt_Spine_01_SKN");
    WeaponDesc.pSocketMatrix_Hand = dynamic_cast<CBody_Player*>(pBody)->Get_BoneMatrix("jnt_weapon");
    WeaponDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
    WeaponDesc.pMoving = &m_eMoving;
    WeaponDesc.pAttack = &m_eAttack;

    if (FAILED(__super::Add_PartObject(TEXT("Part_Weapon"), ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon"), &WeaponDesc)))
        return E_FAIL;

    return S_OK;
}

CPlayer* CPlayer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CPlayer* pInstance = new CPlayer(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CPlayer"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CPlayer::Clone(void* pArg)
{
    CPlayer* pInstance = new CPlayer(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Clone : CPlayer"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CPlayer::Free()
{
    __super::Free();

    Safe_Release(m_pNavigationCom);
}
