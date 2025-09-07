#include "Loader.h"

#include "GameInstance.h"

#include "BackGround.h"
#include "Logo_Logo.h"
#include "Logo_StartButton.h"
#include "Terrain.h"
#include "Monster_Skeleton.h"
#include "MapObject.h"
#include "Camera_Free.h"
#include "Camera_Player.h"
#include "Cursor.h"
#include "Fade.h"
#include "Client_Defines.h"
#include "Player.h"
#include "Body_Player.h"
#include "Weapon.h"
#include "Player_Speare.h"
#include "Navi_Bridge.h"

#include "Body_Monster_Skeleton.h"
#include "Weapon_Skeleton_Spear.h"
#include "Weapon_Skeleton_Bow.h"
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

	

	/* 스레드를 생성하고 */
	/* 생성한 스레드가 로딩을 할 수 있도록 처리한다. */

	/* 스택 메모리를 제외한 기타 다른 메모리공간(힙, 데이터, 코드, ) 은 스레드간 서로 공유한다. */
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

	lstrcpy(m_szLoadingText, TEXT("텍스쳐를 로딩중입니다."));
	/* Prototype_Component_Texture_BackGround */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_Component_Texture_Logo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Blood_Spear/Textures/Logo_bg%d.png"), 2))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_Component_Texture_Cursor"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Blood_Spear/Textures/Cursor.png"), 1))))
		return E_FAIL;

	/*if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_Component_Texture_Logo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Blood_Spear/Textures/Logo%d.png"), 1))))
		return E_FAIL;*/

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_Component_Texture_Logo_Logo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Blood_Spear/Textures/Logo_Logo%d.png"), 2))))
		return E_FAIL;

	

	lstrcpy(m_szLoadingText, TEXT("모델을 로딩중입니다."));

	lstrcpy(m_szLoadingText, TEXT("쉐이더를 로딩중입니다."));

	lstrcpy(m_szLoadingText, TEXT("게임오브젝트원형를 로딩중입니다."));

	/* Prototype_GameObject_BackGround */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_GameObject_BackGround"),
		CBackGround::Create(m_pDevice, m_pContext))))
		return E_FAIL;	

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_GameObject_Logo"),
		CLogo_Logo::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_GameObject_StartButton"),
		CLogo_StartButton::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_GameObject_Cursor"),
		CCursor::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_GameObject_Fade"),
		CFade::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었습니다."));
	
	m_isFinished = true;

	return S_OK;
}

HRESULT CLoader::Loading_For_GamePlay_Level()
{
	lstrcpy(m_szLoadingText, TEXT("텍스쳐를 로딩중입니다."));
	/* Prototype_Component_Texture_Terrain */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Texture_Terrain"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Terrain/Tile%d.dds"), 2))))
		return E_FAIL;
	//	return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("모델을 로딩중입니다."));
	/* Prototype_Component_VIBuffer_Terrain */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_VIBuffer_Terrain"),
		CVIBuffer_Terrain::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Terrain/Height1.bmp")))))
		return E_FAIL;

	_matrix		PreTransformMatrix = XMMatrixIdentity();


	// Hero 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Hero"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::ANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Hero.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Hero 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// Rock_AA 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Rock_AA"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Rock_AA.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Rock_AA 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}
	// Bridge 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Bridge"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Bridge.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Bridge 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}
	// Spear 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Spear"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::ANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Spear.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Spear 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// Spear_Static 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.1f, 0.1f, 0.1f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Spear_Static"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Spear_Static.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Spear_Static 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}


	// Monster 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Monster_Skeleton"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::ANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Monster.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Monster 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// Monster_Spear 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Monster_Spear"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Monster_Spear.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Monster_Spear 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// Monster_Bow 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Monster_Bow"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Monster_Bow.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Monster_Bow 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	lstrcpy(m_szLoadingText, TEXT("네비게이션을 로딩중입니다."));
	/* Prototype_Component_Navigation */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Navigation"),
		CNavigation::Create(m_pDevice, m_pContext, TEXT("../../Mapdata/navmesh.nav")))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("콜라이더를 로딩중입니다."));
	/* Prototype_Component_Collider_AABB */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_AABB"),
		CCollider::Create(m_pDevice, m_pContext, COLLIDER::AABB))))
		return E_FAIL;

	/* Prototype_Component_Collider_OBB */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_OBB"),
		CCollider::Create(m_pDevice, m_pContext, COLLIDER::OBB))))
		return E_FAIL;

	/* Prototype_Component_Collider_Sphere */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Collider_Sphere"),
		CCollider::Create(m_pDevice, m_pContext, COLLIDER::SPHERE))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("쉐이더를 로딩중입니다."));
	/* Prototype_Component_Shader_VtxNorTex */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxNorTex"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxNorTex.hlsl"), VTXNORTEX::Elements, VTXNORTEX::iNumElements))))
		return E_FAIL;

	/* Prototype_Component_Shader_VtxMesh */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxMesh"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxMesh.hlsl"), VTXMESH::Elements, VTXMESH::iNumElements))))
		return E_FAIL;

	/* Prototype_Component_Shader_VtxMesh_simple */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxMesh_Simple"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxMesh_Simple.hlsl"), VTXMESH_SIMPLE::Elements, VTXMESH_SIMPLE::iNumElements))))
		return E_FAIL;

	/* Prototype_Component_Shader_VtxAnimMesh */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxAnimMesh.hlsl"), VTXANIMMESH::Elements, VTXANIMMESH::iNumElements))))
		return E_FAIL;

	/* Prototype_Component_Shader_VtxAnimMesh */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxAnimMesh_ani"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxAnimMesh_ani.hlsl"), VTXANIMMESH::Elements, VTXANIMMESH::iNumElements))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("게임오브젝트를 로딩중입니다."));

	/* Prototype_GameObject_Terrain*/
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Terrain"),
		CTerrain::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Bridge"),
		CNavi_Bridge::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Camera_Free */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Camera_Free"),
		CCamera_Free::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Camera_Player */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Camera_Player"),
		CCamera_Player::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/* Prototype_GameObject_Player */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Player"),
		CPlayer::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Body_Player */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Body_Player"),
		CBody_Player::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Weapon */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon"),
		CWeapon::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Player_Spear */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Spear"),
		CPlayer_Speare::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/* Prototype_GameObject_Player */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Skeleton"),
		CMonster_Skeleton::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Body_Player */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Body_Skeleton"),
		CBody_Monster_Skeleton::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Weapon */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Skeleton_Spear"),
		CWeapon_Skeleton_Spear::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Weapon */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Skeleton_Bow"),
		CWeapon_Skeleton_Bow::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_MapObject"),
		CMapObject::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/*if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster_Skeleton"),
		CMonster_Skeleton::Create(m_pDevice, m_pContext))))
		return E_FAIL;*/

	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었습니다."));

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
