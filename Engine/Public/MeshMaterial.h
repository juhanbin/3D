#pragma once

#include "Base.h"
#include "BinType.h"
NS_BEGIN(Engine)

class CMeshMaterial final : public CBase
{
private:
	CMeshMaterial(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CMeshMaterial() = default;

public:
	HRESULT Initialize(const _char* pModelFilePath, const aiMaterial* pAIMaterial);
	HRESULT Initialize(const _char* pModelFilePath, const MaterialInfo& bin);
	HRESULT Bind_Resources(class CShader* pShader, const _char* pConstantName, aiTextureType eTextureType, _uint iIndex);

	ID3D11ShaderResourceView* GetDiffuseSRV() const { return m_pDiffuseSRV; }
	ID3D11ShaderResourceView* GetNormalSRV() const { return m_pNormalSRV; }

	const std::string& GetDiffuseFileName() const { return m_DiffuseFileName; }
	const std::string& GetNormalFileName() const { return m_NormalFileName; }

private:
	ID3D11Device* m_pDevice = { nullptr };
	ID3D11DeviceContext* m_pContext = { nullptr };
	vector<ID3D11ShaderResourceView*>			m_SRVs[AI_TEXTURE_TYPE_MAX];

private:
	ID3D11ShaderResourceView* m_pDiffuseSRV = nullptr;
	ID3D11ShaderResourceView* m_pNormalSRV = nullptr;

	std::string m_DiffuseFileName;
	std::string m_NormalFileName;

public:
	static CMeshMaterial* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _char* pModelFilePath, const aiMaterial* pAIMaterial);
	static CMeshMaterial* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _char* pModelFilePath, const MaterialInfo& bin);
	virtual void Free() override;
};

NS_END
