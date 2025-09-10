#include "Layer.h"

#include "GameObject.h"

CLayer::CLayer()
{

}

CComponent* CLayer::Get_Component(const _wstring& strComponentTag, _uint iIndex)
{
	/*auto	iter = m_GameObjects.begin();

	for (size_t i = 0; i < iIndex; i++)
		++iter;
	
	return (*iter)->Get_Component(strComponentTag);*/

	if (m_GameObjects.empty()) return nullptr;
	if (iIndex >= m_GameObjects.size()) return nullptr;

	auto it = m_GameObjects.begin();
	std::advance(it, iIndex);
	if (it == m_GameObjects.end() || !*it) return nullptr;

	return (*it)->Get_Component(strComponentTag);
}

CGameObject* CLayer::Get_GameObject(_uint iIndex)
{
	/*auto	iter = m_GameObjects.begin();

	for (size_t i = 0; i < iIndex; i++)
		++iter;

	return *iter;*/
	if (m_GameObjects.empty()) return nullptr;
	if (iIndex >= m_GameObjects.size()) return nullptr;

	auto it = m_GameObjects.begin();
	std::advance(it, iIndex);
	return (it != m_GameObjects.end()) ? *it : nullptr;
}

void CLayer::Priority_Update(_float fTimeDelta)
{
	for (auto& pGameObject : m_GameObjects)
	{
		if (nullptr != pGameObject)
			pGameObject->Priority_Update(fTimeDelta);
	}
}

void CLayer::Update(_float fTimeDelta)
{
	for (auto& pGameObject : m_GameObjects)
	{
		if (nullptr != pGameObject)
			pGameObject->Update(fTimeDelta);
	}
}

void CLayer::Late_Update(_float fTimeDelta)
{
	for (auto& pGameObject : m_GameObjects)
	{
		if (nullptr != pGameObject)
			pGameObject->Late_Update(fTimeDelta);
	}
}

void CLayer::Clear()
{
	for (auto& pGameObject : m_GameObjects)
		Safe_Release(pGameObject);
	m_GameObjects.clear();
}

CLayer* CLayer::Create()
{
	return new CLayer();
}

void CLayer::Free()
{
	__super::Free();

	for (auto& pGameObject : m_GameObjects)
		Safe_Release(pGameObject);

	m_GameObjects.clear();
	
}
