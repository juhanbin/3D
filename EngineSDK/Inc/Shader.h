#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CShader final : public CComponent
{
private:
	CShader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CShader(const CShader& Prototype);
	virtual ~CShader() = default;

public:
	virtual HRESULT Initialize_Prototype(const _tchar* pShaderFilePath, const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements);
	virtual HRESULT Initialize(void* pArg);

public:
	HRESULT Begin(_uint iPassIndex);
	//셰이더 시작,

	HRESULT Bind_Matrix(const _char* pConstantName, const _float4x4* pMatrix);

private:
	ID3DX11Effect*				m_pEffect = { nullptr };

	//정점 구조체가 GPU에서 어떻게 해석돼야 할지 DirectX에 알려주는 명세서
	vector<ID3D11InputLayout*>	m_InputLayouts;

	_uint						m_iNumPasses = { };

public:
	static CShader* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pShaderFilePath, const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements);
	virtual CComponent* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END