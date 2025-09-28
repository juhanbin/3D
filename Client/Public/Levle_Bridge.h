#pragma once

#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)

class CLevle_Bridge final : public CLevel
{
private:
	CLevle_Bridge(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLevle_Bridge() = default;

public:
	virtual HRESULT Initialize() override;
	virtual void Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;
	
private:
	void Pool_Initialize();
	Engine::CGameObject* CreateSpear_ForPool();
	Engine::CGameObject* CreateArrow_ForPool();
	HRESULT Ready_Lights();
	HRESULT Ready_Layer_Camera(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Camera_Player(const _wstring& strLayerTag);
	HRESULT Ready_Layer_BackGround(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Player(const _wstring& strLayerTag);
	HRESULT Ready_Layer_MapObjects(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Monster(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Effect(const _wstring& strLayerTag);
	HRESULT Ready_Layer_UI(const _wstring& strLayerTag);

	vector<MapObject> m_SceneObjects;
	vector<MapObject> LoadSceneObjects(const char* file);
public:
	static CLevle_Bridge* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END