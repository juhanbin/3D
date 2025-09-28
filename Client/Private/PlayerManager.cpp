#include "PlayerManager.h"
#include "GameInstance.h"   // CurLevelId() 구현 위해 필요
#include <algorithm>

USING(Client)

IMPLEMENT_SINGLETON(CPlayerManager)

// ===== 내부 유틸 =====
inline _uint CPlayerManager::CurLevelId() {
    return CGameInstance::GetInstance()->Get_CurrentLevelID();
}

const CPlayerManager::LevelBucket* CPlayerManager::FindBucket(_uint levelId) const {
    auto it = m_levels.find(levelId);
    return (it == m_levels.end()) ? nullptr : &it->second;
}
CPlayerManager::LevelBucket* CPlayerManager::FindBucket(_uint levelId) {
    auto it = m_levels.find(levelId);
    return (it == m_levels.end()) ? nullptr : &it->second;
}

const CPlayerManager::PlayerData* CPlayerManager::FindPlayer(_uint levelId, _uint playerId) const {
    auto b = FindBucket(levelId);
    if (!b) return nullptr;
    auto it = b->players.find(playerId);
    return (it == b->players.end()) ? nullptr : &it->second;
}
CPlayerManager::PlayerData* CPlayerManager::FindPlayer(_uint levelId, _uint playerId) {
    auto b = FindBucket(levelId);
    if (!b) return nullptr;
    auto it = b->players.find(playerId);
    return (it == b->players.end()) ? nullptr : &it->second;
}

// ===== 등록/해제 =====
void CPlayerManager::Unregister(_uint levelId, _uint playerId)
{
    LevelBucket* b = FindBucket(levelId);
    if (!b) return;

    const bool wasActive = (b->hasActive && b->activeId == playerId);
    b->players.erase(playerId);

    if (b->players.empty()) {
        // 레벨 버킷 자체 제거
        m_levels.erase(levelId);
        return;
    }

    if (wasActive) {
        // 남아있는 첫 플레이어를 Active로 지정
        auto it = b->players.begin();
        b->activeId = it->first;
        b->hasActive = true;
    }
}

// ===== Active 선택 =====
void CPlayerManager::SetActive(_uint levelId, _uint playerId)
{
    LevelBucket* b = FindBucket(levelId);
    if (!b) return;

    if (b->players.find(playerId) == b->players.end()) return;

    b->activeId = playerId;
    b->hasActive = true;
}
void CPlayerManager::SetActiveByCurrentLevel(_uint playerId)
{
    SetActive(CurLevelId(), playerId);
}

void* CPlayerManager::GetActiveRaw() const
{
    const _uint lev = CurLevelId();
    const LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return nullptr;

    auto it = b->players.find(b->activeId);
    if (it == b->players.end()) return nullptr;
    return it->second.pObject;
}

_uint CPlayerManager::GetActiveId() const
{
    const _uint lev = CurLevelId();
    const LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return 0;
    return b->activeId;
}

void* CPlayerManager::GetRaw(_uint levelId, _uint playerId) const
{
    const PlayerData* p = FindPlayer(levelId, playerId);
    return p ? p->pObject : nullptr;
}

// ===== 위치/방향 (현재 레벨 Active) =====
_vector CPlayerManager::GetPos() const
{
    const _uint lev = CurLevelId();
    const LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return XMVectorZero();

    auto it = b->players.find(b->activeId);
    if (it == b->players.end() || !it->second.fnGetPos) return XMVectorZero();
    return it->second.fnGetPos();
}

_vector CPlayerManager::GetForward(_bool flattenY) const
{
    const _uint lev = CurLevelId();
    const LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return XMVectorSet(0.f, 0.f, 1.f, 0.f);

    auto it = b->players.find(b->activeId);
    if (it == b->players.end() || !it->second.fnGetForward) return XMVectorSet(0.f, 0.f, 1.f, 0.f);
    return it->second.fnGetForward(flattenY);
}

_vector CPlayerManager::GetRight() const
{
    const _uint lev = CurLevelId();
    const LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return XMVectorSet(1.f, 0.f, 0.f, 0.f);

    auto it = b->players.find(b->activeId);
    if (it == b->players.end() || !it->second.fnGetRight) return XMVectorSet(1.f, 0.f, 0.f, 0.f);
    return it->second.fnGetRight();
}

_vector CPlayerManager::GetUp() const
{
    const _uint lev = CurLevelId();
    const LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return XMVectorSet(0.f, 1.f, 0.f, 0.f);

    auto it = b->players.find(b->activeId);
    if (it == b->players.end() || !it->second.fnGetUp) return XMVectorSet(0.f, 1.f, 0.f, 0.f);
    return it->second.fnGetUp();
}

// ===== 위치/방향 (지정) =====
_vector CPlayerManager::GetPos(_uint levelId, _uint playerId) const
{
    const PlayerData* p = FindPlayer(levelId, playerId);
    return (p && p->fnGetPos) ? p->fnGetPos() : XMVectorZero();
}
_vector CPlayerManager::GetForward(_uint levelId, _uint playerId, _bool flattenY) const
{
    const PlayerData* p = FindPlayer(levelId, playerId);
    return (p && p->fnGetForward) ? p->fnGetForward(flattenY)
        : XMVectorSet(0.f, 0.f, 1.f, 0.f);
}
_vector CPlayerManager::GetRight(_uint levelId, _uint playerId) const
{
    const PlayerData* p = FindPlayer(levelId, playerId);
    return (p && p->fnGetRight) ? p->fnGetRight()
        : XMVectorSet(1.f, 0.f, 0.f, 0.f);
}
_vector CPlayerManager::GetUp(_uint levelId, _uint playerId) const
{
    const PlayerData* p = FindPlayer(levelId, playerId);
    return (p && p->fnGetUp) ? p->fnGetUp()
        : XMVectorSet(0.f, 1.f, 0.f, 0.f);
}

// ===== HP =====
_float CPlayerManager::GetHP(_uint levelId, _uint playerId) const
{
    const PlayerData* p = FindPlayer(levelId, playerId);
    return p ? p->hp : 0.f;
}
_float CPlayerManager::GetMaxHP(_uint levelId, _uint playerId) const
{
    const PlayerData* p = FindPlayer(levelId, playerId);
    return p ? p->maxHp : 0.f;
}
void CPlayerManager::SetHP(_uint levelId, _uint playerId, _float hp)
{
    PlayerData* p = FindPlayer(levelId, playerId);
    if (!p) return;
    p->hp = clamp_compat(hp, 0.f, p->maxHp);
}
void CPlayerManager::ApplyDamage(_uint levelId, _uint playerId, _float amount)
{
    PlayerData* p = FindPlayer(levelId, playerId);
    if (!p) return;
    const float dmg = max(0.f, amount);
    p->hp = clamp_compat(p->hp - dmg, 0.f, p->maxHp);
}
void CPlayerManager::Heal(_uint levelId, _uint playerId, _float amount)
{
    PlayerData* p = FindPlayer(levelId, playerId);
    if (!p) return;
    const float heal = max(0.f, amount);
    p->hp = clamp_compat(p->hp + heal, 0.f, p->maxHp);
}

// 현재 레벨 Active HP
_float CPlayerManager::GetActiveHP() const
{
    const _uint lev = CurLevelId();
    const LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return 0.f;

    auto it = b->players.find(b->activeId);
    return (it == b->players.end()) ? 0.f : it->second.hp;
}
_float CPlayerManager::GetActiveMaxHP() const
{
    const _uint lev = CurLevelId();
    const LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return 0.f;

    auto it = b->players.find(b->activeId);
    return (it == b->players.end()) ? 0.f : it->second.maxHp;
}
void CPlayerManager::ApplyDamageActive(_float amount)
{
    const _uint lev = CurLevelId();
    LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return;

    PlayerData* p = FindPlayer(lev, b->activeId);
    if (!p) return;
    const float dmg = max(0.f, amount);
    p->hp = clamp_compat(p->hp - dmg, 0.f, p->maxHp);
}
void CPlayerManager::HealActive(_float amount)
{
    const _uint lev = CurLevelId();
    LevelBucket* b = FindBucket(lev);
    if (!b || !b->hasActive) return;

    PlayerData* p = FindPlayer(lev, b->activeId);
    if (!p) return;
    const float heal = max(0.f, amount);
    p->hp = clamp_compat(p->hp + heal, 0.f, p->maxHp);
}

void CPlayerManager::SavePersistentHP(_uint heroSlot, float hp, float maxHp)
{
    if (maxHp <= 0.f) return;           // 0 저장 방지
    hp = clamp_compat(hp, 0.f, maxHp);
    m_persist[heroSlot] = { hp, maxHp };

    wchar_t b[128];
    swprintf(b, 128, L"[PM] Save slot=%u HP=%.1f / Max=%.1f\n", heroSlot, hp, maxHp);
    OutputDebugStringW(b);
}

bool CPlayerManager::ConsumePersistentHP(_uint heroSlot, float& outHP, float& outMaxHP)
{
    auto it = m_persist.find(heroSlot);
    if (it == m_persist.end()) {
        OutputDebugStringW(L"[PM] Consume: not found\n");
        return false;
    }
    if (it->second.max <= 0.f) {
        OutputDebugStringW(L"[PM] Consume: invalid (max<=0)\n");
        return false;
    }
    outHP = clamp_compat(it->second.hp, 0.f, it->second.max);
    outMaxHP = it->second.max;

    wchar_t b[128];
    swprintf(b, 128, L"[PM] Consume slot=%u HP=%.1f / Max=%.1f\n", heroSlot, outHP, outMaxHP);
    OutputDebugStringW(b);

    it->second = {}; // 1회성 소비
    return true;
}

void CPlayerManager::Free()
{
    m_levels.clear();
}
