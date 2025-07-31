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

HRESULT CMeshMaterial::Initialize(const _char* pModelFilePath, const MaterialInfo& bin)
{
	const char* texPaths[3] = { bin.basecolor, bin.normal, bin.arm };
	const char* debugType[3] = { "basecolor", "normal", "arm" };

	for (int i = 0; i < 3; ++i)
	{
		if (strlen(texPaths[i]) == 0)
			continue;

		char fullPath[MAX_PATH] = {};
		char szDrive[MAX_PATH] = {}, szDir[MAX_PATH] = {};
		_splitpath_s(pModelFilePath, szDrive, MAX_PATH, szDir, MAX_PATH, nullptr, 0, nullptr, 0);
		strcpy_s(fullPath, szDrive);
		strcat_s(fullPath, szDir);
		strcat_s(fullPath, texPaths[i]);

		wchar_t wFullPath[MAX_PATH] = {};
		MultiByteToWideChar(CP_ACP, 0, fullPath, -1, wFullPath, MAX_PATH);

		const char* ext = strrchr(texPaths[i], '.');
		HRESULT hr = S_OK;
		ID3D11ShaderResourceView* pSRV = nullptr;

		if (ext && !_stricmp(ext, ".dds"))
			hr = CreateDDSTextureFromFile(m_pDevice, wFullPath, nullptr, &pSRV);
		else
			hr = CreateWICTextureFromFile(m_pDevice, wFullPath, nullptr, &pSRV);

		if (FAILED(hr)) {
			char dbg[512];
			//sprintf_s(dbg, "    [실패] 텍스처 로드 실패: %s\n", fullPath);
			//OutputDebugStringA(dbg);
			continue;
		}

		// aiTextureType으로 매핑
		aiTextureType type = aiTextureType_UNKNOWN;
		if (i == 0) type = aiTextureType_DIFFUSE;   // basecolor
		if (i == 1) type = aiTextureType_NORMALS;   // normal
		if (i == 2) type = aiTextureType_UNKNOWN;   // arm

		m_SRVs[type].push_back(pSRV);

		char dbg[512];
		//sprintf_s(dbg, "[성공] 텍스처 로드 성공: [%s] -> Slot=%d (%s)\n", fullPath, type, debugType[i]);
		//OutputDebugStringA(dbg);
	}

	return S_OK;
}


HRESULT CMeshMaterial::Bind_Resources(CShader* pShader, const _char* pConstantName, aiTextureType eTextureType, _uint iIndex)
{
    if (iIndex >= m_SRVs[eTextureType].size()) {
        char dbg[256];
        //sprintf_s(dbg, "Bind_Resources 실패: 타입=%d, 인덱스=%d, 현재 m_SRVs[%d] 크기=%llu\n", eTextureType, iIndex, eTextureType, (unsigned long long)m_SRVs[eTextureType].size());
        //OutputDebugStringA(dbg);
        return E_FAIL;
    }


    return pShader->Bind_SRV(pConstantName, m_SRVs[eTextureType][iIndex]);
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

CMeshMaterial* CMeshMaterial::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _char* pModelFilePath, const MaterialInfo& bin)
{
	CMeshMaterial* pInstance = new CMeshMaterial(pDevice, pContext);
	if (FAILED(pInstance->Initialize(pModelFilePath,bin)))
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
