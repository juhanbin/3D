#include "PlayerManager.h"
#include "Player.h"
#include <algorithm>

USING(Client)

IMPLEMENT_SINGLETON(CPlayerManager)

void CPlayerManager::Register(_uint id, CPlayer* pPlayer, _float maxHp)
{
    PlayerData d; d.pPlayer = pPlayer; d.maxHp = maxHp; d.hp = maxHp;
    m_players[id] = d;
    if (!m_pActive) { m_pActive = pPlayer; m_activeId = id; }
}

void CPlayerManager::Unregister(_uint id)
{
    const bool wasActive = (m_activeId == id);
    m_players.erase(id);

    if (wasActive) {
        m_pActive = nullptr;
        m_activeId = 0;
        if (!m_players.empty()) {
            auto it = m_players.begin();
            m_pActive = it->second.pPlayer;
            m_activeId = it->first;
        }
    }
}

void CPlayerManager::SetActive(_uint id)
{
    auto it = m_players.find(id);
    if (it == m_players.end()) return;
    m_pActive = it->second.pPlayer;
    m_activeId = id;
}

// ---------- 위치/방향 : Player의 편의 Getter 사용 ----------
_vector CPlayerManager::GetPos() const
{
    return m_pActive ? m_pActive->GetPos() : XMVectorZero();
}

_vector CPlayerManager::GetForward(bool flattenY) const
{
    return m_pActive ? m_pActive->GetForward(flattenY)
        : XMVectorSet(0.f, 0.f, 1.f, 0.f);
}

_vector CPlayerManager::GetRight() const
{
    return m_pActive ? m_pActive->GetRight()
        : XMVectorSet(1.f, 0.f, 0.f, 0.f);
}

_vector CPlayerManager::GetUp() const
{
    return m_pActive ? m_pActive->GetUp()
        : XMVectorSet(0.f, 1.f, 0.f, 0.f);
}
// ----------------------------------------------------------

// ---- HP ----
_float CPlayerManager::GetHP(_uint id) const
{
    auto it = m_players.find(id);
    return (it != m_players.end()) ? it->second.hp : 0.f;
}

_float CPlayerManager::GetMaxHP(_uint id) const
{
    auto it = m_players.find(id);
    return (it != m_players.end()) ? it->second.maxHp : 0.f;
}

void CPlayerManager::SetHP(_uint id, _float hp)
{
    auto it = m_players.find(id);
    if (it == m_players.end()) return;
    it->second.hp = clamp_compat(hp, 0.f, it->second.maxHp);
}

void CPlayerManager::ApplyDamage(_uint id, _float amount)
{
    auto it = m_players.find(id);
    if (it == m_players.end()) return;
    const float dmg = max(0.f, amount);
    it->second.hp = clamp_compat(it->second.hp - dmg, 0.f, it->second.maxHp);
}

void CPlayerManager::Heal(_uint id, _float amount)
{
    auto it = m_players.find(id);
    if (it == m_players.end()) return;
    const float heal = max(0.f, amount);
    it->second.hp = clamp_compat(it->second.hp + heal, 0.f, it->second.maxHp);
}

_float CPlayerManager::GetActiveHP() const { return GetHP(m_activeId); }
void   CPlayerManager::ApplyDamageActive(_float amount) { ApplyDamage(m_activeId, amount); }
void   CPlayerManager::HealActive(_float amount) { Heal(m_activeId, amount); }

void CPlayerManager::Free()
{
    m_players.clear();
    m_pActive = nullptr;
    m_activeId = 0;
}
