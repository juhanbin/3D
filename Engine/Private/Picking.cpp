#include "..\Public\Picking.h"
#include "Transform.h"
#include "GameInstance.h"

IMPLEMENT_SINGLETON(CPicking)

CPicking::CPicking()
{
}


HRESULT CPicking::Initialize(HWND hWnd, _uint iWinCX, _uint iWinCY, ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	m_pDevice = pDevice;
	m_pContext = pContext;

	Safe_AddRef(m_pDevice); Safe_AddRef(m_pContext);

	m_hWnd = hWnd;

	m_iWinCX = iWinCX;

	m_iWinCY = iWinCY;

	return S_OK;
}

void CPicking::Tick()
{
    POINT pt; GetCursorPos(&pt);
    ScreenToClient(m_hWnd, &pt);

    Engine::CGameInstance* pGI = Engine::CGameInstance::GetInstance();
    Safe_AddRef(pGI);

    _matrix V = pGI->Get_Transform_Matrix(D3DTS::VIEW);
    _matrix P = pGI->Get_Transform_Matrix(D3DTS::PROJ);
    _matrix I = XMMatrixIdentity();

    // 화면좌표 → 월드로 바로 언프로젝션
    XMVECTOR spNear = XMVectorSet((float)pt.x, (float)pt.y, 0.f, 1.f);
    XMVECTOR spFar = XMVectorSet((float)pt.x, (float)pt.y, 1.f, 1.f);

    XMVECTOR worldNear = XMVector3Unproject(spNear, 0.f, 0.f, (float)m_iWinCX, (float)m_iWinCY, 0.f, 1.f, P, V, I);
    XMVECTOR worldFar = XMVector3Unproject(spFar, 0.f, 0.f, (float)m_iWinCX, (float)m_iWinCY, 0.f, 1.f, P, V, I);

    XMVECTOR dir = XMVector3Normalize(worldFar - worldNear);
    XMStoreFloat3(&m_vRayPos, worldNear);
    XMStoreFloat3(&m_vRayDir, dir);

    Safe_Release(pGI);
}



void CPicking::Transform_ToLocalSpace(CTransform* pTransform)
{
    _matrix WInv = pTransform->Get_WorldMatrix_Inverse();

    _vector vRayPos = XMVectorSetW(XMLoadFloat3(&m_vRayPos), 1.f);
    _vector vRayDir = XMLoadFloat3(&m_vRayDir);

    //
    vRayPos = XMVector3TransformCoord(vRayPos, WInv);
    vRayDir = XMVector3TransformNormal(vRayDir, WInv);
    vRayDir = XMVector3Normalize(vRayDir);

    XMStoreFloat3(&m_vRayPos_Local, vRayPos);
    XMStoreFloat3(&m_vRayDir_Local, vRayDir);
}



void CPicking::Compute_LocalRayInfo(_float3* pRayDir, _float3* pRayPos, CTransform* pTransform)
{
	_matrix		WorldMatrixInv = pTransform->Get_WorldMatrix_Inverse();

	XMStoreFloat3(pRayPos, XMVector3TransformCoord(XMLoadFloat3(&m_vRayPos), WorldMatrixInv));
	XMStoreFloat3(pRayDir, XMVector3TransformNormal(XMLoadFloat3(&m_vRayDir), WorldMatrixInv));
}

void CPicking::Free()
{
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}
