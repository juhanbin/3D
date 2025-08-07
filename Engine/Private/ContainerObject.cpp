#include "ContainerObject.h"
#include "PartObject.h"

#include "GameInstance.h"

CContainerObject::CContainerObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject { pDevice, pContext }
{

}

CContainerObject::CContainerObject(const CContainerObject& Prototype)
    : CGameObject { Prototype }
{

}

HRESULT CContainerObject::Initialize_Prototype()
{


    return S_OK;
}

HRESULT CContainerObject::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    return S_OK;
}


void CContainerObject::Priority_Update(_float fTimeDelta)
{
    for (auto& Pair : m_PartObjects)
        Pair.second->Priority_Update(fTimeDelta);

}

void CContainerObject::Update(_float fTimeDelta)
{
    for (auto& Pair : m_PartObjects)
        Pair.second->Update(fTimeDelta);
}

void CContainerObject::Late_Update(_float fTimeDelta)
{
    for (auto& Pair : m_PartObjects)
        Pair.second->Late_Update(fTimeDelta);
}

HRESULT CContainerObject::Render()
{

    return S_OK;
}


HRESULT CContainerObject::Add_PartObject(const _wstring& strPartObjectTag, _uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, void* pArg)
{
    if (nullptr != Find_PartObject(strPartObjectTag))
        return E_FAIL;           

    CPartObject*        pPartObject = dynamic_cast<CPartObject*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::GAMEOBJECT, iPrototypeLevelIndex, strPrototypeTag, pArg));
    if (nullptr == pPartObject)
        return E_FAIL;

    m_PartObjects.emplace(strPartObjectTag, pPartObject);

    return S_OK;
}

CPartObject* CContainerObject::Find_PartObject(const _wstring& strPartObjectTag)
{
    auto    iter = m_PartObjects.find(strPartObjectTag);
    if (iter == m_PartObjects.end())
        return nullptr;

    return iter->second;    
}

void CContainerObject::Free()
{
    __super::Free();

    for (auto& Pair : m_PartObjects)
        Safe_Release(Pair.second);

    m_PartObjects.clear();
}


