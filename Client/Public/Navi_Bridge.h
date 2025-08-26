#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class CNavi_Bridge final : public CGameObject
{
private:
	CNavi_Bridge(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNavi_Bridge(const CNavi_Bridge& Prototype);
	virtual ~CNavi_Bridge() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual void Priority_Update(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

private:
	CNavigation* m_pNavigationCom = { nullptr };

private:
	HRESULT Ready_Components();
	HRESULT Bind_ShaderResources();

public:
	static CNavi_Bridge* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END