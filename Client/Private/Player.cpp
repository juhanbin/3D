#include "Player.h"
#include "GameInstance.h"

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

    return S_OK;
}

void CPlayer::Priority_Update(_float fTimeDelta)
{
    __super::Priority_Update(fTimeDelta);
}

void CPlayer::Update(_float fTimeDelta)
{   
    bool moving = false;

    if (GetKeyState(VK_DOWN) & 0x8000) { m_pTransformCom->Go_Backward(fTimeDelta); moving = true; }
    if (GetKeyState(VK_LEFT) & 0x8000)  m_pTransformCom->Turn(XMVectorSet(0, 1, 0, 0), -fTimeDelta);
    if (GetKeyState(VK_RIGHT) & 0x8000) m_pTransformCom->Turn(XMVectorSet(0, 1, 0, 0), fTimeDelta);
    if (GetKeyState(VK_UP) & 0x8000) { m_pTransformCom->Go_Straight(fTimeDelta); moving = true; }

    const bool attack = m_pGameInstance->MousePressing(MOUSEKEYSTATE::RB);

    m_iState &= ~(IDLE | RUN | ATTACK);
    if (attack)      m_iState |= ATTACK;
    else if (moving) m_iState |= RUN;
    else             m_iState |= IDLE;

    __super::Update(fTimeDelta);
}

void CPlayer::Late_Update(_float fTimeDelta)
{
    

    __super::Late_Update(fTimeDelta);
}

HRESULT CPlayer::Render()
{

    return S_OK;
}

HRESULT CPlayer::Ready_Components()
{
 

    return S_OK;
}

HRESULT CPlayer::Ready_PartObjects()
{
    CBody_Player::BODY_DESC         BodyDesc{};
    BodyDesc.pState = &m_iState;
    BodyDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();

    if (FAILED(__super::Add_PartObject(TEXT("Part_Body"), ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Body_Player"), &BodyDesc)))
        return E_FAIL;

    CPartObject*      pBody = Find_PartObject(TEXT("Part_Body"));
    if (nullptr == pBody)
        return E_FAIL;
    
    CWeapon::WEAPON_DESC                 WeaponDesc{};
    WeaponDesc.pState = &m_iState;
    WeaponDesc.pSocketMatrix = dynamic_cast<CBody_Player*>(pBody)->Get_BoneMatrix("jnt_weapon");
    WeaponDesc.pSocketMatrix_Hand = dynamic_cast<CBody_Player*>(pBody)->Get_BoneMatrix("jnt_Spine_01_SKN");
    WeaponDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();

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


}
