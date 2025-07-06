#pragma once

#include "GameObject.h"
#include "Transform.h"


NS_BEGIN(Engine)

class ENGINE_DLL CUIObject abstract : public CGameObject
{
public:
	typedef struct tagUIObjectDesc : public CTransform::TRANSFORM_DESC
	{
		_float4 vEye = _float4(0.f, 0.f, 0.f, 1.f), vAt = _float4(0.f, 0.f, 1.f, 1.f);
		_float fNear = { 0.f }, fFar = { 1.f };

		_float fX,  fY, fSizeX, fSizeY;
	}UIOBJECT_DESC;

protected:
	CUIObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CUIObject(const CUIObject& Prototype);
	virtual ~CUIObject() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual void Priority_Update(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

protected:
	_float					m_fX{}, m_fY{}, m_fSizeX{}, m_fSizeY{};
	_float4x4				m_ViewMatrix = {};
	_float4x4				m_ProjMatrix = {};

public:
	virtual CGameObject* Clone(void* pArg) = 0;
	virtual void Free() override;
};

NS_END