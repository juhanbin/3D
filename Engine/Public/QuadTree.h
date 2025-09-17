//#pragma once
//
//#include "Base.h"
//
//NS_BEGIN(Engine)
//
//class CQuadTree final : public CBase
//{
//public:
//	enum CORNER { CORNER_LT, CORNER_RT, CORNER_RB, CORNER_LB, CORNER_END };
//
//private:
//	CQuadTree();
//	virtual ~CQuadTree() = default;
//
//public:
//	HRESULT Initialize(_uint iLT, _uint iRT, _uint iRB, _uint iLB);
//	void Culling(class CGameInstance* pGameInstance, const _float3* pVertexPositions, _uint* pIndices, _uint* pNumIndices);
//	_bool isDraw(class CGameInstance* pGameInstance, const _float3* pVertexPositions);
//private:
//	_uint		m_iCorners[CORNER_END] = {};
//	CQuadTree* m_pChildren[CORNER_END] = {};
//
//	_uint		m_iCenter = {};
//	_float		m_fRange = {};
//
//public:
//	static CQuadTree* Create(_uint iLT, _uint iRT, _uint iRB, _uint iLB);
//	virtual void Free() override;
//};
//
//NS_END