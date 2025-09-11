#pragma once

#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)

class CLevel_GamePlay final : public CLevel
{
private:
	CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLevel_GamePlay() = default;

public:
	virtual HRESULT Initialize() override;
	virtual void Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

private:
	void Pool_Initialize();
	Engine::CGameObject* CreateSpear_ForPool();
	Engine::CGameObject* CreateArrow_ForPool();
	Engine::CGameObject* CreateBoss_Fire_ForPool();
	HRESULT Ready_Lights();
	HRESULT Ready_Layer_Camera(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Camera_Player(const _wstring& strLayerTag);
	HRESULT Ready_Layer_BackGround(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Player(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Monster(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Mushroom(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Boss_Hand_L(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Boss_Hand_R(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Boss_Mask(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Eye(const _wstring& strLayerTag);
	HRESULT Ready_Layer_MapObjects(const _wstring& strLayerTag);
	HRESULT Ready_Layer_Effect(const _wstring& strLayerTag);

	vector<MapObject> m_SceneObjects;
	vector<MapObject> LoadSceneObjects(const char* file);
public:
	static CLevel_GamePlay* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END