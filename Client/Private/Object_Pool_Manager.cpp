#include "Object_Pool_Manager.h"
#include "GameInstance.h"
#include "GameObject.h"
#include <algorithm>

USING(Client)
USING(Engine)

IMPLEMENT_SINGLETON(CObject_Pool_Manager)

CObject_Pool_Manager::CObject_Pool_Manager()
{
    m_byLevel.resize(ENUM_CLASS(LEVEL::END));
}

HRESULT CObject_Pool_Manager::Register_Pool(LEVEL level, const _tchar* pLayerTag, size_t capacity, FactoryFunc factory)
{
    const int li = ToIndex(level);
    auto& layerMap = m_byLevel[li];
    auto& pool = layerMap[ToLayerKey(pLayerTag)];
    pool.factory = factory;

    for (size_t i = 0; i < capacity; ++i) {
        CGameObject* obj = factory ? factory() : nullptr;
        if (!obj) return E_FAIL;
        SetActive(obj, false);
        pool.freeList.push_back(obj);
    }
    return S_OK;
}

Engine::CGameObject* CObject_Pool_Manager::Acquire(LEVEL level, const _tchar* pLayerTag, void* pInitArg)
{
    const int li = ToIndex(level);
    auto& layerMap = m_byLevel[li];
    auto it = layerMap.find(ToLayerKey(pLayerTag));
    if (it == layerMap.end()) return nullptr;

    Pool& pool = it->second;
    CGameObject* obj = nullptr;

    if (!pool.freeList.empty()) {
        obj = pool.freeList.back();
        pool.freeList.pop_back();
    }
    else {
        obj = pool.factory ? pool.factory() : nullptr;
        if (!obj) return nullptr;
    }

    pool.usedList.push_back(obj);
    OnAcquire(obj, pInitArg);
    SetActive(obj, true);
    return obj;
}

void CObject_Pool_Manager::Release(LEVEL level, const _tchar* pLayerTag, CGameObject* pObj)
{
    if (!pObj) return;

    const int li = ToIndex(level);
    auto& layerMap = m_byLevel[li];
    auto it = layerMap.find(ToLayerKey(pLayerTag));
    if (it == layerMap.end()) return;

    Pool& pool = it->second;

    // usedList에서 제거
    auto uit = std::find(pool.usedList.begin(), pool.usedList.end(), pObj);
    if (uit != pool.usedList.end())
        pool.usedList.erase(uit);

    OnRelease(pObj);
    SetActive(pObj, false);
    pool.freeList.push_back(pObj);
}

void CObject_Pool_Manager::Clear_All()
{
    for (auto& layerMap : m_byLevel) {
        layerMap.clear();
    }
}

void CObject_Pool_Manager::SetActive(CGameObject* pObj, bool active)
{
    pObj->Set_Active(active);
}

void CObject_Pool_Manager::OnAcquire(CGameObject* pObj, void* pInitArg)
{
    pObj->Reuse_Begin(pInitArg);
}

void CObject_Pool_Manager::OnRelease(CGameObject* pObj)
{
    pObj->Reuse_End();
}
