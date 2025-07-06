#pragma once

#include "Base.h"

/* 기존레벨과 새로운 레벨을 교체해준다. */
/* 현재 할당되어있는 레벨의 갱신과 렌더링을 담당한다. */

/* 오브젝트 매니져 + 자원 매니져 => 레벨별로 구분해서 보관한다. */
/* 레벨 교체 시에 기존 레벨을 파괴하거나 기존 레벨에서 쓰기위해서 준비해놨던 자원들을 정리한다. */

/* 할당된 레벨을 보관한다. */

NS_BEGIN(Engine)

class CLevel_Manager final : public CBase
{
private:
	CLevel_Manager();
	virtual ~CLevel_Manager() = default;

public:
	HRESULT Open_Level(_uint iLevelID, class CLevel* pNewLevel);
	void Update(_float fTimeDelta);
	HRESULT Render();

private:
	class CLevel*				m_pCurrentLevel = { nullptr };
	class CGameInstance*		m_pGameInstance = { nullptr };
	_uint						m_iCurrentLevelID = { };

private:
	HRESULT Clear_Resources();

public:
	static CLevel_Manager* Create();
	virtual void Free() override;

};

NS_END