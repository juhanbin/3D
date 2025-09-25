#pragma once
#include "Client_Defines.h"
#include "Base.h"
#include <unordered_map>
#include <functional>   // std::function

NS_BEGIN(Engine)
class CTransform;
NS_END

NS_BEGIN(Client)

// 전방선언(구체 타입에 의존하지 않도록)
// class CPlayer;  // 사용 안 함

class CPlayerManager final : public CBase {
    DECLARE_SINGLETON(CPlayerManager)

public:
    struct PlayerData {
        void* pObject = nullptr;  // 실제 플레이어 객체 주소(타입 자유)
        _float  hp = 100.f;
        _float  maxHp = 100.f;

        // 위치/방향 콜백(플레이어 클래스가 달라도 호출 가능)
        std::function<_vector()>              fnGetPos;
        std::function<_vector(_bool)>         fnGetForward;
        std::function<_vector()>              fnGetRight;
        std::function<_vector()>              fnGetUp;
    };

    struct LevelBucket {
        std::unordered_map<_uint, PlayerData> players;
        _uint   activeId = 0;
        bool    hasActive = false;
    };

public:
    // ============== 등록/해제 ==============
    template<typename T>
    void Register(_uint levelId, _uint playerId, T* pPlayer, _float maxHp = 100.f) {
        LevelBucket& bucket = m_levels[levelId];

        // 기존 등록 여부 확인
        auto itOld = bucket.players.find(playerId);

        PlayerData d;

        if (itOld != bucket.players.end() && maxHp < 0.f) {
            // -1.f 등 음수면 기존 HP/MaxHP 유지
            d = itOld->second;             // hp, maxHp 그대로 가져옴
        }
        else {
            // 새로 초기화 (음수지만 기존이 없으면 기본치로)
            d.maxHp = (maxHp >= 0.f) ? maxHp : 100.f;
            d.hp = d.maxHp;
        }

        // 객체 포인터/콜백은 항상 갱신
        d.pObject = static_cast<void*>(pPlayer);
        d.fnGetPos = [pPlayer]() { return pPlayer->GetPos(); };
        d.fnGetForward = [pPlayer](_bool flattenY) { return pPlayer->GetForward(flattenY); };
        d.fnGetRight = [pPlayer]() { return pPlayer->GetRight(); };
        d.fnGetUp = [pPlayer]() { return pPlayer->GetUp(); };

        bucket.players[playerId] = std::move(d);

        if (!bucket.hasActive) {
            bucket.activeId = playerId;
            bucket.hasActive = true;
        }
    }


    void Unregister(_uint levelId, _uint playerId);

    // ============== Active 선택 ==============
    void SetActive(_uint levelId, _uint playerId);
    void SetActiveByCurrentLevel(_uint playerId);

    // 현재 레벨의 Active 객체 포인터(필요 시 캐스팅해서 사용)
    void* GetActiveRaw() const;
    _uint GetActiveId() const;

    // 특정 레벨/플레이어의 Raw 포인터가 필요할 때
    void* GetRaw(_uint levelId, _uint playerId) const;

    // ============== 위치/방향 (카메라 등) ==============
    // 현재 레벨의 Active 기준
    _vector GetPos() const;
    _vector GetForward(_bool flattenY = true) const;
    _vector GetRight() const;
    _vector GetUp() const;

    // 특정 레벨/플레이어 기준
    _vector GetPos(_uint levelId, _uint playerId) const;
    _vector GetForward(_uint levelId, _uint playerId, _bool flattenY = true) const;
    _vector GetRight(_uint levelId, _uint playerId) const;
    _vector GetUp(_uint levelId, _uint playerId) const;

    // ============== HP ==============
    _float  GetHP(_uint levelId, _uint playerId) const;
    _float  GetMaxHP(_uint levelId, _uint playerId) const;
    void    SetHP(_uint levelId, _uint playerId, _float hp);
    void    ApplyDamage(_uint levelId, _uint playerId, _float amount);
    void    Heal(_uint levelId, _uint playerId, _float amount);

    // 현재 레벨 Active 기준 HP
    _float  GetActiveHP() const;
    _float  GetActiveMaxHP() const;
    void    ApplyDamageActive(_float amount);
    void    HealActive(_float amount);

private:
    CPlayerManager() = default;
    virtual ~CPlayerManager() = default;
    virtual void Free() override;

private:
    static inline _uint CurLevelId(); // GameInstance에서 현재 레벨 조회
    const LevelBucket* FindBucket(_uint levelId) const;
    LevelBucket* FindBucket(_uint levelId);

    const PlayerData* FindPlayer(_uint levelId, _uint playerId) const;
    PlayerData* FindPlayer(_uint levelId, _uint playerId);

    template<typename T>
    inline T clamp_compat(T v, T lo, T hi) const {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

private:
    // 레벨ID -> 버킷
    std::unordered_map<_uint, LevelBucket> m_levels;
};

NS_END
