#include "GameObject.h"

#include "GameInstance.h"

CGameObject::CGameObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

CGameObject::CGameObject(const CGameObject& Prototype)
	: m_pDevice{ Prototype.m_pDevice }
	, m_pContext{ Prototype.m_pContext }
	, m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

CComponent* CGameObject::Get_Component(const _wstring& strComponentTag)
{
	auto	iter = m_Components.find(strComponentTag);
	if (iter == m_Components.end())
		return nullptr;

	return iter->second;
}

HRESULT CGameObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CGameObject::Initialize(void* pArg)
{
	m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);
	if (nullptr == m_pTransformCom)
		return E_FAIL;

	if (FAILED(m_pTransformCom->Initialize(pArg)))
		return E_FAIL;

	m_Components.emplace(TEXT("Com_Transform"), m_pTransformCom);

	Safe_AddRef(m_pTransformCom);

	m_bActive = true;

	return S_OK;
}

void CGameObject::Priority_Update(_float fTimeDelta)
{
	if (!m_bActive) return;
}

void CGameObject::Update(_float fTimeDelta)
{
	if (!m_bActive) return;
}

void CGameObject::Late_Update(_float fTimeDelta)
{
	if (!m_bActive) return;
}

HRESULT CGameObject::Render()
{
	if (!m_bActive) return S_OK;
	return S_OK;
}

HRESULT CGameObject::Add_Component(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, const _wstring& strComponentTag, CComponent** ppOut, void* pArg)
{
	if (nullptr != Get_Component(strComponentTag))
		return E_FAIL;

	CComponent* pComponent = dynamic_cast<CComponent*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::COMPONENT, iPrototypeLevelIndex, strPrototypeTag, pArg));
	if (nullptr == pComponent)
		return E_FAIL;

	m_Components.emplace(strComponentTag, pComponent);

	*ppOut = pComponent;

	Safe_AddRef(pComponent);

	return S_OK;
}

HRESULT CGameObject::Add_GameObject_ToLayer(_uint iLayerLevelIndex, const _wstring& strLayerTag, CGameObject* pGameObject)
{
	if (!pGameObject) return E_FAIL;
	if (iLayerLevelIndex >= m_Layers.size()) return E_FAIL;

	CLayer* pLayer = Find_Layer(iLayerLevelIndex, strLayerTag);
	if (!pLayer) {
		pLayer = CLayer::Create();
		if (!pLayer) return E_FAIL;

		// 레이어 테이블에 먼저 등록
		m_Layers[iLayerLevelIndex].emplace(strLayerTag, pLayer);

		// 오브젝트 추가 실패 시 롤백
		if (FAILED(pLayer->Add_GameObject(pGameObject))) {
			m_Layers[iLayerLevelIndex].erase(strLayerTag);
			Safe_Release(pLayer);
			return E_FAIL;
		}
		return S_OK;
	}

	// 기존 레이어가 있을 때 실패 처리
	if (FAILED(pLayer->Add_GameObject(pGameObject)))
		return E_FAIL;

	return S_OK;
}

void CGameObject::Free()
{
	__super::Free();

	for (auto& Pair : m_Components)
		Safe_Release(Pair.second);
	m_Components.clear();

	Safe_Release(m_pTransformCom);
	Safe_Release(m_pGameInstance);
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}
