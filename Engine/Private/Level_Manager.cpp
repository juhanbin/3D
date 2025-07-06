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

void CLevel_Manager::Update(_float fTimeDelta)
{
	if (nullptr == m_pCurrentLevel)
		return;

	m_pCurrentLevel->Update(fTimeDelta);
}

HRESULT CLevel_Manager::Render()
{
	if (nullptr == m_pCurrentLevel)
		return E_FAIL;

	return m_pCurrentLevel->Render();
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
