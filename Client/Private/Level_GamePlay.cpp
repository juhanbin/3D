#include "Level_GamePlay.h"

#include "GameInstance.h"
#include "Camera_Free.h"
#include "Camera_Player.h"
#include "Monster_Skeleton.h"
#include "Player.h"
#include "MapObject.h"
#include <fstream>
#include "Object_Pool_Manager.h"
#include "Player_Speare.h"
#include "Mushroom.h"
#include "Boss_Hand_L.h"
#include "Boss_Hand_R.h"
#include "Boss_Mask.h"
#include "Parasit_Eye.h"
#include <functional>

// 공용 MapObject 구조체는 헤더에 정의
//#pragma pack(push,1)
//struct MapObject
//{
//	int id;
//	int type;
//	float size[3];
//	float rot[3];
//	float pos[3];
//	char fbxPath[260];
//	char binPath[260];
//};
//#pragma pack(pop)

//애니메이션 파일입출력. 플레이어 상태 제어. 네비게이션 기능 추가. 

CLevel_GamePlay::CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CLevel{ pDevice, pContext }
{
}

HRESULT CLevel_GamePlay::Initialize()
{
	if (FAILED(Ready_Lights()))
		return E_FAIL;

	/* 현재 레벨을 구성해주기 위한 객체들을 생성한다. */
	if (FAILED(Ready_Layer_Camera(TEXT("Layer_Camera"))))
		return E_FAIL;

	/*if (FAILED(Ready_Layer_Camera_Player(TEXT("Layer_Camera"))))
		return E_FAIL;*/

	if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
		return E_FAIL;

	m_SceneObjects = LoadSceneObjects("../../Mapdata/scene.bin");

	if (FAILED(Ready_Layer_Player(TEXT("Layer_Player"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Monster(TEXT("Layer_Monster"))))
		return E_FAIL;

  	if (FAILED(Ready_Layer_Mushroom(TEXT("Layer_Mushroom"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Boss_Hand_L(TEXT("Layer_Boss_Hand_L"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Boss_Hand_R(TEXT("Layer_Boss_Hand_R"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Boss_Mask(TEXT("Ready_Layer_Boss_Mask"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_Eye(TEXT("Ready_Layer_Eye"))))
		return E_FAIL;
	
	if (FAILED(Ready_Layer_MapObjects(TEXT("Layer_MapObject"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Effect(TEXT("Layer_Effect"))))
		return E_FAIL;

	Pool_Initialize();


	return S_OK;
}

void CLevel_GamePlay::Update(_float fTimeDelta)
{

}

HRESULT CLevel_GamePlay::Render()
{
	SetWindowText(g_hWnd, TEXT("게임플레이레벨입니다."));

	return S_OK;
}

vector<MapObject> CLevel_GamePlay::LoadSceneObjects(const char* file)
{
	std::vector<MapObject> objs;
	std::ifstream ifs(file, std::ios::binary);
	if (!ifs.is_open())
	{
		OutputDebugStringW(L"[SCENE] scene.bin을 열 수 없습니다.\n");
		return objs;
	}

	uint32_t count = 0;
	ifs.read((char*)&count, sizeof(count));

	objs.resize(count);
	for (uint32_t i = 0; i < count; ++i)
		ifs.read((char*)&objs[i], sizeof(MapObject));

	wchar_t buf[128];
	swprintf(buf, 128, L"[SCENE] 로드된 객체 수: %u\n", count);
	OutputDebugStringW(buf);

	return objs;
}

void CLevel_GamePlay::Pool_Initialize() {
	auto* pool = CObject_Pool_Manager::GetInstance();

	HRESULT hr = pool->Register_Pool(
		LEVEL::GAMEPLAY, L"Layer_Spear", 64,
		std::bind(&CLevel_GamePlay::CreateSpear_ForPool, this));
#ifdef _DEBUG
	if (FAILED(hr)) OutputDebugStringW(L"[POOL] Spear Register_Pool 실패\n");
#endif

	hr = pool->Register_Pool(
		LEVEL::GAMEPLAY, L"Layer_Arrow", 64,
		std::bind(&CLevel_GamePlay::CreateArrow_ForPool, this));
#ifdef _DEBUG
	if (FAILED(hr)) OutputDebugStringW(L"[POOL] Arrow Register_Pool 실패\n");
	else OutputDebugStringW(L"[POOL] Arrow Register_Pool OK (64)\n");
#endif
	hr = pool->Register_Pool(
		LEVEL::GAMEPLAY, L"Layer_Boss_Fire", 64,
		std::bind(&CLevel_GamePlay::CreateBoss_Fire_ForPool, this));
#ifdef _DEBUG
	if (FAILED(hr)) OutputDebugStringW(L"[POOL] Boss_Fire Register_Pool 실패\n");
	else OutputDebugStringW(L"[POOL] Boss_Fire Register_Pool OK (64)\n");
#endif
}



Engine::CGameObject* CLevel_GamePlay::CreateSpear_ForPool()
{
	CGameObject* p = static_cast<CGameObject*>(
		CGameInstance::GetInstance()->Clone_Prototype(
			PROTOTYPE::GAMEOBJECT,
			ENUM_CLASS(LEVEL::GAMEPLAY),
			TEXT("Prototype_GameObject_Spear"),
			nullptr)); // 프리워밍이라 pArg 없음
	if (!p) return nullptr;

	CGameInstance::GetInstance()
		->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), L"Layer_Spear", p);

	p->Set_Active(false);
	return p;
}

Engine::CGameObject* CLevel_GamePlay::CreateArrow_ForPool()
{
	CGameObject* p = static_cast<CGameObject*>(
		CGameInstance::GetInstance()->Clone_Prototype(
			PROTOTYPE::GAMEOBJECT,
			ENUM_CLASS(LEVEL::GAMEPLAY),
			TEXT("Prototype_GameObject_Weapon_Skeleton_Arrow"),
			nullptr));

	if (!p) {
		OutputDebugStringW(L"[POOL][Arrow] Clone_Prototype 실패! Prototype_GameObject_Arrow 미등록 가능성\n");
		return nullptr;
	}

	CGameInstance::GetInstance()->Add_GameObject_ToLayer(
		ENUM_CLASS(LEVEL::GAMEPLAY), L"Layer_Arrow", p);

	p->Set_Active(false);
	return p;
}

Engine::CGameObject* CLevel_GamePlay::CreateBoss_Fire_ForPool()
{
	CGameObject* p = static_cast<CGameObject*>(
		CGameInstance::GetInstance()->Clone_Prototype(
			PROTOTYPE::GAMEOBJECT,
			ENUM_CLASS(LEVEL::GAMEPLAY),
			TEXT("Prototype_GameObject_Boss_Fire"),
			nullptr));

	if (!p) {
		OutputDebugStringW(L"[POOL][Arrow] Prototype_GameObject_Boss_Fire 실패! Prototype_GameObject_Boss_Fire 미등록 가능성\n");
		return nullptr;
	}

	CGameInstance::GetInstance()->Add_GameObject_ToLayer(
		ENUM_CLASS(LEVEL::GAMEPLAY), L"Layer_Boss_Fire", p);

	p->Set_Active(false);
	return p;
}


HRESULT CLevel_GamePlay::Ready_Lights()
{
	LIGHT_DESC			LightDesc{};

	//(LightDesc.Diffuse * MtrlDesc.Diffuse) * (fShade(0 ~ 1) + (LightDesc.Ambient * MtrlDesc.Ambient))

	LightDesc.eType = LIGHT_DESC::TYPE::DIRECTIONAL;
	LightDesc.vDirection = _float4(1.f, -1.f, 1.f, 0.f);
	LightDesc.vDiffuse = _float4(1.f, 1.f, 1.f, 1.f);
	LightDesc.vAmbient = _float4(0.4f, 0.4f, 0.4f, 1.f);
	LightDesc.vSpecular = _float4(1.f, 1.f, 1.f, 1.f);

	if (FAILED(m_pGameInstance->Add_Light(LightDesc)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Camera(const _wstring& strLayerTag)
{
	CCamera_Free::CAMERA_FREE_DESC		CameraDesc{};

	CameraDesc.vEye = _float4(0.f, 20.f, -15.f, 1.f);
	CameraDesc.vAt = _float4(0.f, 0.f, 0.f, 1.f);
	CameraDesc.fFovy = XMConvertToRadians(60.0f);
	CameraDesc.fNear = 0.1f;
	CameraDesc.fFar = 500.f;
	CameraDesc.fSpeedPerSec = 10.f;
	CameraDesc.fRotationPerSec = XMConvertToRadians(90.0f);
	CameraDesc.fMouseSensor = 0.2f;

	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
		ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Camera_Free"), &CameraDesc)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Camera_Player(const _wstring& strLayerTag)
{
	CCamera_Player::CAMERA_FREE_DESC CameraDesc{};

	// 기본 카메라(베이스) 파라미터
	CameraDesc.vEye = _float4(0.f, 2.f, -5.f, 1.f);
	CameraDesc.vAt = _float4(0.f, 1.5f, 0.f, 1.f);
	CameraDesc.fFovy = XMConvertToRadians(60.0f);
	CameraDesc.fNear = 0.1f;
	CameraDesc.fFar = 500.f;
	CameraDesc.fSpeedPerSec = 10.f;
	CameraDesc.fRotationPerSec = XMConvertToRadians(180.0f);

	// 플레이어 카메라 전용 파라미터
	CameraDesc.fMouseSensor = 0.002f;   // 마우스 감도(라디안/픽셀)
	CameraDesc.fInitDistance = 1.5f;    // 시작 거리

	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
		ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
		ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Camera_Player"), &CameraDesc)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_BackGround(const _wstring& strLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
		ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Terrain"))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
		ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Bridge"))))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Player(const _wstring& strLayerTag)
{
	for (auto& obj : m_SceneObjects)
	{
		if ((EObjectType)obj.type == EObjectType::HERO)
		{
			CPlayer::HERO_DESC desc{};
			desc.type = static_cast<EObjectType>(obj.type);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
				ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Player"), &desc)))
			{
				OutputDebugStringW(L"[SCENE] Player Add 실패!\n");
			}
		}
	}
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Monster(const _wstring& strLayerTag)
{
	for (auto& obj : m_SceneObjects)
	{
		if ((EObjectType)obj.type == EObjectType::MONSTER || (EObjectType)obj.type == EObjectType::SKELETON_SPEAR || (EObjectType)obj.type == EObjectType::SKELETON_BOW )
		{
			CMonster_Skeleton::Monster_Skeleton_DESC desc{};
			desc.type = static_cast<EObjectType>(obj.type);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
				ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Skeleton"), &desc)))
			{
				OutputDebugStringW(L"[SCENE] Monster Add 실패!\n");
			}
		}
	}
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Mushroom(const _wstring& strLayerTag)
{
	for (auto& obj : m_SceneObjects)
	{
		if ((EObjectType)obj.type == EObjectType::MUSHROOM)
		{
			CMushroom::Mushroom_DESC desc{};
			desc.type = static_cast<EObjectType>(obj.type);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
				ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Mushroom"), &desc)))
			{
				OutputDebugStringW(L"[SCENE] Mushroom Add 실패!\n");
			}
		}
	}
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Boss_Hand_L(const _wstring& strLayerTag)
{
	for (auto& obj : m_SceneObjects)
	{
		if ((EObjectType)obj.type == EObjectType::BOSS_HAND_L)
		{
			CBoss_Hand_L::Boss_Hand_L_DESC desc{};
			desc.type = static_cast<EObjectType>(obj.type);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
				ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Boss_Hand_L"), &desc)))
			{
				OutputDebugStringW(L"[SCENE] Boss_Hand_L Add 실패!\n");
			}
		}
	}
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Boss_Hand_R(const _wstring& strLayerTag)
{
	for (auto& obj : m_SceneObjects)
	{
		if ((EObjectType)obj.type == EObjectType::BOSS_HAND_R)
		{
			CBoss_Hand_R::Boss_Hand_R_DESC desc{};
			desc.type = static_cast<EObjectType>(obj.type);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
				ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Boss_Hand_R"), &desc)))
			{
				OutputDebugStringW(L"[SCENE] Boss_Hand_R Add 실패!\n");
			}
		}
	}
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Boss_Mask(const _wstring& strLayerTag)
{
	for (auto& obj : m_SceneObjects)
	{
		if ((EObjectType)obj.type == EObjectType::BOSS_MASK)
		{
			CBoss_Mask::Boss_Mask desc{};
			desc.type = static_cast<EObjectType>(obj.type);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
				ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Boss_Mask"), &desc)))
			{
				OutputDebugStringW(L"[SCENE] Boss_Mask Add 실패!\n");
			}
		}
	}
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Eye(const _wstring& strLayerTag)
{
	for (auto& obj : m_SceneObjects)
	{
		if ((EObjectType)obj.type == EObjectType::MONSTER_EYE)
		{
			CParasit_Eye::Parasit_Eye_Desc desc{};
			desc.type = static_cast<EObjectType>(obj.type);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
				ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Eye"), &desc)))
			{
				OutputDebugStringW(L"[SCENE] Boss_Mask Add 실패!\n");
			}
		}
	}
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_MapObjects(const _wstring& strLayerTag)
{
	for (auto& obj : m_SceneObjects)
	{
		if ((EObjectType)obj.type == EObjectType::ROCK_AA)
		{
			CMapObject::MAPOBJECT_DESC desc{};
			desc.type = static_cast<EObjectType>(obj.type);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
				ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_MapObject"), &desc)))
			{
				OutputDebugStringW(L"[SCENE] MapObject Add 실패!\n");
			}
		}
		if ((EObjectType)obj.type == EObjectType::BRIDGE)
		{
			CMapObject::MAPOBJECT_DESC desc{};
			desc.type = static_cast<EObjectType>(obj.type);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
				ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_MapObject"), &desc)))
			{
				OutputDebugStringW(L"[SCENE] MapObject Add 실패!\n");
			}
		}
	}
	return S_OK;
}



HRESULT CLevel_GamePlay::Ready_Layer_Effect(const _wstring& strLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
		ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Particle"))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
		ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Snow"))))
		return E_FAIL;

	return S_OK;
}




CLevel_GamePlay* CLevel_GamePlay::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLevel_GamePlay* pInstance = new CLevel_GamePlay(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX(TEXT("Failed to Created : CLevel_GamePlay"));
		Safe_Release(pInstance);
	}

	return pInstance;
}


void CLevel_GamePlay::Free()
{
	__super::Free();

	CObject_Pool_Manager::GetInstance()->Clear_All();

}
