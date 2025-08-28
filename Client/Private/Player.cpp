#include "Player.h"
#include "GameInstance.h"
#include "PlayerManager.h"
#include "Body_Player.h"
#include "Weapon.h"

USING(Client)

CPlayer::CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CContainerObject{ pDevice, pContext } {
}

CPlayer::CPlayer(const CPlayer& Prototype)
    : CContainerObject{ Prototype } {
}

HRESULT CPlayer::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CPlayer::Initialize(void* pArg)
{
    GAMEOBJECT_DESC Desc{};
    Desc.fSpeedPerSec = 10.f;
    Desc.fRotationPerSec = XMConvertToRadians(180.f);

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (pArg)
    {
        HERO_DESC* pDesc = static_cast<HERO_DESC*>(pArg);
        m_eType = pDesc->type;

        XMMATRIX S = XMMatrixScaling(pDesc->vScale.x, pDesc->vScale.y, pDesc->vScale.z);
        XMMATRIX R = XMMatrixRotationRollPitchYaw(
            XMConvertToRadians(pDesc->vRot.x),
            XMConvertToRadians(pDesc->vRot.y),
            XMConvertToRadians(pDesc->vRot.z));
        XMMATRIX T = XMMatrixTranslation(pDesc->vPos.x, pDesc->vPos.y, pDesc->vPos.z);

        XMFLOAT4X4 W; XMStoreFloat4x4(&W, S * R * T);

        m_pTransformCom->Set_State(Engine::STATE::RIGHT, XMLoadFloat4((XMFLOAT4*)&W.m[0]));
        m_pTransformCom->Set_State(Engine::STATE::UP, XMLoadFloat4((XMFLOAT4*)&W.m[1]));
        m_pTransformCom->Set_State(Engine::STATE::LOOK, XMLoadFloat4((XMFLOAT4*)&W.m[2]));
        m_pTransformCom->Set_State(Engine::STATE::POSITION, XMLoadFloat4((XMFLOAT4*)&W.m[3]));
    }

    if (FAILED(Ready_Components()))   return E_FAIL;
    if (FAILED(Ready_PartObjects()))  return E_FAIL;

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
    const bool w = m_pGameInstance->KeyPressing(DIK_W);
    const bool a = m_pGameInstance->KeyPressing(DIK_A);
    const bool s = m_pGameInstance->KeyPressing(DIK_S);
    const bool d = m_pGameInstance->KeyPressing(DIK_D);
    const bool anyMove = (w || a || s || d);

    const bool shiftDown = m_pGameInstance->KeyDown(DIK_LSHIFT);
    const bool shiftPress = m_pGameInstance->KeyPressing(DIK_LSHIFT);
    const bool shiftUp = m_pGameInstance->KeyUp(DIK_LSHIFT);

    const bool LDown = m_pGameInstance->MouseDown(MOUSEKEYSTATE::LB);
    const bool LPress = m_pGameInstance->MousePressing(MOUSEKEYSTATE::LB);
    const bool LUp = m_pGameInstance->MouseUp(MOUSEKEYSTATE::LB);

    const bool RDown = m_pGameInstance->MouseDown(MOUSEKEYSTATE::RB);
    const bool RPress = m_pGameInstance->MousePressing(MOUSEKEYSTATE::RB);
    const bool RUp = m_pGameInstance->MouseUp(MOUSEKEYSTATE::RB);

    const bool SpaceDown = m_pGameInstance->KeyDown(DIK_SPACE);

    const bool anyMouse = (LDown || LPress || LUp || RDown || RPress || RUp);

    if (anyMouse)
    {
        if (LUp || (RPress && LDown))
            m_eAttack = ATTACK::THROW;
        else if (RUp)
            m_eAttack = ATTACK::NONE;
        else if (LDown && m_eAttack == ATTACK::NONE)
            m_eAttack = ATTACK::ENTER;
        else if (RDown && m_eAttack == ATTACK::NONE)
            m_eAttack = ATTACK::IDLE;
        else if (LPress || RPress)
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
        if (m_eAttack != ATTACK::THROW &&
            m_eAttack != ATTACK::GROUND &&
            m_eAttack != ATTACK::ENTER)
            m_eAttack = ATTACK::NONE;

        if (shiftDown) { m_bShiftPressed = true; m_fShiftHeldSec = 0.f; }
        if (m_bShiftPressed && shiftPress) {
            m_fShiftHeldSec += fTimeDelta;
            if (anyMove && m_fShiftHeldSec >= kRunHoldThreshold)
                m_eMoving = MOVING::RUN;
        }
        if (shiftUp) {
            if (m_fShiftHeldSec < kRunHoldThreshold) {
                m_eMoving = MOVING::DASH;
                m_pTransformCom->Go_Straight(fTimeDelta * kDashImpulseMul, m_pNavigationCom);
                m_iDashFlagFrames = 1;
            }
            m_bShiftPressed = false;
        }

        if (!shiftPress && m_eMoving != MOVING::DASH)
            m_eMoving = anyMove ? MOVING::JOG : MOVING::IDLE;
    }

    if (SpaceDown)
        m_eAttack = ATTACK::GROUND;

    const bool aimingHeldNow = (LPress || RPress);
    const bool aimingModeNow =
        aimingHeldNow ||
        m_eAttack == ATTACK::ENTER ||
        m_eAttack == ATTACK::IDLE ||
        m_eAttack == ATTACK::FRONT ||
        m_eAttack == ATTACK::BACK ||
        m_eAttack == ATTACK::LEFT ||
        m_eAttack == ATTACK::RIGHT ||
        m_eAttack == ATTACK::THROW;

    float mul = 1.f;
    if (aimingModeNow)               mul = (RPress || LPress) ? kAimRunMul : kAimWalkMul;
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
        else if (a) {
            m_pTransformCom->Go_Straight(fTimeDelta * mul);
            m_pTransformCom->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), -fTimeDelta);
        }
        else if (d) {
            m_pTransformCom->Go_Straight(fTimeDelta * mul);
            m_pTransformCom->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), fTimeDelta);
        }
    }

    if (m_eMoving == MOVING::DASH) {
        if (m_iDashFlagFrames > 0) {
            --m_iDashFlagFrames;
        }
        else {
            m_eMoving = anyMove ? MOVING::JOG : MOVING::IDLE;
        }
    }

    m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
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
    m_pColliderCom->Render();
    if (m_pNavigationCom) m_pNavigationCom->Render();
#endif
    return S_OK;
}

// --- Helpers ---
_vector CPlayer::Get_TransformState(Engine::STATE s) const
{
    return m_pTransformCom ? m_pTransformCom->Get_State(s) : XMVectorZero();
}

_vector CPlayer::GetPos() const { return Get_TransformState(Engine::STATE::POSITION); }
_vector CPlayer::GetRight() const { return XMVector3Normalize(Get_TransformState(Engine::STATE::RIGHT)); }
_vector CPlayer::GetUp() const { return XMVector3Normalize(Get_TransformState(Engine::STATE::UP)); }
_vector CPlayer::GetForward(bool flattenY) const
{
    _vector f = Get_TransformState(Engine::STATE::LOOK);
    return flattenY ? XMVector3Normalize(XMVectorSetY(f, 0.f)) : XMVector3Normalize(f);
}

HRESULT CPlayer::Ready_Components()
{
    CNavigation::NAVIGATION_DESC NaviDesc{};
    NaviDesc.iCurrentCellIndex = 0;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Navigation"),
        TEXT("Com_Navigation"), reinterpret_cast<CComponent**>(&m_pNavigationCom), &NaviDesc)))
        return E_FAIL;

    CBounding_AABB::BOUNDING_AABB_DESC  AABBDesc{};
    AABBDesc.vExtents = _float3(0.4f, 0.7f, 0.4f);
    AABBDesc.vCenter = _float3(0.f, AABBDesc.vExtents.y, 0.f);


    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_AABB"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &AABBDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CPlayer::Ready_PartObjects()
{
    CBody_Player::BODY_DESC BodyDesc{};
    BodyDesc.pState = &m_iState;
    BodyDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
    BodyDesc.pMoving = &m_eMoving;
    BodyDesc.pAttack = &m_eAttack;

    if (FAILED(__super::Add_PartObject(TEXT("Part_Body"),
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Body_Player"), &BodyDesc)))
        return E_FAIL;

    CPartObject* pBody = Find_PartObject(TEXT("Part_Body"));
    if (!pBody) return E_FAIL;

    CWeapon::WEAPON_DESC WDesc{};
    WDesc.pState = &m_iState;
    WDesc.pSocketMatrix = dynamic_cast<CBody_Player*>(pBody)->Get_BoneMatrix("jnt_Spine_01_SKN");
    WDesc.pSocketMatrix_Hand = dynamic_cast<CBody_Player*>(pBody)->Get_BoneMatrix("jnt_weapon");
    WDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
    WDesc.pMoving = &m_eMoving;
    WDesc.pAttack = &m_eAttack;

    if (FAILED(__super::Add_PartObject(TEXT("Part_Weapon"),
        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon"), &WDesc)))
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
    Safe_Release(m_pColliderCom);
    Safe_Release(m_pNavigationCom);
}
