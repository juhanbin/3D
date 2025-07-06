#include "Prototype_Manager.h"
#include "GameObject.h"
#include "Component.h"

#include "GameInstance.h"

CPrototype_Manager::CPrototype_Manager()	
{
	
}

HRESULT CPrototype_Manager::Initialize(_uint iNumLevels)
{
	m_pPrototypes = new PROTOTYPES[iNumLevels];

	m_iNumLevels = iNumLevels;

	return S_OK;
}

HRESULT CPrototype_Manager::Add_Prototype(_uint iPrototpyeLevelIndex, const _wstring& strPrototypeTag, CBase* pPrototype)
{
	if (nullptr == m_pPrototypes || 
		m_iNumLevels <= iPrototpyeLevelIndex ||
		nullptr != Find_Prototype(iPrototpyeLevelIndex, strPrototypeTag))
		return E_FAIL;

	m_pPrototypes[iPrototpyeLevelIndex].emplace(strPrototypeTag, pPrototype);

	return S_OK;
}

CBase* CPrototype_Manager::Clone_Prototype(PROTOTYPE ePrototype, _uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, void* pArg)
{
	CBase* pPrototype = Find_Prototype(iPrototypeLevelIndex, strPrototypeTag);

	if (nullptr == pPrototype)
		return nullptr;

	CBase* pGameObject = { nullptr };

	if (PROTOTYPE::GAMEOBJECT == ePrototype)
		pGameObject = dynamic_cast<CGameObject*>(pPrototype)->Clone(pArg);
	else
		pGameObject = dynamic_cast<CComponent*>(pPrototype)->Clone(pArg);

	if (nullptr == pGameObject)
		return nullptr;

	return pGameObject;
}

void CPrototype_Manager::Clear(_uint iLevelIndex)
{
	if (iLevelIndex >= m_iNumLevels)
		return;

	for (auto& Pair : m_pPrototypes[iLevelIndex])
		Safe_Release(Pair.second);

	m_pPrototypes[iLevelIndex].clear();
}


CBase* CPrototype_Manager::Find_Prototype(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag)
{
	/* ÀÌÁøÅ½»ö */
	auto	iter = m_pPrototypes[iPrototypeLevelIndex].find(strPrototypeTag);

	if (iter == m_pPrototypes[iPrototypeLevelIndex].end())
		return nullptr;

	return iter->second;	
}

CPrototype_Manager* CPrototype_Manager::Create(_uint iNumLevels)
{
	CPrototype_Manager* pInstance = new CPrototype_Manager();

	if (FAILED(pInstance->Initialize(iNumLevels)))
	{
		MSG_BOX(TEXT("Failed to Created : CPrototype_Manager"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPrototype_Manager::Free()
{
	__super::Free();

	for (size_t i = 0; i < m_iNumLevels; i++)
	{
		for (auto& Pair : m_pPrototypes[i])
			Safe_Release(Pair.second);
		m_pPrototypes[i].clear();
	}

	Safe_Delete_Array(m_pPrototypes);	
}
