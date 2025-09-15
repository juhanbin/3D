#pragma once

#include "Base.h"

/* 추가하는 기능. */
/* 화면에 그려질 객체들을 그리는 순서대로 분류하여 보관한 클래스 */
/* 보관한 순서대로 객첻르의 렌더콜을 해준다. */

NS_BEGIN(Engine)

class CRenderer final : public CBase
{
private:
	CRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CRenderer() = default;

public:
	HRESULT Initialize();
	HRESULT Add_RenderGroup(RENDERGROUP eRenderGroup, class CGameObject* pRenderObject);
	HRESULT Draw();

#ifdef _DEBUG
public:
	HRESULT Add_DebugComponent(class CComponent* pComponent);

#endif

private:
	ID3D11Device* m_pDevice = { nullptr };
	ID3D11DeviceContext* m_pContext = { nullptr };
	class CGameInstance* m_pGameInstance = { nullptr };

	list<class CGameObject*>				m_RenderObjects[ENUM_CLASS(RENDERGROUP::END)];

	ID3D11BlendState* m_pAlphaBlendState = nullptr;
	ID3D11DepthStencilState* m_pUIDepthStencilState = nullptr;


private:
	class CShader* m_pShader = { nullptr };
	class CVIBuffer_Rect* m_pVIBuffer = { nullptr };

	_float4x4		m_WorldMatrix{}, m_ViewMatrix{}, m_ProjMatrix{};
	ID3D11DepthStencilView* m_pShadowDSV = { nullptr };

	_float									m_fViewportWidth{}, m_fViewportHeight{};

#ifdef _DEBUG
private:
	list<class CComponent*>					m_DebugComponent;
#endif


private:
	HRESULT Render_Priority();
	HRESULT Render_Shadow();
	HRESULT Render_NonBlend();
	HRESULT Render_Lights();
	HRESULT Render_Combined();
	HRESULT Render_NonLight();
	HRESULT Render_Blend();
	HRESULT Render_UI();
	HRESULT Render_Fade();


private:
	HRESULT Ready_Shadow_Depth_Stencil_View();
	HRESULT SetUp_Viewport(_float fWidth, _float fHeight);

#ifdef _DEBUG
	HRESULT Render_Debug();
#endif


public:
	static CRenderer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;

};

NS_END