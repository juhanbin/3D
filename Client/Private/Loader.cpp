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
#include "Weapon_Skeleton_Arrow.h"

#include "Mushroom.h"
#include "Boss_Hand_L.h"
#include "Boss_Hand_R.h"
#include "Boss_Mask.h"
#include "Boss_Fire.h"

#include "Parasit_Eye.h"
#include "Particle.h"
#include "Snow.h"

#include "Explosion.h"
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

	/* Prototype_Component_Texture_Mask_Terrain */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Texture_Mask_Terrain"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Terrain/TerrainMask.dds"), 1))))
		return E_FAIL;

	/* Prototype_Component_Texture_Brush */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Texture_Brush"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Terrain/Brush.png"), 1))))
		return E_FAIL;

	/* Prototype_Component_Texture_Snow */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Texture_Snow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Snow/Snow.png"), 1))))
		return E_FAIL;

	/* Prototype_Component_Texture_Explosion */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Texture_Explosion"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../Bin/Resources/Textures/Explosion/Explosion%d.png"), 90))))
		return E_FAIL;

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
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	
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

	// Monster_Arrow 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Monster_Arrow"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Monster_Arrow.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Monster_Arrow 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// Monster_Mushroom 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Mushroom"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::ANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Mushroom.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Monster_Mushroom 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// Boss_hand_L 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Boss_hand_L"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::ANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Boss_hand_L.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Boss_hand_L 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// Boss_hand_R 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Boss_hand_R"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::ANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Boss_hand_R.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Boss_hand_R 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// Boss_Mask 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Boss_Mask"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Boss_Mask.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Boss_Mask 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// Boss_Fire 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Boss_Fire"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Boss_Eye_Mid.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Boss_Fire 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	// eye 프로토타입 등록
	PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(180.0f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_Model_Eye"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Eye.bin", PreTransformMatrix))))
	{
		OutputDebugStringA("[LOADER] Eye 모델 프로토타입 등록 실패!\n");
		return E_FAIL;
	}

	/* Prototype_Component_Particle_Explosion */
	CVIBuffer_Rect_Instance::RECT_INSTANCE_DESC		ExploDesc{};
	ExploDesc.iNumInstance = 300;
	ExploDesc.vCenter = _float3(-9.6f, 5.f, 20.f);
	ExploDesc.vRange = _float3(0.2f, 0.2f, 0.2f);
	ExploDesc.vSize = _float2(0.05f, 0.1f);
	ExploDesc.vLifeTime = _float2(0.5f, 2.f);
	ExploDesc.vPivot = _float3(-9.6f, 5.f, 20.f);
	ExploDesc.vSpeed = _float2(0.5f, 1.f);
	ExploDesc.isLoop = true;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Particle_Explosion"),
		CVIBuffer_Rect_Instance::Create(m_pDevice, m_pContext, &ExploDesc))))
		return E_FAIL;

	/* Prototype_Component_Particle_Snow */
	CVIBuffer_Point_Instance::POINT_INSTANCE_DESC		SnowDesc{};
	SnowDesc.iNumInstance = 3000;
	SnowDesc.vCenter = _float3(64.f, 20.f, 64.f);
	SnowDesc.vRange = _float3(128.f, 1.f, 128.f);
	SnowDesc.vSize = _float2(0.1f, 0.2f);
	SnowDesc.vLifeTime = _float2(5.0f, 10.f);
	SnowDesc.vPivot = _float3(0.f, 0.f, 0.f);
	SnowDesc.vSpeed = _float2(1.5f, 3.f);
	SnowDesc.isLoop = true;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Particle_Snow"),
		CVIBuffer_Point_Instance::Create(m_pDevice, m_pContext, &SnowDesc))))
		return E_FAIL;

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

	/* Prototype_Component_Shader_VtxAnimMesh */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxAnimMesh.hlsl"), VTXANIMMESH::Elements, VTXANIMMESH::iNumElements))))
		return E_FAIL;

	/* Prototype_Component_Shader_VtxAnimMesh */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxAnimMesh_ani"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxAnimMesh_ani.hlsl"), VTXANIMMESH::Elements, VTXANIMMESH::iNumElements))))
		return E_FAIL;

	/* Prototype_Component_Shader_VtxInstance_Particle */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxInstance_Particle"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxInstance_Particle.hlsl"), VTXPARTICLE::Elements, VTXPARTICLE::iNumElements))))
		return E_FAIL;

	/* Prototype_Component_Shader_VtxInstance_PointParticle*/
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_Component_Shader_VtxInstance_PointParticle"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxInstance_PointParticle.hlsl"), VTXPOINTPARTICLE::Elements, VTXPOINTPARTICLE::iNumElements))))
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

	/* Prototype_GameObject_Weapon_Arrow */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Weapon_Skeleton_Arrow"),
		CWeapon_Skeleton_Arrow::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Mushroom */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Mushroom"),
		CMushroom::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Boss_Hand_L */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Boss_Hand_L"),
		CBoss_Hand_L::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Boss_Hand_R */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Boss_Hand_R"),
		CBoss_Hand_R::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Boss_Mask */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Boss_Mask"),
		CBoss_Mask::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Boss_Fire"),
		CBoss_Fire::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Eye"),
		CParasit_Eye::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_MapObject"),
		CMapObject::Create(m_pDevice, m_pContext))))
		return E_FAIL;


	///* Prototype_GameObject_Particle */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Particle"),
		CParticle::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	///* Prototype_GameObject_Snow */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Snow"),
		CSnow::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* Prototype_GameObject_Effect_Explosion */
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Effect_Explosion"),
		CExplosion::Create(m_pDevice, m_pContext))))
		return E_FAIL;

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
