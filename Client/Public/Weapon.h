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
	XMFLOAT3 m_alignEuler = { XMConvertToRadians(90.f), XMConvertToRadians(0.f), XMConvertToRadians(180.f) };
	// 상태별(허리/조준) 미세 보정
	XMFLOAT3 m_equipEuler = { 89.7f, 0.f, 0.f };
	XMFLOAT3 m_aimEuler = { XMConvertToRadians(160.f), XMConvertToRadians(350.f), 0.f };  // 필요에 맞게
	_float3  m_equipOffset = { 0.0f, 0.30f, 0.2f };                   // 허리에서 약간 뒤/아래
	_float3  m_aimOffset = { 0.02f, 0.07f, -0.12f };                 // 조준시 손 중심 근처

	_float4x4 m_GripLocal; // Initialize에서 Identity로 세팅됨
private:
	HRESULT Ready_Components();
	HRESULT Bind_ShaderResources();

public:
	static CWeapon* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END