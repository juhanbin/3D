#include "Shader.h"

CShader::CShader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent { pDevice, pContext }
{

}

CShader::CShader(const CShader& Prototype)
	: CComponent{ Prototype }
	, m_pEffect { Prototype.m_pEffect }
	, m_iNumPasses { Prototype.m_iNumPasses }
	, m_InputLayouts { Prototype.m_InputLayouts }
{
	Safe_AddRef(m_pEffect);

	for (auto& pInputLayout : m_InputLayouts)
		Safe_AddRef(pInputLayout);
}

HRESULT CShader::Initialize_Prototype(const _tchar* pShaderFilePath, const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements)
{	
	/*D3DCOMPILE_SKIP_VALIDATION*/

	_uint			iHlslFlag = {};

#ifdef _DEBUG	
	iHlslFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	iHlslFlag = D3DCOMPILE_OPTIMIZATION_LEVEL1;
#endif

	if (FAILED(D3DX11CompileEffectFromFile(pShaderFilePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, iHlslFlag, 0, m_pDevice, &m_pEffect, nullptr)))
		return E_FAIL;

	//이펙트 객체에서 "첫 번째 테크닉"을 얻음
	ID3DX11EffectTechnique* pTechnique = m_pEffect->GetTechniqueByIndex(0);
	if (nullptr == pTechnique)
		return E_FAIL;

	//테크닉의 속성(설명)을 얻어옴
	D3DX11_TECHNIQUE_DESC	TechniqueDesc{};
	pTechnique->GetDesc(&TechniqueDesc);

	//그 테크닉이 가진 패스 개수(Passes)를 m_iNumPasses에 저장
	m_iNumPasses = TechniqueDesc.Passes;

	for (size_t i = 0; i < m_iNumPasses; i++)
	{
		//각 패스 객체를 하나씩 꺼냄
		ID3DX11EffectPass* pPass = pTechnique->GetPassByIndex(i);
		if (nullptr == pPass)
			return E_FAIL;		

		//패스의 속성(설명)을 PassDesc에 저장
		D3DX11_PASS_DESC	PassDesc{};
		pPass->GetDesc(&PassDesc);

		// InputLayout(인풋 레이아웃)이란?
		//정점 버퍼에 들어 있는 데이터(예: POSITION, NORMAL, TEXCOORD 등)**가
		// 메모리에 어떻게 저장되어 있는지, 그리고 각각의 의미가 뭔지 GPU / 셰이더에 "정확히 설명"해주는 객체

		//Elements(엘리먼츠)란?
		//정점 1개당, 어떤 속성이 어떤 순서/위치에 저장돼 있는지 각각 "엘리먼트" 단위로 정의하는 것.
		//D3D11_INPUT_ELEMENT_DESC Elements[] = {
		//{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		ID3D11InputLayout* pInputLayout = { nullptr };

		if (FAILED(m_pDevice->CreateInputLayout(pElements, iNumElements, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pInputLayout)))
			return E_FAIL;

		m_InputLayouts.push_back(pInputLayout);
	}

	return S_OK;
}

HRESULT CShader::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT CShader::Begin(_uint iPassIndex)
{
	if (iPassIndex >= m_iNumPasses)
		return E_FAIL;

	if (FAILED(m_pEffect->GetTechniqueByIndex(0)->GetPassByIndex(iPassIndex)->Apply(0, m_pContext)))
		return E_FAIL;
	
	//패스에 맞는 InputLayout을 Input Assembler(IA)에 바인딩
	m_pContext->IASetInputLayout(m_InputLayouts[iPassIndex]);

	return S_OK;
}

//m_pShaderCom->Bind_Matrix("g_WorldMatrix", )

HRESULT CShader::Bind_Matrix(const _char* pConstantName, const _float4x4* pMatrix)
{
	//셰이더에서 pConstantName(예: "g_WorldMatrix")라는 이름의 변수(상수버퍼 멤버)를 찾음
	ID3DX11EffectVariable* pVariable = m_pEffect->GetVariableByName(pConstantName);
	if (nullptr == pVariable)
		return E_FAIL;

	//찾은 변수를 "행렬 타입"으로 캐스팅(변환)
	ID3DX11EffectMatrixVariable* pMatrixVariable = pVariable->AsMatrix();
	if (nullptr == pMatrixVariable)
		return E_FAIL;

	//실제 C++의 행렬 포인터(pMatrix, float4x4 구조체)**를 셰이더에 전달
	return pMatrixVariable->SetMatrix(reinterpret_cast<const _float*>(pMatrix));
}

HRESULT CShader::Bind_SRV(const _char* pConstantName, ID3D11ShaderResourceView* pSRV)
{
	ID3DX11EffectVariable* pVariable = m_pEffect->GetVariableByName(pConstantName);
	if (nullptr == pVariable)
		return E_FAIL;

	ID3DX11EffectShaderResourceVariable* pSRVariable = pVariable->AsShaderResource();
	if (nullptr == pSRVariable)
		return E_FAIL;

	return pSRVariable->SetResource(pSRV);
}


CShader* CShader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pShaderFilePath, const D3D11_INPUT_ELEMENT_DESC* pElements, _uint iNumElements)
{
	CShader* pInstance = new CShader(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(pShaderFilePath, pElements, iNumElements)))
	{
		MSG_BOX(TEXT("Failed to Created : CShader"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CShader::Clone(void* pArg)
{
	CShader* pInstance = new CShader(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX(TEXT("Failed to Cloned : CShader"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CShader::Free()
{
	__super::Free();

	Safe_Release(m_pEffect);

	for (auto& pInputLayout : m_InputLayouts)
		Safe_Release(pInputLayout);
	m_InputLayouts.clear();
}
