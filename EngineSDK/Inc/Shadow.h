#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CShadow final : public CBase
{
private:
	CShadow();
	virtual ~CShadow() = default;

public:
	const _float4x4* Get_Transform_Float4x4(D3DTS eTransformState) const;

public:
	HRESULT Initialize(_uint iWinSizeX, _uint iWinSizeY);
	HRESULT Ready_ShadowLight(SHADOW_LIGHT_DESC LightDesc);

private:
	_float				m_fViewportWidth{}, m_fViewportHeight{};
	_float4x4			m_Matrices[ENUM_CLASS(D3DTS::END)];

public:
	static CShadow* Create(_uint iWinSizeX, _uint iWinSizeY);
	virtual void Free();
};

NS_END