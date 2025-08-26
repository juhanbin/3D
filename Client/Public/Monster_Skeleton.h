//#pragma once
//#include "Client_Defines.h"
//#include "ContainerObject.h"
//
//NS_BEGIN(Engine)
//class CShader;
//class CModel;
//NS_END
//
//
//NS_BEGIN(Client)
//class CMonster_Skeleton final :public CContainerObject
//{
//public:
//	struct Monster_Skeleton_DESC : public CGameObject::GAMEOBJECT_DESC
//	{
//		EObjectType type{ EObjectType::MONSTER };
//		_float3 vScale{ 1.f,1.f,1.f };
//		_float3 vRot{ 0.f,0.f,0.f };
//		_float3 vPos{ 0.f,0.f,0.f };
//	};
//
//private:
//	CMonster_Skeleton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
//	CMonster_Skeleton(const CMonster_Skeleton& Prototype);
//	virtual ~CMonster_Skeleton() = default;
//
//public:
//	virtual HRESULT Initialize_Prototype();
//	virtual HRESULT Initialize(void* pArg);
//	virtual void Priority_Update(_float fTimeDelta);
//	virtual void Update(_float fTimeDelta);
//	virtual void Late_Update(_float fTimeDelta);
//	virtual HRESULT Render();
//
//private:
//	CShader* m_pShaderCom = { nullptr };
//	CModel* m_pModelCom = { nullptr };
//
//private:
//	EObjectType m_eType = { EObjectType::MONSTER };
//	_uint				m_iState = { };
//	MOVING				m_eMoving = MOVING::IDLE;
//	_bool				m_bFinishAnim = { true };
//	_bool				m_bDashQueued = { false };
//
//	ATTACK				m_eAttack = ATTACK::NONE;
//
//	_bool				m_bshiftPressed = false;
//	_float				m_fshiftHeldSec = 0.f;
//	const float			RUN_HOLD_THRESHOLD = 0.20f;
//private:
//	HRESULT Ready_Components();
//	HRESULT Bind_ShaderResources();
//
//public:
//	static CMonster_Skeleton* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
//	virtual CGameObject* Clone(void* pArg) override;
//	virtual void Free() override;
//};
//NS_END
