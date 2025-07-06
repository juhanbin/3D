#pragma once

#include "Base.h"

/* 클라개발자가 정의한 여러 레벨클래스들의 부모가 되는 클래스 .*/

NS_BEGIN(Engine)

class ENGINE_DLL CLevel abstract : public CBase
{
protected:
	CLevel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLevel() = default;

public:
	virtual HRESULT Initialize();
	virtual void Update(_float fTimeDelta);
	virtual HRESULT Render();

protected:
	ID3D11Device* m_pDevice = { nullptr };
	ID3D11DeviceContext* m_pContext = { nullptr };
	class CGameInstance*	m_pGameInstance = { nullptr };

public:	
	virtual void Free();
};

NS_END