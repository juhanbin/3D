#include "Navi_Intro.h"
#include "GameInstance.h"

CNavi_Intro::CNavi_Intro(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{
}

CNavi_Intro::CNavi_Intro(const CNavi_Intro& Prototype)
    : CGameObject{ Prototype }
{
}

HRESULT CNavi_Intro::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CNavi_Intro::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components()))
        return E_FAIL;

    return S_OK;
}

void CNavi_Intro::Priority_Update(_float fTimeDelta)
{
    int a = 10;
}

void CNavi_Intro::Update(_float fTimeDelta)
{
    m_pNavigationCom->Update(m_pTransformCom->Get_WorldMatrix());
}

void CNavi_Intro::Late_Update(_float fTimeDelta)
{
    if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::PRIORITY, this)))
        return;
}

HRESULT CNavi_Intro::Render()
{

#ifdef _DEBUG
    m_pNavigationCom->Render();

#endif

    return S_OK;
}

HRESULT CNavi_Intro::Ready_Components()
{
    if (FAILED(CGameObject::Add_Component(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Navigation_Intro"),
        TEXT("Com_Navigation"), reinterpret_cast<CComponent**>(&m_pNavigationCom), nullptr)))
        return E_FAIL;


    return S_OK;
}

HRESULT CNavi_Intro::Bind_ShaderResources()
{
    return S_OK;
}

CNavi_Intro* CNavi_Intro::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CNavi_Intro* pInstance = new CNavi_Intro(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX(TEXT("Failed to Created : CNavi_Intro"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CNavi_Intro::Clone(void* pArg)
{
    CNavi_Intro* pInstance = new CNavi_Intro(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CNavi_Intro"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CNavi_Intro::Free()
{
    __super::Free();

    Safe_Release(m_pNavigationCom);
}
