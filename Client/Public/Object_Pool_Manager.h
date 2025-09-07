#pragma once
#include "Client_Defines.h"
#include "Base.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)

class CObject_Pool_Manager final : public CBase
{
    DECLARE_SINGLETON(CObject_Pool_Manager)

private:
    CObject_Pool_Manager();                 // 레벨 수만큼 컨테이너 리사이즈
    virtual ~CObject_Pool_Manager() = default;

public:
    using FactoryFunc = function<CGameObject* ()>;

    // 풀 등록
    HRESULT Register_Pool(LEVEL level, const _tchar* pLayerTag, size_t capacity, FactoryFunc factory);

    // 하나 꺼내기 (없으면 factory로 추가 생성)
    CGameObject* Acquire(LEVEL level, const _tchar* pLayerTag, void* pInitArg = nullptr);

    // 반납
    void Release(LEVEL level, const _tchar* pLayerTag, CGameObject* pObj);

    // 전부 정리 (소유권 정책에 맞게 적절히)
    void Clear_All();

private:
    struct Pool {
        vector<CGameObject*> freeList;
        vector<CGameObject*> usedList;
        FactoryFunc factory;
    };

    vector<unordered_map<wstring, Pool>> m_byLevel;

private:
    static inline int ToIndex(LEVEL lv) { return ENUM_CLASS(lv); }
    static inline wstring ToLayerKey(const _tchar* p) { return p ? wstring(p) : L""; }

    void SetActive(CGameObject* pObj, bool active);
    void OnAcquire(CGameObject* pObj, void* pInitArg);
    void OnRelease(CGameObject* pObj);
};

NS_END
