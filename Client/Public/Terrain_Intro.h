#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CShader;
class CTexture;
class CNavigation;
class CVIBuffer_Terrain;
NS_END

NS_BEGIN(Client)

class CTerrain_Intro final : public CGameObject
{
public:
	enum TEXTURE { TEXTURE_DIFFUSE, TEXTURE_MASK, TEXTURE_BRUSH, TEXTURE_END };
private:
	CTerrain_Intro(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTerrain_Intro(const CTerrain_Intro& Prototype);
	virtual ~CTerrain_Intro() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual void Priority_Update(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

private:
	CShader* m_pShaderCom = { nullptr };
	CTexture* m_pTextureCom[TEXTURE_END] = { nullptr };
	CNavigation* m_pNavigationCom = { nullptr };
	CVIBuffer_Terrain* m_pVIBufferCom = { nullptr };

private:
	HRESULT Ready_Components();
	HRESULT Bind_ShaderResources();

public:
	static CTerrain_Intro* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END