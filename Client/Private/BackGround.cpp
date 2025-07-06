#include "BackGround.h"

CBackGround::CBackGround(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
}

CBackGround::CBackGround(const CBackGround& Prototype)
{
}

HRESULT CBackGround::Initialize_Prototype()
{
    return E_NOTIMPL;
}

HRESULT CBackGround::Initialize(void* pArg)
{
    return E_NOTIMPL;
}

void CBackGround::Priority_Update(_float fTimeDelta)
{
}

void CBackGround::Update(_float fTimeDelta)
{
}

void CBackGround::Late_Update(_float fTimeDelta)
{
}

HRESULT CBackGround::Render()
{
    return E_NOTIMPL;
}

CBackGround* CBackGround::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    return nullptr;
}

CGameObject* CBackGround::Clone(void* pArg)
{
    return nullptr;
}

void CBackGround::Free()
{
}
