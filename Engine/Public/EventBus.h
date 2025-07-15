#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CEventBus final : public CBase
{
private:
    CEventBus();
    virtual ~CEventBus() = default;

public:
    using EVENTCALLBACK = function<void(void*)>;

public:
    HRESULT Register_Event(_uint iLevelID, const _wstring& strEventTag, EVENTCALLBACK Callback);
    void    Dispatch_Event(_uint iLevelID, const _wstring& strEventTag, void* pArg = nullptr);
    HRESULT Clear_Level(_uint iLevelID);

private:
    using EVENTKEY = pair<_uint, _wstring>;
    map<EVENTKEY, vector<EVENTCALLBACK>>       m_Events;

public:
    static CEventBus* Create();
    virtual void Free() override;
};

NS_END