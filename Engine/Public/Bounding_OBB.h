#pragma once

#include "Bounding.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBounding_OBB final : public CBounding
{
public:
	typedef struct tagBoundingOBBDesc : public CBounding::BOUNDING_DESC
	{
		_float3		vExtents;
		_float3		vAngles;
	}BOUNDING_OBB_DESC;

	typedef struct tagOBBDesc
	{
		_float3		vCenter;
		_float3		vAlignDir[3];
		_float3		vCenterDir[3];
	}OBBDESC;
private:
	CBounding_OBB(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CBounding_OBB() = default;
public:
	const BoundingOrientedBox* Get_Desc() const {
		return m_pDesc;
	}
public:
	HRESULT Initialize(const CBounding::BOUNDING_DESC* pDesc);
	virtual void Update(_fmatrix WorldMatrix) override;
	virtual _bool Intersect(COLLIDER eType, CBounding* pTarget) override;
#ifdef _DEBUG
public:
	virtual HRESULT Render(PrimitiveBatch<VertexPositionColor>* pBatch, _fvector vColor) override;
#endif

private:
	BoundingOrientedBox*		m_pOriginalDesc{};
	BoundingOrientedBox*		m_pDesc{};

private:
	_bool Intersect_OBB(const CBounding_OBB* pTarget);
	OBBDESC Compute_OBBDesc() const;

public:
	static CBounding_OBB* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CBounding::BOUNDING_DESC* pDesc);
	virtual void Free() override;

};

NS_END