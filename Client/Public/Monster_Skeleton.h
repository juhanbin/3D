#pragma once
#include "Client_Defines.h"
#include "ContainerObject.h"
#include "Transform.h"

NS_BEGIN(Engine)
class CCollider;
class CNavigation;
NS_END


NS_BEGIN(Client)
class CWeapon_Skeleton_Spear;
class CBody_Monster_Skeleton;

class CMonster_Skeleton final :public CContainerObject
{
public:
	struct Monster_Skeleton_DESC
	{
		EObjectType type{ EObjectType::MONSTER };
		_float3 vScale{ 1.f,1.f,1.f };
		_float3 vRot{ 0.f,0.f,0.f };
		_float3 vPos{ 0.f,0.f,0.f };
	};

private:
	CMonster_Skeleton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Skeleton(const CMonster_Skeleton& Prototype);
	virtual ~CMonster_Skeleton() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual void Priority_Update(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

private:
	CNavigation* m_pNavigationCom = nullptr;
	CCollider* m_pColliderCom[ENUM_CLASS(COLLIDER::END)] = { nullptr };
private:
	MONSTER   m_iState = MONSTER::SPEARE_IDLE;
	EObjectType m_eType = { EObjectType::MONSTER };

	_bool		m_bisHit = false;
	_float		m_fHitTimer = 0.f;
	template<typename T>
	static inline T clamp_compat(T v, T lo, T hi) {
		return (v < lo) ? lo : (v > hi) ? hi : v;
	}
private:
	HRESULT Ready_Components();
	HRESULT Ready_PartObjects();
	_bool Collision_ToPlayer();

public:
	static CMonster_Skeleton* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};
NS_END
