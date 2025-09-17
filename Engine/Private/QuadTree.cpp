//#include "QuadTree.h"
//#include "GameInstance.h"
//
//
//CQuadTree::CQuadTree()
//{
//}
//
//HRESULT CQuadTree::Initialize(_uint iLT, _uint iRT, _uint iRB, _uint iLB)
//{
//	m_iCorners[CORNER_LT] = iLT;
//	m_iCorners[CORNER_RT] = iRT;
//	m_iCorners[CORNER_RB] = iRB;
//	m_iCorners[CORNER_LB] = iLB;
//
//	if (1 == m_iCorners[CORNER_RT] - m_iCorners[CORNER_LT])
//		return S_OK;
//
//	m_iCenter = (m_iCorners[CORNER_LT] + m_iCorners[CORNER_RB]) >> 1;
//
//	_uint			iLC, iTC, iRC, iBC;
//
//	iLC = (m_iCorners[CORNER_LT] + m_iCorners[CORNER_LB]) >> 1;
//	iTC = (m_iCorners[CORNER_LT] + m_iCorners[CORNER_RT]) >> 1;
//	iRC = (m_iCorners[CORNER_RT] + m_iCorners[CORNER_RB]) >> 1;
//	iBC = (m_iCorners[CORNER_LB] + m_iCorners[CORNER_RB]) >> 1;
//
//	m_pChildren[CORNER_LT] = CQuadTree::Create(m_iCorners[CORNER_LT], iTC, m_iCenter, iLC);
//	m_pChildren[CORNER_RT] = CQuadTree::Create(iTC, m_iCorners[CORNER_RT], iRC, m_iCenter);
//	m_pChildren[CORNER_RB] = CQuadTree::Create(m_iCenter, iRC, m_iCorners[CORNER_RB], iBC);
//	m_pChildren[CORNER_LB] = CQuadTree::Create(iLC, m_iCenter, iBC, m_iCorners[CORNER_LB]);
//
//	return S_OK;
//}
//
//void CQuadTree::Culling(CGameInstance* pGameInstance, const _float3* pVertexPositions, _uint* pIndices, _uint* pNumIndices)
//{
//	if (nullptr == m_pChildren[0] ||
//		true == isDraw(pGameInstance, pVertexPositions))
//	{
//		_uint		iIndices[] = {
//			m_iCorners[0],
//			m_iCorners[1],
//			m_iCorners[2],
//			m_iCorners[3],
//		};
//
//		_bool		isIn[4] = {
//			pGameInstance->isIn_Frustum_LocalSpace(XMVectorSetW(XMLoadFloat3(&pVertexPositions[iIndices[0]]), 1.f)),
//			pGameInstance->isIn_Frustum_LocalSpace(XMVectorSetW(XMLoadFloat3(&pVertexPositions[iIndices[1]]), 1.f)),
//			pGameInstance->isIn_Frustum_LocalSpace(XMVectorSetW(XMLoadFloat3(&pVertexPositions[iIndices[2]]), 1.f)),
//			pGameInstance->isIn_Frustum_LocalSpace(XMVectorSetW(XMLoadFloat3(&pVertexPositions[iIndices[3]]), 1.f)),
//		};
//
//		if (true == isIn[0] ||
//			true == isIn[1] ||
//			true == isIn[2])
//		{
//			pIndices[(*pNumIndices)++] = iIndices[0];
//			pIndices[(*pNumIndices)++] = iIndices[1];
//			pIndices[(*pNumIndices)++] = iIndices[2];
//		}
//
//		if (true == isIn[0] ||
//			true == isIn[2] ||
//			true == isIn[3])
//		{
//			pIndices[(*pNumIndices)++] = iIndices[0];
//			pIndices[(*pNumIndices)++] = iIndices[2];
//			pIndices[(*pNumIndices)++] = iIndices[3];
//		}
//
//		return;
//	}
//
//	/* ³» ÄõµåÆ®¸®¶û ÀýµÎÃ¼°¡ °ãÃÆ´Ï? \*/
//	_float		fRange = XMVector3Length(XMLoadFloat3(&pVertexPositions[m_iCorners[CORNER_LT]]) - XMLoadFloat3(&pVertexPositions[m_iCenter])).m128_f32[0];
//
//	if (true == pGameInstance->isIn_Frustum_LocalSpace(XMLoadFloat3(&pVertexPositions[m_iCenter]), fRange))
//	{
//		for (size_t i = 0; i < CORNER_END; i++)
//		{
//			if (nullptr != m_pChildren[i])
//				m_pChildren[i]->Culling(pGameInstance, pVertexPositions, pIndices, pNumIndices);
//		}
//	}
//}
//
//_bool CQuadTree::isDraw(CGameInstance* pGameInstance, const _float3* pVertexPositions)
//{
//	_vector	vCamPosition = XMLoadFloat4(pGameInstance->Get_CamPosition());
//	_vector vCenterPos = XMLoadFloat3(&pVertexPositions[m_iCenter]);
//
//	if (XMVector3Length(vCamPosition - vCenterPos).m128_f32[0] * 0.2f > (m_iCorners[CORNER_RT] - m_iCorners[CORNER_LT]))
//		return true;
//
//	return false;
//}
//
//CQuadTree* CQuadTree::Create(_uint iLT, _uint iRT, _uint iRB, _uint iLB)
//{
//	CQuadTree* pInstance = new CQuadTree();
//
//	if (FAILED(pInstance->Initialize(iLT, iRT, iRB, iLB)))
//	{
//		MSG_BOX(TEXT("Failed to Created : CQuadTree"));
//		Safe_Release(pInstance);
//	}
//
//	return pInstance;
//}
//
//void CQuadTree::Free()
//{
//	__super::Free();
//}
