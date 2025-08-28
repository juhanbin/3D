#pragma once
#include "Client_Defines.h"
#include "Base.h"
#include <unordered_map>

NS_BEGIN(Engine)
class CTransform;
NS_END

NS_BEGIN(Client)
class CPlayer;

class CPlayerManager final : public CBase {
    DECLARE_SINGLETON(CPlayerManager)
public:
    struct PlayerData {
        CPlayer* pPlayer = nullptr; 
        _float   hp = 100.f;
        _float   maxHp = 100.f;
    };

    // 등록/선택
    void    Register(_uint id, CPlayer* pPlayer, _float maxHp = 100.f);
    void    Unregister(_uint id);
    void    SetActive(_uint id);
    CPlayer* GetActive() const { return m_pActive; }
    _uint   GetActiveId() const { return m_activeId; }

    // 위치/방향 (카메라용)
    _vector GetPos() const;                         
    _vector GetForward(bool flattenY = true) const; 
    _vector GetRight() const;
    _vector GetUp() const;

    // HP
    _float  GetHP(_uint id) const;
    _float  GetMaxHP(_uint id) const;
    void    SetHP(_uint id, _float hp);
    void    ApplyDamage(_uint id, _float amount);
    void    Heal(_uint id, _float amount);


    _float  GetActiveHP() const;
    void    ApplyDamageActive(_float amount);
    void    HealActive(_float amount);

    template<typename T>
    inline T clamp_compat(T v, T lo, T hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }
private:
    CPlayerManager() = default;
    virtual ~CPlayerManager() = default;
    virtual void Free() override;

private:
    std::unordered_map<_uint, PlayerData> m_players;
    CPlayer* m_pActive = nullptr;
    _uint    m_activeId = 0;
};
NS_END
