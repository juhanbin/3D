#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CPicking final : public CBase
{
	DECLARE_SINGLETON(CPicking)
public:
	CPicking();
	virtual ~CPicking() = default;

public:
	HRESULT Initialize(HWND hWnd, _uint iWinCX, _uint iWinCY, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Tick();
	void Transform_ToLocalSpace(class CTransform* pTransform);
	void Compute_LocalRayInfo(_float3* pRayDir, _float3* pRayPos, CTransform* pTransform);

public:
	const _float3& Get_RayPosW() const { return m_vRayPos; }
	const _float3& Get_RayDirW() const { return m_vRayDir; }

	_vector Get_RayPos() { return XMLoadFloat3(&m_vRayPos_Local); }
	_vector Get_RayDir() { return XMLoadFloat3(&m_vRayDir_Local); }


private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	HWND				m_hWnd;
	_uint				m_iWinCX, m_iWinCY;

	_float3 m_vRayDir = { 0,0,1 };
	_float3 m_vRayPos = { 0,0,0 };

	_float3 m_vRayDir_Local = { 0,0,1 };
	_float3 m_vRayPos_Local = { 0,0,0 };
public:
	virtual void Free() override;
};

NS_END