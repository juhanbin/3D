#pragma once

#include "Client_Defines.h"
#include "PartObject.h"

#include "Player.h"

NS_BEGIN(Engine)
class CCollider;
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CWeapon final : public CPartObject
{
public:
	typedef struct tagWeaponDesc : public CPartObject::PARTOBJECT_DESC
	{
		const _float4x4* pSocketMatrix = { nullptr };
		const _float4x4* pSocketMatrix_Hand = { nullptr };
		_uint* pState = { nullptr };
		MOVING* pMoving = { nullptr };
		ATTACK* pAttack = { nullptr };
	}WEAPON_DESC;
private:
	CWeapon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CWeapon(const CWeapon& Prototype);
	virtual ~CWeapon() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual void Priority_Update(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

private:
	CCollider* m_pColliderCom = { nullptr };
	CShader* m_pShaderCom = { nullptr };
	CModel* m_pModelCom = { nullptr };

private:
	const _float4x4* m_pSocketMatrix = { nullptr };
	const _float4x4* m_pSocketMatrix_Hand = { nullptr };

	_uint* m_pParentState = { nullptr };
	MOVING* m_pMoving = { nullptr };
	ATTACK* m_pAttack = { nullptr };

private:
	bool      m_lastAiming = false; // 직전 프레임 조준 여부
	XMFLOAT3  m_eulerEquip = { XMConvertToRadians(0.f),  XMConvertToRadians(0.f),  XMConvertToRadians(0.f) }; // 허리 장착 각
	XMFLOAT3  m_eulerAim = { XMConvertToRadians(120.f), XMConvertToRadians(0.f),   XMConvertToRadians(0.f) }; // 조준 각

private:
	HRESULT Ready_Components();
	HRESULT Bind_ShaderResources();

public:
	static CWeapon* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END