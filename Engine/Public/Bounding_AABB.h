#pragma once

#include "Bounding.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBounding_AABB final : public CBounding
{
public:
	typedef struct tagBoundingAABBDesc : public CBounding::BOUNDING_DESC
	{
		_float3		vExtents;
	}BOUNDING_AABB_DESC;
private:
	CBounding_AABB(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CBounding_AABB() = default;

public:
	const BoundingBox* Get_Desc() const {
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
	BoundingBox*		m_pOriginalDesc{};
	BoundingBox*		m_pDesc{};

private:
	_bool Intersect_AABB(const CBounding_AABB* pTarget);
	_float3 Compute_Min()  const;
	_float3 Compute_Max()  const;

public:
	static CBounding_AABB* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CBounding::BOUNDING_DESC* pDesc);
	virtual void Free() override;

};

NS_END