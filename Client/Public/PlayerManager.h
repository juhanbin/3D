#pragma once
#include "Client_Defines.h"
#include "Base.h"
#include <unordered_map>

NS_BEGIN(Engine)
class CTransform; // 있어도 되고 없어도 됨(직접 안 씀)
NS_END

NS_BEGIN(Client)
class CPlayer;

class CPlayerManager final : public CBase {
    DECLARE_SINGLETON(CPlayerManager)
public:
    struct PlayerData {
        CPlayer* pPlayer = nullptr; // 소유 X, 생명주기는 게임오브젝트가 가짐
        _float   hp = 100.f;
        _float   maxHp = 100.f;
    };

    // 등록/선택
    void    Register(_uint id, CPlayer* pPlayer, _float maxHp = 100.f);
    void    Unregister(_uint id);
    void    SetActive(_uint id);
    CPlayer* GetActive() const { return m_pActive; }
    _uint   GetActiveId() const { return m_activeId; }

    // 위치/방향 (카메라/AI용)
    _vector GetPos() const;                         // 활성 플레이어 위치
    _vector GetForward(bool flattenY = true) const; // 전방(필요시 수평화)
    _vector GetRight() const;
    _vector GetUp() const;

    // HP
    _float  GetHP(_uint id) const;
    _float  GetMaxHP(_uint id) const;
    void    SetHP(_uint id, _float hp);
    void    ApplyDamage(_uint id, _float amount);
    void    Heal(_uint id, _float amount);

    // 편의: 활성 대상
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
