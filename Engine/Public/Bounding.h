#pragma once

#include "Base.h"
#include "DebugDraw.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBounding abstract : public CBase
{
public:
	typedef struct tagBoundingDesc 
	{
		_float3			vCenter;
	}BOUNDING_DESC;
protected:
	CBounding(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);	
	virtual ~CBounding() = default;

public:
	virtual HRESULT Initialize();
	virtual void Update(_fmatrix WorldMatrix) {}
	virtual _bool Intersect(COLLIDER eType, CBounding* pTarget) = 0;



#ifdef _DEBUG
public:
	virtual HRESULT Render(PrimitiveBatch<VertexPositionColor>* pBatch, _fvector vColor) { return S_OK; }
#endif

protected:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };

public:
	virtual void Free() override;
};

NS_END