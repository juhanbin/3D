#include "EventBus.h"

CEventBus::CEventBus()
{
}

HRESULT CEventBus::Register_Event(_uint iLevelID, const _wstring& strEventTag, EVENTCALLBACK Callback)
{
    EVENTKEY key{ iLevelID, strEventTag };
    m_Events[key].push_back(Callback);
    return S_OK;
}

void CEventBus::Dispatch_Event(_uint iLevelID, const _wstring& strEventTag, void* pArg)
{
    EVENTKEY key{ iLevelID, strEventTag };
    auto iter = m_Events.find(key);
    if (iter == m_Events.end())
        return;

    for (auto& func : iter->second)
        func(pArg);
}

HRESULT CEventBus::Clear_Level(_uint iLevelID)
{
    for (auto iter = m_Events.begin(); iter != m_Events.end(); )
    {
        if (iter->first.first == iLevelID)
            iter = m_Events.erase(iter);
        else
            ++iter;
    }

    return S_OK;
}

CEventBus* CEventBus::Create()
{
    return new CEventBus();
}

void CEventBus::Free()
{
    __super::Free();
    m_Events.clear();
}