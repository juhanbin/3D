#include "MeshMaterial.h"
#include "Shader.h"

CMeshMaterial::CMeshMaterial(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CMeshMaterial::Initialize(const _char* pModelFilePath, const aiMaterial* pAIMaterial)
{
	for (size_t i = 1; i < AI_TEXTURE_TYPE_MAX; i++)
	{
		_uint		iNumTextures = pAIMaterial->GetTextureCount(static_cast<aiTextureType>(i));

		for (size_t j = 0; j < iNumTextures; j++)
		{
			ID3D11ShaderResourceView* pSRV = { nullptr };
			/* pModelFilePath : D:\Burger\153\Framework\Client\Bin\Resources\Models\Fiona\Fiona.fbx */

			/* 뽑아서 저장해뒀던 경로 + 파일이름 + 확장자 */
			aiString	strTexturePath;

			if (FAILED(pAIMaterial->GetTexture(static_cast<aiTextureType>(i), j, &strTexturePath)))
				break;

			_char			szFullPath[MAX_PATH] = {};
			_char			szDrive[MAX_PATH] = {};
			_char			szDir[MAX_PATH] = {};
			_char			szFileName[MAX_PATH] = {};
			_char			szExt[MAX_PATH] = {};

			_splitpath_s(pModelFilePath, szDrive, MAX_PATH, szDir, MAX_PATH, nullptr, 0, nullptr, 0);
			_splitpath_s(strTexturePath.data, nullptr, 0, nullptr, 0, szFileName, MAX_PATH, szExt, MAX_PATH);

			strcpy_s(szFullPath, szDrive);
			strcat_s(szFullPath, szDir);
			strcat_s(szFullPath, szFileName);
			strcat_s(szFullPath, szExt);

			_tchar			szTextureFilePath[MAX_PATH] = {};
			MultiByteToWideChar(CP_ACP, 0, szFullPath, strlen(szFullPath), szTextureFilePath, MAX_PATH);


			HRESULT		hr = {};

			if (false == strcmp(".tga", szExt))
				hr = E_FAIL;

			if (false == strcmp(".dds", szExt))
				hr = CreateDDSTextureFromFile(m_pDevice, szTextureFilePath, nullptr, &pSRV);
			else
				hr = CreateWICTextureFromFile(m_pDevice, szTextureFilePath, nullptr, &pSRV);

			if (FAILED(hr))
				return E_FAIL;

			m_SRVs[i].push_back(pSRV);
		}
	}



	return S_OK;
}

HRESULT CMeshMaterial::Initialize(const _char* pModelFilePath, const MaterialInfoBin2& bin)
{
    for (int i = 0; i < bin.numTextures; ++i)
    {
        const TextureSlotBin& slot = bin.textures[i];
        if (strlen(slot.path) == 0)
            continue;

        char szFullPath[MAX_PATH] = {};
        char szDrive[MAX_PATH] = {}, szDir[MAX_PATH] = {};
        char szFileName[MAX_PATH] = {}, szExt[MAX_PATH] = {};

        _splitpath_s(pModelFilePath, szDrive, MAX_PATH, szDir, MAX_PATH, nullptr, 0, nullptr, 0);
        _splitpath_s(slot.path, nullptr, 0, nullptr, 0, szFileName, MAX_PATH, szExt, MAX_PATH);

        strcpy_s(szFullPath, szDrive);
        strcat_s(szFullPath, szDir);
        strcat_s(szFullPath, szFileName);
        strcat_s(szFullPath, szExt);

        wchar_t szTextureFilePath[MAX_PATH] = {};
        MultiByteToWideChar(CP_ACP, 0, szFullPath, -1, szTextureFilePath, MAX_PATH);

        ID3D11ShaderResourceView* pSRV = nullptr;
        HRESULT hr = S_OK;

        if (false == strcmp(".tga", szExt))
            hr = E_FAIL;

        if (false == strcmp(".dds", szExt))
            hr = CreateDDSTextureFromFile(m_pDevice, szTextureFilePath, nullptr, &pSRV);
        else
            hr = CreateWICTextureFromFile(m_pDevice, szTextureFilePath, nullptr, &pSRV);

        if (FAILED(hr))
            return E_FAIL;

        // slot.type이 TextureType::DIFFUSE(=0), NORMAL(=1) 등. m_SRVs[slot.type]에 push
        m_SRVs[slot.type].push_back(pSRV);
    }
    return S_OK;
}



HRESULT CMeshMaterial::Bind_Resources(CShader* pShader, const _char* pConstantName, aiTextureType eTextureType, _uint iIndex)
{
	if (iIndex >= m_SRVs[eTextureType].size())
		return E_FAIL;

	return pShader->Bind_SRV(pConstantName, m_SRVs[eTextureType][iIndex]);
}

HRESULT CMeshMaterial::Bind_Resources_Bin(CShader* pShader, const _char* pConstantName, int texType, _uint iIndex)
{
	if (texType < 0 || texType >= AI_TEXTURE_TYPE_MAX)
		return E_FAIL;

	if (iIndex >= m_SRVs[texType].size())
		return E_FAIL;

	return pShader->Bind_SRV(pConstantName, m_SRVs[texType][iIndex]);
}

CMeshMaterial* CMeshMaterial::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _char* pModelFilePath, const aiMaterial* pAIMaterial)
{
	CMeshMaterial* pInstance = new CMeshMaterial(pDevice, pContext);

	if (FAILED(pInstance->Initialize(pModelFilePath, pAIMaterial)))
	{
		MSG_BOX(TEXT("Failed to Created : CMeshMaterial"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

CMeshMaterial* CMeshMaterial::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _char* pModelFilePath, const MaterialInfoBin2& bin)
{
	CMeshMaterial* pInstance = new CMeshMaterial(pDevice, pContext);
	if (FAILED(pInstance->Initialize(pModelFilePath, bin)))
	{
		MSG_BOX(TEXT("Failed to Create: CMeshMaterial (BIN)"));
		Safe_Release(pInstance);
	}
	return pInstance;
}


void CMeshMaterial::Free()
{
	__super::Free();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

	for (auto& SRVs : m_SRVs)
	{
		for (auto& pSRV : SRVs)
			Safe_Release(pSRV);
		SRVs.clear();
	}
}
