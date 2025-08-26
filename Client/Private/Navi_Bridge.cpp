#include "Navi_Bridge.h"
#include "GameInstance.h"

CNavi_Bridge::CNavi_Bridge(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{
}

CNavi_Bridge::CNavi_Bridge(const CNavi_Bridge& Prototype)
    : CGameObject{ Prototype }
{
}

HRESULT CNavi_Bridge::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CNavi_Bridge::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CNavi_Bridge::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CNavi_Bridge::Update(_float fTimeDelta)
{
    m_pNavigationCom->Update(m_pTransformCom->Get_WorldMatrix());
}

void CNavi_Bridge::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::PRIORITY, this)))
        return;
}

HRESULT CNavi_Bridge::Render()
{

#ifdef _DEBUG
    m_pNavigationCom->Render();

#endif

    return S_OK;
}

HRESULT CNavi_Bridge::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Navigation"),
        TEXT("Com_Navigation"), reinterpret_cast<CComponent**>(&m_pNavigationCom), nullptr)))
        return E_FAIL;


    return S_OK;
}

HRESULT CNavi_Bridge::Bind_ShaderResources()
{
    return S_OK;
}

CNavi_Bridge* CNavi_Bridge::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CNavi_Bridge* pInstance = new CNavi_Bridge(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CNavi_Bridge"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CNavi_Bridge::Clone(void* pArg)
{
    CNavi_Bridge* pInstance = new CNavi_Bridge(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CNavi_Bridge"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CNavi_Bridge::Free()
{
    __super::Free();

    Safe_Release(m_pNavigationCom);
}
