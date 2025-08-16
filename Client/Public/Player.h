#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class CPlayer final : public CContainerObject
{
public:
	struct HERO_DESC : public CGameObject::GAMEOBJECT_DESC
	{
		EObjectType type{ EObjectType::HERO };
		_float3 vScale{ 1.f,1.f,1.f };
		_float3 vRot{ 0.f,0.f,0.f };
		_float3 vPos{ 0.f,0.f,0.f };
	};
	enum STATE { 
		IDLE	= 0x00000001, 
		RUN		= 0x00000002,
		ATTACK	= 0x00000004,
	};
private:
	CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CPlayer(const CPlayer& Prototype);
	virtual ~CPlayer() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual void Priority_Update(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

private:
	EObjectType			m_eType = { EObjectType::HERO };
	_uint				m_iState = { };

	/*_bool				m_bAttack = {};
	_bool				m_bMoving = {};*/

private:
	HRESULT Ready_Components();	
	HRESULT Ready_PartObjects();

public:
	static CPlayer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END