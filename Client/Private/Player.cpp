#include "Player.h"
#include "GameInstance.h"
#include "PlayerManager.h"
#include "Body_Player.h"
#include "Weapon.h"
#include "Player_Speare.h"
#include "Object_Pool_Manager.h"
#include "Mushroom.h"
#include "Weapon_Skeleton_Arrow.h"
USING(Client)

CPlayer::CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CContainerObject{ pDevice, pContext } {
}

CPlayer::CPlayer(const CPlayer& Prototype)
    : CContainerObject{ Prototype } {
}

HRESULT CPlayer::Initialize_Prototype() { return S_OK; }

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

    // PlayerManager 등록 및 활성화
    CPlayerManager::GetInstance()->Register(ENUM_CLASS(LEVEL::BOSS), 2, this, -1.f);
    CPlayerManager::GetInstance()->SetActive(ENUM_CLASS(LEVEL::BOSS), 2);

    return S_OK;
}

void CPlayer::Priority_Update(_float fTimeDelta)
{
    __super::Priority_Update(fTimeDelta);
}

void CPlayer::Update(_float fTimeDelta)
{
    // ---- 입력 ----
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

    // ---- 공격/이동 상태 갱신(기존 로직) ----
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
                m_pTransformCom->Go_Straight(fTimeDelta * kDashImpulseMul);
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

    // ---- 이동 수행(이동 전 위치 백업) ----
    _vector prevPos = m_pTransformCom->Get_State(Engine::STATE::POSITION);

    float mul = 1.f;
    if (aimingModeNow)                mul = (RPress || LPress) ? kAimRunMul : kAimWalkMul;
    else if (m_eMoving == MOVING::RUN) mul = kRunMul;

    if (aimingModeNow) {
        if (w) m_pTransformCom->Go_Straight(fTimeDelta * mul);
        if (s) m_pTransformCom->Go_Backward(fTimeDelta * mul);
        if (a) m_pTransformCom->Go_Left(fTimeDelta * mul);
        if (d) m_pTransformCom->Go_Right(fTimeDelta * mul);
    }
    else {
        if (w) m_pTransformCom->Go_Straight(fTimeDelta * mul);
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
        if (m_iDashFlagFrames > 0) --m_iDashFlagFrames;
        else m_eMoving = anyMove ? MOVING::JOG : MOVING::IDLE;
    }

    // ---- 콜라이더 업데이트 & 충돌 해결 ----
    m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());

    if (ResolveBlockingCollisions()) {
        m_pTransformCom->Set_State(Engine::STATE::POSITION, prevPos);
        m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
    }

    // ---- 트리거 데미지(죽은 버섯의 트리거 등) ----
    TickDamageTriggers(fTimeDelta);
    TickHostileHits(fTimeDelta);
    // ---- 투척 발사 ----
    if (m_eAttack == ATTACK::THROW) {
        Throw_Spear();
        m_eAttack = ATTACK::NONE;
    }

    __super::Update(fTimeDelta);
}

void CPlayer::Late_Update(_float fTimeDelta)
{
    // 네비 위 보정
    m_pTransformCom->Set_State(
        Engine::STATE::POSITION,
        m_pNavigationCom->Compute_OnCell(m_pTransformCom->Get_State(Engine::STATE::POSITION)));

    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
        return;

#ifdef _DEBUG
    m_pGameInstance->Add_DebugComponent(m_pColliderCom);
    m_pGameInstance->Add_DebugComponent(m_pNavigationCom);
#endif
    __super::Late_Update(fTimeDelta);
}

HRESULT CPlayer::Render()
{
    return S_OK;
}

// --- Helpers ---
_vector CPlayer::Get_TransformState(Engine::STATE s) const
{
    return m_pTransformCom ? m_pTransformCom->Get_State(s) : XMVectorZero();
}

_vector CPlayer::GetPos()    const { return Get_TransformState(Engine::STATE::POSITION); }
_vector CPlayer::GetRight()  const { return XMVector3Normalize(Get_TransformState(Engine::STATE::RIGHT)); }
_vector CPlayer::GetUp()     const { return XMVector3Normalize(Get_TransformState(Engine::STATE::UP)); }
_vector CPlayer::GetForward(bool flattenY) const
{
    _vector f = Get_TransformState(Engine::STATE::LOOK);
    return flattenY ? XMVector3Normalize(XMVectorSetY(f, 0.f)) : XMVector3Normalize(f);
}

void CPlayer::Throw_Spear()
{
    using namespace DirectX;

    XMVECTOR pos = m_pTransformCom->Get_State(Engine::STATE::POSITION);
    XMVECTOR up = GetUp();                  // 단위
    XMVECTOR fwd = GetForward(false);       // 단위

    // 살짝 위/앞
    XMVECTOR spawn = pos + up * 1.5f + fwd * 0.6f;

    CPlayer_Speare::DESC desc{};
    XMStoreFloat3(&desc.pos, spawn);
    XMStoreFloat3(&desc.dir, fwd);
    desc.speed = 35.f;
    desc.gravity = -9.8f;
    desc.maxLife = 4.f;
    desc.owner = this;

    CObject_Pool_Manager::GetInstance()
        ->Acquire(LEVEL::BOSS, L"Layer_Spear", &desc);
}

HRESULT CPlayer::Ready_Components()
{
    CNavigation::NAVIGATION_DESC NaviDesc{};
    NaviDesc.iCurrentCellIndex = 0;

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_Component_Navigation_Boss"),
        TEXT("Com_Navigation"), reinterpret_cast<CComponent**>(&m_pNavigationCom), &NaviDesc)))
        return E_FAIL;

    // 플레이어 본체 충돌(막힘/피격 판단에 사용)
    CBounding_Sphere::BOUNDING_SPHERE_DESC S{};
    S.fRadius = 0.7f;
    S.vCenter = _float3(0.f, S.fRadius, 0.f);

    if (FAILED(CGameObject::Add_Component(
        ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Collider_Sphere"),
        TEXT("Com_Collider"), reinterpret_cast<CComponent**>(&m_pColliderCom), &S)))
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
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_Body_Player"), &BodyDesc)))
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
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_Weapon"), &WDesc)))
        return E_FAIL;

    return S_OK;
}

// ====== 충돌/트리거 유틸 구현 ======
bool CPlayer::CheckBlockingWithLayer(const _wstring& layerName) const
{
    const _uint level = ENUM_CLASS(LEVEL::BOSS);

    for (_uint i = 0;; ++i)
    {
        CGameObject* obj = m_pGameInstance->Find_GameObject(level, layerName, i);
        if (!obj) break;

        CCollider* col = static_cast<CCollider*>(obj->Get_Component(L"Com_Collider"));
        if (!col) continue;

        if (m_pColliderCom->Intersect(col))
            return true;
    }
    return false;
}

bool CPlayer::CheckTriggerWithLayer(const _wstring& layerName) const
{
    const _uint level = ENUM_CLASS(LEVEL::BOSS);

    for (_uint i = 0;; ++i)
    {
        CGameObject* obj = m_pGameInstance->Find_GameObject(level, layerName, i);
        if (!obj) break;

        CCollider* trig = static_cast<CCollider*>(obj->Get_Component(L"Com_Trigger"));
        if (!trig) continue;

        if (m_pColliderCom->Intersect(trig))
            return true;
    }
    return false;
}

bool CPlayer::ResolveBlockingCollisions()
{
    if (!m_pTransformCom || !m_pColliderCom) return false;

    const _uint    level = ENUM_CLASS(LEVEL::BOSS);
    const _wstring layer = L"Layer_Mushroom";

    DirectX::XMVECTOR prevR = m_pTransformCom->Get_State(Engine::STATE::RIGHT);
    DirectX::XMVECTOR prevU = m_pTransformCom->Get_State(Engine::STATE::UP);
    DirectX::XMVECTOR prevL = m_pTransformCom->Get_State(Engine::STATE::LOOK);
    DirectX::XMVECTOR prevP = m_pTransformCom->Get_State(Engine::STATE::POSITION);

    m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());

    bool blocked = false;

    for (_uint i = 0;; ++i)
    {
        CGameObject* obj = m_pGameInstance->Find_GameObject(level, layer, i);
        if (!obj) break;

        auto* mush = dynamic_cast<CMushroom*>(obj);
        if (!mush || !mush->IsAlive()) continue;

        Engine::CCollider* cBlock = mush->GetCollider_Block();
        if (!cBlock) continue;

        if (m_pColliderCom->Intersect(cBlock)) { blocked = true; break; }
    }

    if (blocked)
    {
        m_pTransformCom->Set_State(Engine::STATE::RIGHT, prevR);
        m_pTransformCom->Set_State(Engine::STATE::UP, prevU);
        m_pTransformCom->Set_State(Engine::STATE::LOOK, prevL);
        m_pTransformCom->Set_State(Engine::STATE::POSITION, prevP);

        m_pColliderCom->Update(m_pTransformCom->Get_WorldMatrix());
    }

    return blocked;
}

void CPlayer::TickDamageTriggers(float dt)
{
    m_damageTickAcc += dt;
    if (m_damageTickAcc < m_damageTickGap) return;
    m_damageTickAcc = 0.f;

    constexpr float kDeathTriggerDamage = 5.f;

    auto* pm = CPlayerManager::GetInstance();
    if (!pm) return;

    const float hpBefore = pm->GetActiveHP();

    const _uint level = ENUM_CLASS(LEVEL::BOSS);
    const _wstring layer = L"Layer_Mushroom";

    int hitCount = 0;

    for (_uint i = 0;; ++i)
    {
        CGameObject* obj = m_pGameInstance->Find_GameObject(level, layer, i);
        if (!obj) break;

        auto* mush = dynamic_cast<CMushroom*>(obj);
        if (!mush || !mush->IsDead()) continue;

        Engine::CCollider* trig = mush->GetCollider_TriggerIfActive();
        if (!trig) continue;

        if (m_pColliderCom->Intersect(trig)) {
            ++hitCount;
            pm->ApplyDamageActive(kDeathTriggerDamage);
        }
    }

    const float hpAfter = pm->GetActiveHP();

    if (hitCount > 0) {
        wchar_t wbuf[128];
        swprintf(wbuf, 128, L"[PLAYER] hited HP: %.1f -> %.1f\n",
            hpBefore, hpAfter);
        OutputDebugStringW(wbuf);
    }
}

void CPlayer::ApplyDamage(int amount)
{
    // 필요 시 다른 소스(예: 근접 피격 등)에서 호출 가능
    auto* pm = CPlayerManager::GetInstance();
    if (!pm) return;

    // 적용 전/후 HP 로깅
    const float hpBefore = pm->GetActiveHP();

    pm->ApplyDamageActive(static_cast<float>(amount));

    const float hpAfter = pm->GetActiveHP();

    wchar_t wbuf[128];
    swprintf(wbuf, 128, L"[PLAYER] ApplyDamage(%d)  HP: %.1f -> %.1f\n",
        amount, hpBefore, hpAfter);
    OutputDebugStringW(wbuf);
}

void CPlayer::TickHostileHits(float dt)
{
    auto* pm = CPlayerManager::GetInstance();
    if (!pm || !m_pColliderCom) return;

    const _uint level = ENUM_CLASS(LEVEL::BOSS);

    // ---------- 1) 적 화살 ----------
    {
        const std::wstring layer = L"Layer_Arrow";
        for (_uint i = 0;; ++i)
        {
            CGameObject* obj = m_pGameInstance->Find_GameObject(level, layer, i);
            if (!obj) break;

            auto* col = static_cast<Engine::CCollider*>(obj->Get_Component(L"Com_Collider"));
            if (!col) continue;

            if (m_pColliderCom->Intersect(col))
            {
                constexpr float kArrowDamage = 12.f;

                const float hpBefore = pm->GetActiveHP();
                pm->ApplyDamageActive(kArrowDamage);
                const float hpAfter = pm->GetActiveHP();

                // HP 로그
                wchar_t wbuf[128];
                swprintf(wbuf, 128, L"HITED Arrow =%.1f  HP: %.1f -> %.1f\n",
                    kArrowDamage, hpBefore, hpAfter);
                OutputDebugStringW(wbuf);

                // 화살은 한 번 맞으면 풀로 복귀(중복 히트 방지)
                if (auto* arrow = dynamic_cast<CWeapon_Skeleton_Arrow*>(obj))
                    int a = 0;// arrow->ReturnToPool(); // public이어야 함
                else
                    obj->Set_Active(false); // 최소한 비활성화
            }
        }
    }

    // ---------- 2) 적 근접 무기(스켈레톤 창 등) ----------
    {
        const std::wstring layer = L"Layer_MonsterHit";
        for (_uint i = 0;; ++i)
        {
            CGameObject* obj = m_pGameInstance->Find_GameObject(level, layer, i);
            if (!obj) break;

            auto* hitCol = static_cast<Engine::CCollider*>(obj->Get_Component(L"Com_Collider"));
            if (!hitCol) continue;

            if (m_pColliderCom->Intersect(hitCol))
            {
                constexpr float kSpearDamage = 15.f;

                const float hpBefore = pm->GetActiveHP();
                pm->ApplyDamageActive(kSpearDamage);
                const float hpAfter = pm->GetActiveHP();

                wchar_t wbuf[128];
                swprintf(wbuf, 128, L"HITED MonsterSpear =%.1f  HP: %.1f -> %.1f\n",
                    kSpearDamage, hpBefore, hpAfter);
                OutputDebugStringW(wbuf);

            }
        }
    }
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
