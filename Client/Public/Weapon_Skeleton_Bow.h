#pragma once

#include "Client_Defines.h"
#include "PartObject.h"

#include "Monster_Skeleton.h"


NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CWeapon_Skeleton_Bow final : public CPartObject
{
public:
	typedef struct tagWeaponDesc : public CPartObject::PARTOBJECT_DESC
	{
		const _float4x4* pSocketMatrix = { nullptr };
		_uint* pState = { nullptr };

	}WEAPON_DESC;
private:
	CWeapon_Skeleton_Bow(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CWeapon_Skeleton_Bow(const CWeapon_Skeleton_Bow& Prototype);
	virtual ~CWeapon_Skeleton_Bow() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual void Priority_Update(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

private:
	CShader* m_pShaderCom = { nullptr };
	CModel* m_pModelCom = { nullptr };

private:
	const _float4x4* m_pSocketMatrix = { nullptr };

	_uint* m_pParentState = { nullptr };

private:
	HRESULT Ready_Components();
	HRESULT Bind_ShaderResources();

public:
	static CWeapon_Skeleton_Bow* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END