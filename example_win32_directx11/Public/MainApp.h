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
	// --- MapTool 관련 함수 ---
	void Render_ImGuiPanel();
	void SaveScene(const char* filename);
	bool LoadScene(const char* filename);
	void PushUndo();
	void RefreshScene();

private:
	// --- MapTool 데이터 ---
	vector<MapObject> m_Objects;
	vector<vector<MapObject>> m_UndoStack;
	int m_Selected = -1;
	float m_TempSize[3] = { 1,1,1 };
	float m_TempRot[3] = { 0,0,0 };
	float m_TempPos[3] = { 0,0,0 };

	ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
	ID3D11DepthStencilView* m_pDepthStencilView = nullptr;
public:
	static CMainApp* Create();
	virtual void Free() override;		
};

NS_END
