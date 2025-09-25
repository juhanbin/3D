#include "Level_Manager.h"
#include "GameInstance.h"

#include "Level.h"

CLevel_Manager::CLevel_Manager()
	: m_pGameInstance { CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
}

HRESULT CLevel_Manager::Open_Level(_uint iLevelID, CLevel* pNewLevel)
{
	/* 기존레벨용 자원을 파괴한다. */
	if (FAILED(Clear_Resources()))
		return E_FAIL;

	if (0 != Safe_Release(m_pCurrentLevel))
		MSG_BOX(TEXT("Failed to Change Level"));	

	m_pCurrentLevel = pNewLevel;		

	m_iCurrentLevelID = iLevelID;	

	return S_OK;
}

void CLevel_Manager::Queue_Open_Level(_uint iLevelID, std::function<CLevel* ()> factory)
{
	m_Pending.pending = true;
	m_Pending.id = iLevelID;
	m_Pending.factory = std::move(factory);
}

void CLevel_Manager::Update(_float fTimeDelta)
{
	if (nullptr == m_pCurrentLevel)
		return;

	m_pCurrentLevel->Update(fTimeDelta);


}

HRESULT CLevel_Manager::Render()
{
	if (!m_pCurrentLevel) return E_FAIL;

	HRESULT hr = m_pCurrentLevel->Render();

	// ★ 이번 프레임을 다 그린 뒤에 안전하게 레벨 전환
	if (m_Pending.pending) {
		CLevel* pNew = m_Pending.factory ? m_Pending.factory() : nullptr;
		if (pNew) Open_Level(m_Pending.id, pNew);
		m_Pending = {};
	}
	return hr;
}


HRESULT CLevel_Manager::Clear_Resources()
{
	if (nullptr != m_pCurrentLevel)
		return m_pGameInstance->Clear_Resources(m_iCurrentLevelID);

	return S_OK;
}

CLevel_Manager* CLevel_Manager::Create()
{
	return new CLevel_Manager();
}

void CLevel_Manager::Free()
{
	__super::Free();

	Safe_Release(m_pCurrentLevel);
	Safe_Release(m_pGameInstance);


}
