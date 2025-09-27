#pragma once
#include "Transform.h"
#include "Shader.h"
#include "Component.h"

#include "Navigation.h"
NS_BEGIN(Engine)

class ENGINE_DLL CTransform final : public CComponent
{
public:
	typedef struct tagTransformDesc
	{
		_float		fSpeedPerSec;
		_float		fRotationPerSec;
	}TRANSFORM_DESC;

private:
	CTransform(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTransform(const CTransform& Prototype) = delete;
	virtual ~CTransform() = default;

public:
	_vector Get_State(STATE eState) const {
		return XMLoadFloat4(reinterpret_cast<const _float4*>(&m_WorldMatrix.m[ENUM_CLASS(eState)]));
	}

	_float3 Get_Scaled() const {
		return _float3(
			XMVectorGetX(XMVector3Length(Get_State(STATE::RIGHT))),
			XMVectorGetX(XMVector3Length(Get_State(STATE::UP))),
			XMVectorGetX(XMVector3Length(Get_State(STATE::LOOK)))
		);
	}

	_matrix Get_WorldMatrix() {
		return XMLoadFloat4x4(&m_WorldMatrix);
	}
	_matrix Get_WorldMatrix_Inverse() {
		return XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_WorldMatrix));
	}

	const _float4x4* Get_WorldMatrixPtr() {
		return &m_WorldMatrix;
	}
	void Set_State(STATE eState, _fvector vState) {
		XMStoreFloat4(reinterpret_cast<_float4*>(&m_WorldMatrix.m[ENUM_CLASS(eState)]), vState);
	}

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);

public:
	HRESULT Bind_Shader_Resource(class CShader* pShader, const _char* pConstantName);

public:
	void Scale(_float3 vScale);
	void Scaling(_float3 vScale);
	void Go_Straight(_float fTimeDelta);
	void Go_Straight(_float fTimeDelta, CNavigation* pNavigation);
	void Go_Straight(_float fTimeDelta, _float speedMul, CNavigation* pNavigation);
	void Go_Left(_float fTimeDelta);
	void Go_Right(_float fTimeDelta);
	void Go_Backward(_float fTimeDelta);
	void Rotation(_fvector vAxis, _float fRadian);
	void Rotation(_float fX, _float fY, _float fZ);
	void Turn(_fvector vAxis, _float fTimeDelta);
	void LookAt(_fvector vAt);
	void Chase(_fvector vTargetPos, _float fTimeDelta, _float fLimit = 0.f);

	void ScalingKeepPos(_float3 vScale);
	void RotationKeepPos(_float rx, _float ry, _float rz);
	void RotationKeepPos(_fvector axis, _float rad);

	void Set_Scaled(_float3 vScale);              // 스케일만 교체(회전/위치 유지)
	void Set_Position(_fvector p);                // 위치 세터 (벡터)
	void Set_Position(const _float3& p);          // 위치 세터 (float3)
	_float3 Get_PositionF() const;
private:
	inline void SetBasisKeepPos(_fvector r, _fvector u, _fvector l) {
		_vector pos = Get_State(STATE::POSITION);     // ★ 위치 캐시
		Set_State(STATE::RIGHT, r);
		Set_State(STATE::UP, u);
		Set_State(STATE::LOOK, l);
		Set_State(STATE::POSITION, pos);              // ★ 위치 복원
	}

public:
	void Translate(_fvector delta);

private:
	_float4x4				m_WorldMatrix = {};
	_float					m_fSpeedPerSec = {};
	_float					m_fRotationPerSec = {};
	_float3					m_vAngles = {};
public:
	static CTransform* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent* Clone(void* pArg) { return nullptr; };
	virtual void Free() override;
};

NS_END