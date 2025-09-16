#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CFrustum final : public CBase
{
private:
	CFrustum();
	virtual ~CFrustum() = default;

public:
	HRESULT Initialize();
	void Update();

	_bool isIn_WorldSpace(_fvector vWorldPos);


private:
	_float4			m_vPoints[8] = {};
	_float4			m_vWorldPlanes[6] = {};

	class CGameInstance* m_pGameInstance = { nullptr };

private:
	void Make_Planes(const _float4* pPoints, _float4* pPlanes);

public:
	static CFrustum* Create();
	virtual void Free() override;
};

NS_END