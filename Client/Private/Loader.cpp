#include "Loader.h"

#include "GameInstance.h"

#include "BackGround.h"
//#include "Terrain.h"
//#include "Monster.h"
//#include "Camera.h"
//#include "Player.h"
//#include "Effect.h"
//#include "Sky.h"

CLoader::CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext { pContext }
	, m_pGameInstance { CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

unsigned int APIENTRY LoadingMain(void* pArg)
{
	CLoader* pLoader = static_cast<CLoader*>(pArg);

	if (FAILED(pLoader->Loading()))
		return 1;

	return 0;
}


HRESULT CLoader::Initialize(LEVEL eNextLevelID)
{
	m_eNextLevelID = eNextLevelID;

	InitializeCriticalSection(&m_CriticalSection);

	

	/* �����带 �����ϰ� */
	/* ������ �����尡 �ε��� �� �� �ֵ��� ó���Ѵ�. */

	/* ���� �޸𸮸� ������ ��Ÿ �ٸ� �޸𸮰���(��, ������, �ڵ�, ) �� �����尣 ���� �����Ѵ�. */
	m_hThread = (HANDLE)_beginthreadex(nullptr, 0, LoadingMain, this, 0, nullptr);
	if (0 == m_hThread)
		return E_FAIL;

	return S_OK;
}

HRESULT CLoader::Loading()
{
	EnterCriticalSection(&m_CriticalSection);

	CoInitializeEx(nullptr, 0);

	HRESULT			hr = {};

	switch(m_eNextLevelID)
	{
	case LEVEL::LOGO:
		hr = Loading_For_Logo_Level();
		break;
	case LEVEL::GAMEPLAY:
		hr = Loading_For_GamePlay_Level();
		break;
	}

	if (FAILED(hr))
		return E_FAIL;

	LeaveCriticalSection(&m_CriticalSection);

	return S_OK;
}

HRESULT CLoader::Loading_For_Logo_Level()
{

	lstrcpy(m_szLoadingText, TEXT("�ؽ��ĸ� �ε����Դϴ�."));
	/* Prototype_Component_Texture_BackGround */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_Component_Texture_BackGround"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Default%d.jpg"), 2))))
		return E_FAIL;


	lstrcpy(m_szLoadingText, TEXT("���� �ε����Դϴ�."));

	lstrcpy(m_szLoadingText, TEXT("���̴��� �ε����Դϴ�."));

	lstrcpy(m_szLoadingText, TEXT("���ӿ�����Ʈ������ �ε����Դϴ�."));

	/* Prototype_GameObject_BackGround */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_GameObject_BackGround"),
		CBackGround::Create(m_pDevice, m_pContext))))
		return E_FAIL;	

	lstrcpy(m_szLoadingText, TEXT("�ε��� �Ϸ�Ǿ����ϴ�."));
	
	m_isFinished = true;

	return S_OK;
}

HRESULT CLoader::Loading_For_GamePlay_Level()
{
	lstrcpy(m_szLoadingText, TEXT("�ؽ��ĸ� �ε����Դϴ�."));
	///* Prototype_Component_Texture_Terrain */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_Component_Texture_Terrain"),
	//	CTexture::Create(m_pDevice, m_pContext, TEXTURE::RECT, TEXT("../Bin/Resources/Textures/Terrain/Tile0.jpg"), 1))))
	//	return E_FAIL;

	///* Prototype_Component_Texture_Player */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_Component_Texture_Player"),
	//	CTexture::Create(m_pDevice, m_pContext, TEXTURE::RECT, TEXT("../Bin/Resources/Textures/Player/Player0.png"), 1))))
	//	return E_FAIL;

	///* Prototype_Component_Texture_Explosion */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_Component_Texture_Explosion"),
	//	CTexture::Create(m_pDevice, m_pContext, TEXTURE::RECT, TEXT("../Bin/Resources/Textures/Explosion/Explosion%d.png"), 90))))
	//	return E_FAIL;


	///* Prototype_Component_Texture_Sky */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_Component_Texture_Sky"),
	//	CTexture::Create(m_pDevice, m_pContext, TEXTURE::CUBE, TEXT("../Bin/Resources/Textures/SkyBox/Sky_%d.dds"), 4))))
	//	return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("���� �ε����Դϴ�."));
	///* Prototype_Component_VIBuffer_Terrain */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_Component_VIBuffer_Terrain"),
	//	CVIBuffer_Terrain::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Terrain/Height1.bmp")))))
	//	return E_FAIL;

	///* Prototype_Component_VIBuffer_Cube */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_Component_VIBuffer_Cube"),
	//	CVIBuffer_Cube::Create(m_pDevice, m_pContext))))
	//	return E_FAIL;

	//
	lstrcpy(m_szLoadingText, TEXT("���̴��� �ε����Դϴ�."));
	
	lstrcpy(m_szLoadingText, TEXT("���ӿ�����Ʈ�� �ε����Դϴ�."));

	///* Prototype_GameObject_Terrain*/
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_GameObject_Terrain"),
	//	CTerrain::Create(m_pDevice, m_pContext))))
	//	return E_FAIL;

	///* Prototype_GameObject_Camera*/
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_GameObject_Camera"),
	//	CCamera::Create(m_pDevice, m_pContext))))
	//	return E_FAIL;

	///* Prototype_GameObject_Player */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_GameObject_Player"),
	//	CPlayer::Create(m_pDevice, m_pContext))))
	//	return E_FAIL;

	///* Prototype_GameObject_Sky */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_GameObject_Sky"),
	//	CSky::Create(m_pDevice, m_pContext))))
	//	return E_FAIL;

	///* Prototype_GameObject_Monster */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_GameObject_Monster"),
	//	CMonster::Create(m_pDevice, m_pContext))))
	//	return E_FAIL;

	///* Prototype_GameObject_Effect */
	//if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LEVEL_GAMEPLAY), TEXT("Prototype_GameObject_Effect"),
	//	CEffect::Create(m_pDevice, m_pContext))))
	//	return E_FAIL;

	
	lstrcpy(m_szLoadingText, TEXT("�ε��� �Ϸ�Ǿ����ϴ�."));

	m_isFinished = true;

	return S_OK;
}
CLoader* CLoader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID)
{
	CLoader* pInstance = new CLoader(pDevice, pContext);

	if (FAILED(pInstance->Initialize(eNextLevelID)))
	{
		MSG_BOX(TEXT("Failed to Created : CLoader"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CLoader::Free()
{
	__super::Free();
	
	WaitForSingleObject(m_hThread, INFINITE);

	CloseHandle(m_hThread);

	DeleteCriticalSection(&m_CriticalSection);

	Safe_Release(m_pGameInstance);
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}
