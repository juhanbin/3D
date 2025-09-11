#pragma once

#include "VIBuffer_Instance.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer_Rect_Instance final : public CVIBuffer_Instance
{
public:
	/* 파티클 인ㅌ스턴싱을 위해서만 핑료한 뎅치터.  */
	typedef struct tagRectInstanceDesc : public CVIBuffer_Instance::INSTANCE_DESC
	{
		_float3			vPivot;
		_float2			vSpeed;
		_float2			vLifeTime;
		_bool			isLoop;
	}RECT_INSTANCE_DESC;
private:
	CVIBuffer_Rect_Instance(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVIBuffer_Rect_Instance(const CVIBuffer_Rect_Instance& Prototype);
	virtual ~CVIBuffer_Rect_Instance() = default;

public:
	virtual HRESULT Initialize_Prototype(const INSTANCE_DESC* pDesc) override;
	virtual HRESULT Initialize(void* pArg) override;

public:
	void Spread(_float fTimeDelta);
	void Drop(_float fTimeDelta);


public:
	static CVIBuffer_Rect_Instance* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const INSTANCE_DESC* pDesc);
	virtual CComponent* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END