#pragma once

#include "Edit_Defines.h"
#include "Base.h"

NS_BEGIN(Engine)
class CGameInstance;
NS_END

NS_BEGIN(Edit)

class CMainApp final : public CBase
{
private:
	CMainApp();
	virtual ~CMainApp() = default; 

public:
	HRESULT Initialize();
	void Update(_float fTimeDelta);
	HRESULT Render();

private:
	CGameInstance*			m_pGameInstance = { nullptr };
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };

private:
	HRESULT Ready_Prototype_ForStatic();
	HRESULT Start_Level(LEVEL eStartLevelID);

public:
	static CMainApp* Create();
	virtual void Free() override;		
};

NS_END
