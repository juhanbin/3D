#include "Navigation.h"
#include "Cell.h"


#include "GameInstance.h"

_float4x4 CNavigation::m_WorldMatrix = {};

CNavigation::CNavigation(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{
}

CNavigation::CNavigation(const CNavigation& Prototype)
	: CComponent{ Prototype }
	, m_Cells{ Prototype.m_Cells }
#ifdef _DEBUG	
	, m_pShader{ Prototype.m_pShader }
#endif
{
	for (auto& pCell : m_Cells)
		Safe_AddRef(pCell);

#ifdef _DEBUG	
	Safe_AddRef(m_pShader);
#endif
}

HRESULT CNavigation::Initialize_Prototype(const _tchar* pNavigationFilePath)
{
	_ulong			dwByte = { };
	HANDLE			hFile = CreateFile(pNavigationFilePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	while (true)
	{
		_float3		vPoints[3];

		ReadFile(hFile, vPoints, sizeof(_float3) * 3, &dwByte, nullptr);
		if (0 == dwByte)
			break;

		CCell* pCell = CCell::Create(m_pDevice, m_pContext, vPoints, m_Cells.size());
		if (nullptr == pCell)
			return E_FAIL;

		m_Cells.push_back(pCell);
	}

	SetUp_Neighbors();

#ifdef _DEBUG
	m_pShader = CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Engine_Shader_Cell.hlsl"), VTXPOS::Elements, VTXPOS::iNumElements);
	if (nullptr == m_pShader)
		return E_FAIL;
#endif

	return S_OK;
}

HRESULT CNavigation::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return S_OK;

	NAVIGATION_DESC* pDesc = static_cast<NAVIGATION_DESC*>(pArg);

	m_iCurrentCellIndex = pDesc->iCurrentCellIndex;

	return S_OK;
}

void CNavigation::Update(_fmatrix WorldMatrix)
{
	XMStoreFloat4x4(&m_WorldMatrix, WorldMatrix);

}

_bool CNavigation::isMove(_fvector vPosition)
{
	_vector vLocalPos = XMVector3TransformCoord(vPosition, XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_WorldMatrix)));

	_int		iNeighborIndex = { -1 };

	if (true == m_Cells[m_iCurrentCellIndex]->isIn(vLocalPos, &iNeighborIndex))
		return true;
	else
	{
		if (-1 != iNeighborIndex)/*ㅣㅇ웃이 있으면 */
		{
			while (true)
			{
				if (-1 == iNeighborIndex)
					return false;

				if (true == m_Cells[iNeighborIndex]->isIn(vLocalPos, &iNeighborIndex))
					break;
			}

			m_iCurrentCellIndex = iNeighborIndex;

			return true;
		}
		else
			/* 이웃이 없으면 */
			return false;
	}
}

_vector CNavigation::Compute_OnCell(_fvector vPosition)
{
	_vector vLocalPos = XMVector3TransformCoord(vPosition, XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_WorldMatrix)));

	_float		fHeight = m_Cells[m_iCurrentCellIndex]->Compute_Height(vLocalPos);

	vLocalPos = XMVectorSetY(vLocalPos, fHeight);

	return XMVector3TransformCoord(vLocalPos, XMLoadFloat4x4(&m_WorldMatrix));


}

#ifdef _DEBUG

HRESULT CNavigation::Render()
{
	if (FAILED(m_pShader->Bind_Matrix("g_WorldMatrix", &m_WorldMatrix)))
		return E_FAIL;
	if (FAILED(m_pShader->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
		return E_FAIL;
	if (FAILED(m_pShader->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
		return E_FAIL;

	_float4		vColor = {};

	_float4x4	WorldMatrix = m_WorldMatrix;

	if (-1 == m_iCurrentCellIndex)
	{
		vColor = _float4(0.f, 1.f, 0.f, 1.f);

		if (FAILED(m_pShader->Bind_Matrix("g_WorldMatrix", &WorldMatrix)))
			return E_FAIL;

		m_pShader->Bind_RawValue("g_vColor", &vColor, sizeof(_float4));

		m_pShader->Begin(0);

		for (auto& pCell : m_Cells)
			pCell->Render();
	}

	else
	{
		vColor = _float4(1.f, 0.f, 0.f, 1.f);

		WorldMatrix._42 += 0.1f;

		if (FAILED(m_pShader->Bind_Matrix("g_WorldMatrix", &WorldMatrix)))
			return E_FAIL;

		m_pShader->Bind_RawValue("g_vColor", &vColor, sizeof(_float4));

		m_pShader->Begin(0);

		m_Cells[m_iCurrentCellIndex]->Render();
	}



	return S_OK;
}

#endif

void CNavigation::SetUp_Neighbors()
{
	for (auto& pSourCell : m_Cells)
	{
		for (auto& pDestCell : m_Cells)
		{
			if (pSourCell == pDestCell)
				continue;

			if (true == pDestCell->Compare_Points(pSourCell->Get_Point(NAVPOINT::A), pSourCell->Get_Point(NAVPOINT::B)))
			{
				pSourCell->Set_Neighbor(LINE::AB, pDestCell);
			}
			if (true == pDestCell->Compare_Points(pSourCell->Get_Point(NAVPOINT::B), pSourCell->Get_Point(NAVPOINT::C)))
			{
				pSourCell->Set_Neighbor(LINE::BC, pDestCell);
			}
			if (true == pDestCell->Compare_Points(pSourCell->Get_Point(NAVPOINT::C), pSourCell->Get_Point(NAVPOINT::A)))
			{
				pSourCell->Set_Neighbor(LINE::CA, pDestCell);
			}
		}
	}
}

CNavigation* CNavigation::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pNavigationFilePath)
{
	CNavigation* pInstance = new CNavigation(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(pNavigationFilePath)))
	{
		MSG_BOX(TEXT("Failed to Created : CNavigation"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CNavigation::Clone(void* pArg)
{
	CNavigation* pInstance = new CNavigation(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX(TEXT("Failed to Cloned : CNavigation"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CNavigation::Free()
{
	__super::Free();

	for (auto& pCell : m_Cells)
		Safe_Release(pCell);

	m_Cells.clear();

#ifdef _DEBUG	
	Safe_Release(m_pShader);
#endif
}
