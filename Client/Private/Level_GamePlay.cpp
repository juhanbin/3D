#include "Level_GamePlay.h"

#include "GameInstance.h"
#include "Camera_Free.h"
#include "Monster.h"
#include <fstream>

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

	if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Player(TEXT("Layer_Player"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Monster(TEXT("Layer_Monster"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Effect(TEXT("Layer_Effect"))))
		return E_FAIL;




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

HRESULT CLevel_GamePlay::Ready_Layer_BackGround(const _wstring& strLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
		ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Terrain"))))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Player(const _wstring& strLayerTag)
{


	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Monster(const _wstring& strLayerTag)
{
        std::ifstream ifs("../../Mapdata/scene.txt");
        if (!ifs.is_open())
        {
                if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
                        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster"))))
                        return E_FAIL;
                return S_OK;
        }

        /*struct MapObject
        {
                int id; int type; float pos[3]; float size[3]; float rot[3];
        } obj{};

        while (ifs >> obj.id >> obj.type
                    >> obj.pos[0] >> obj.pos[1] >> obj.pos[2]
                    >> obj.size[0] >> obj.size[1] >> obj.size[2]
                    >> obj.rot[0] >> obj.rot[1] >> obj.rot[2])
        {
                if (obj.type != 4)
                        continue;
                CMonster::MONSTER_DESC desc{};
                desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);
                desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
                desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
                if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
                        ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster"), &desc)))
                        return E_FAIL;
        }*/
		struct MapObject
		{
			int id; int type; float pos[3]; float size[3]; float rot[3];
		} obj{};

		while (ifs >> obj.id >> obj.type
			>> obj.pos[0] >> obj.pos[1] >> obj.pos[2]
			>> obj.size[0] >> obj.size[1] >> obj.size[2]
			>> obj.rot[0] >> obj.rot[1] >> obj.rot[2])
		{
			if (obj.type != 4)
				continue;

			/*WCHAR buf[256];
			wsprintf(buf, L"[SCENE] id=%d type=%d pos=(%.2f,%.2f,%.2f) scale=(%.2f,%.2f,%.2f) rot=(%.2f,%.2f,%.2f)\n",
				obj.id, obj.type,
				obj.pos[0], obj.pos[1], obj.pos[2],
				obj.size[0], obj.size[1], obj.size[2],
				obj.rot[0], obj.rot[1], obj.rot[2]);
			OutputDebugStringW(buf);*/

			CMonster::MONSTER_DESC desc{};
			desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);
			desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
			desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);

			/*char szDbg[256];
			sprintf_s(szDbg,sizeof(szDbg), "[MON DESC] pos=(%.2f,%.2f,%.2f) scale=(%.2f,%.2f,%.2f) rot=(%.2f,%.2f,%.2f)\n",
				desc.vPos.x, desc.vPos.y, desc.vPos.z,
				desc.vScale.x, desc.vScale.y, desc.vScale.z,
				desc.vRot.x, desc.vRot.y, desc.vRot.z);
			OutputDebugStringA(szDbg);*/

			if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
				ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster"), &desc)))
				OutputDebugStringW(L"[SCENE] 몬스터 Add 실패!\n");
		}

	/*if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::GAMEPLAY), strLayerTag,
		ENUM_CLASS(LEVEL::GAMEPLAY), TEXT("Prototype_GameObject_Monster"))))
		return E_FAIL;*/


        return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Effect(const _wstring& strLayerTag)
{

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



}
