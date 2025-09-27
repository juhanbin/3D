#include "Level_Boss.h"

#include "GameInstance.h"
#include "Camera_Free.h"
#include "Camera_Player.h"
#include "Player.h"
#include "MapObject_Boss.h"

#include <fstream>
#include <functional>

#include "Object_Pool_Manager.h"
#include "Boss_Hand_L.h"
#include "Boss_Hand_R.h"
#include "Boss_Mask.h"
#include "Boss_Controller.h"
#include "Boss_Fire.h"
#include "Level_Loading.h"
using namespace Client;

CLevel_Boss::CLevel_Boss(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CLevel{ pDevice, pContext } {
}

// ===================== Initialize/Loop =====================

HRESULT CLevel_Boss::Initialize()
{
    if (FAILED(Ready_Lights()))                                   return E_FAIL;
    if (FAILED(Ready_Layer_Camera_Player(TEXT("Layer_Camera"))))         return E_FAIL;
    if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround")))) return E_FAIL;

    m_SceneObjects = LoadSceneObjects("../../Mapdata/scene_boss.bin");

    if (FAILED(Ready_Layer_Player(TEXT("Layer_Player"))))         return E_FAIL;
    if (FAILED(Ready_Layer_MapObjects(TEXT("Layer_MapObject"))))  return E_FAIL;

    // 파츠(포인터 확보: Clone → Add)
    if (FAILED(Ready_Layer_Boss_Hand_L(TEXT("Layer_Boss_Hand_L")))) return E_FAIL;
    if (FAILED(Ready_Layer_Boss_Hand_R(TEXT("Layer_Boss_Hand_R")))) return E_FAIL;
    if (FAILED(Ready_Layer_Boss_Mask(TEXT("Layer_Boss_Mask"))))     return E_FAIL;

    // 컨트롤러(파츠 포인터 전달)
    if (FAILED(Ready_Layer_Boss_Controller(TEXT("Layer_Boss"))))    return E_FAIL;

    // 이펙트/풀
    if (FAILED(Ready_Layer_Effect(TEXT("Layer_Effect"))))           return E_FAIL;
    Pool_Initialize();

    return S_OK;
}

void CLevel_Boss::Update(_float /*fTimeDelta*/)
{
    if (m_pGameInstance->Get_DIKeyState(DIK_C) & 0x80)
    {
        // 바로 Open_Level 금지!
        m_pGameInstance->Queue_Open_Level(
            static_cast<_uint>(LEVEL::LOADING),
            [dev = m_pDevice, ctx = m_pContext]() {
                return CLevel_Loading::Create(dev, ctx, LEVEL::LOGO);
            }
        );
    }
}

HRESULT CLevel_Boss::Render()
{
    SetWindowText(g_hWnd, TEXT("보스레벨입니다."));
    return S_OK;
}

// ===================== Scene loading =====================

std::vector<MapObject> CLevel_Boss::LoadSceneObjects(const char* file)
{
    std::vector<MapObject> objs;
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs.is_open())
    {
        OutputDebugStringW(L"[SCENE] scene_boss.bin 을 열 수 없습니다.\n");
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

// ===================== Pool =====================

void CLevel_Boss::Pool_Initialize()
{
    auto* pool = CObject_Pool_Manager::GetInstance();

    // Boss_Fire 풀 (보스 탄) ? 지금은 사용 안하지만 유지 가능
    pool->Register_Pool(
        LEVEL::BOSS, L"Layer_Boss_Fire", 64,
        std::bind(&CLevel_Boss::CreateBossFire_ForPool, this));

    // 플레이어 투창 풀 ? 필요시 사용
    pool->Register_Pool(
        LEVEL::BOSS, L"Layer_Spear", 64,
        std::bind(&CLevel_Boss::CreateSpear_ForPool, this));
}

Engine::CGameObject* CLevel_Boss::CreateBossFire_ForPool()
{
    // ? 올바른 프로토타입으로 복제하고, 보스 레벨의 Boss_Fire 레이어에 적재
    auto* p = static_cast<CGameObject*>(
        CGameInstance::GetInstance()->Clone_Prototype(
            PROTOTYPE::GAMEOBJECT,
            ENUM_CLASS(LEVEL::BOSS),
            TEXT("Prototype_GameObject_Boss_Fire"),
            nullptr));
    if (!p) return nullptr;

    CGameInstance::GetInstance()->Add_GameObject_ToLayer(
        ENUM_CLASS(LEVEL::BOSS), L"Layer_Boss_Fire", p);

    p->Set_Active(false);
    return p;
}

Engine::CGameObject* CLevel_Boss::CreateSpear_ForPool()
{
    auto* p = static_cast<CGameObject*>(
        CGameInstance::GetInstance()->Clone_Prototype(
            PROTOTYPE::GAMEOBJECT,
            ENUM_CLASS(LEVEL::STATIC),   // 스태틱 저장소에 프로토타입이 있다면 그대로 사용
            TEXT("Prototype_GameObject_Spear"),
            nullptr));
    if (!p) return nullptr;

    CGameInstance::GetInstance()->Add_GameObject_ToLayer(
        ENUM_CLASS(LEVEL::BOSS), L"Layer_Spear", p);

    p->Set_Active(false);
    return p;
}

// ===================== Scene parts =====================

HRESULT CLevel_Boss::Ready_Lights()
{
    LIGHT_DESC L{};
    L.eType = LIGHT_DESC::TYPE::DIRECTIONAL;
    L.vDirection = _float4(1.f, -1.f, 1.f, 0.f);
    L.vDiffuse = _float4(0.6f, 0.6f, 0.6f, 1.f);
    L.vAmbient = _float4(0.2f, 0.2f, 0.2f, 1.f);
    L.vSpecular = _float4(1.f, 1.f, 1.f, 1.f);
    if (FAILED(m_pGameInstance->Add_Light(L))) return E_FAIL;

    L.eType = LIGHT_DESC::TYPE::POINT;
    L.vPosition = _float4(20.f, 5.f, 20.f, 1.f);
    L.fRange = 10.f;
    L.vDiffuse = _float4(1.f, 0.f, 0.f, 1.f);
    L.vAmbient = _float4(0.4f, 0.1f, 0.1f, 1.f);
    L.vSpecular = L.vDiffuse;
    if (FAILED(m_pGameInstance->Add_Light(L))) return E_FAIL;

    L.eType = LIGHT_DESC::TYPE::POINT;
    L.vPosition = _float4(30.f, 5.f, 20.f, 1.f);
    L.fRange = 10.f;
    L.vDiffuse = _float4(0.f, 1.f, 0.f, 1.f);
    L.vAmbient = _float4(0.1f, 0.4f, 0.1f, 1.f);
    L.vSpecular = L.vDiffuse;
    if (FAILED(m_pGameInstance->Add_Light(L))) return E_FAIL;

    SHADOW_LIGHT_DESC S{};
    S.vEye = _float4(-20.f, 20.f, -20.f, 1.f);
    S.vAt = _float4(0.f, 0.f, 0.f, 1.f);
    S.fFovy = XMConvertToRadians(60.f);
    S.fNear = 0.1f;  S.fFar = 1000.f;
    if (FAILED(m_pGameInstance->Ready_ShadowLight(S))) return E_FAIL;

    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_Camera(const _wstring& tag)
{
    CCamera_Free::CAMERA_FREE_DESC d{};
    d.vEye = _float4(0.f, 20.f, -15.f, 1.f);
    d.vAt = _float4(0.f, 0.f, 0.f, 1.f);
    d.fFovy = XMConvertToRadians(60.0f);
    d.fNear = 0.1f; d.fFar = 500.f;
    d.fSpeedPerSec = 10.f;
    d.fRotationPerSec = XMConvertToRadians(90.0f);
    d.fMouseSensor = 0.2f;

    if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
        ENUM_CLASS(LEVEL::BOSS), tag,
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_Camera_Free"), &d)))
        return E_FAIL;

    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_Camera_Player(const _wstring& tag)
{
    CCamera_Player::CAMERA_FREE_DESC d{};
    d.vEye = _float4(0.f, 2.f, -5.f, 1.f);
    d.vAt = _float4(0.f, 1.5f, 0.f, 1.f);
    d.fFovy = XMConvertToRadians(60.0f);
    d.fNear = 0.1f; d.fFar = 500.f;
    d.fSpeedPerSec = 10.f;
    d.fRotationPerSec = XMConvertToRadians(180.0f);
    d.fMouseSensor = 0.002f;
    d.fInitDistance = 1.5f;

    if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
        ENUM_CLASS(LEVEL::BOSS), tag,
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_Camera_Player"), &d)))
        return E_FAIL;

    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_BackGround(const _wstring& tag)
{
    if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
        ENUM_CLASS(LEVEL::BOSS), tag,
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_Terrain_Boss"))))
        return E_FAIL;
    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_Player(const _wstring& tag)
{
    for (auto& obj : m_SceneObjects)
    {
        if ((EObjectType)obj.type == EObjectType::HERO)
        {
            CPlayer::HERO_DESC d{};
            d.type = static_cast<EObjectType>(obj.type);
            d.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
            d.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
            d.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

            if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
                ENUM_CLASS(LEVEL::BOSS), tag,
                ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_Player"), &d)))
            {
                OutputDebugStringW(L"[SCENE] Player Add 실패!\n");
            }
        }
    }
    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_Boss_Hand_L(const _wstring& tag)
{
    for (auto& obj : m_SceneObjects)
    {
        if ((EObjectType)obj.type == EObjectType::BOSS_HAND_L)
        {
            CBoss_Hand_L::DESC d{};
            d.type = static_cast<EObjectType>(obj.type);
            d.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
            d.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
            d.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

            auto* pObj = static_cast<CGameObject*>(
                m_pGameInstance->Clone_Prototype(PROTOTYPE::GAMEOBJECT,
                    ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_Boss_Hand_L"), &d));
            if (!pObj) { OutputDebugStringW(L"[SCENE] Boss_Hand_L Clone 실패!\n"); continue; }

            if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
                ENUM_CLASS(LEVEL::BOSS), tag, pObj)))
            {
                OutputDebugStringW(L"[SCENE] Boss_Hand_L Add 실패!\n"); Safe_Release(pObj); continue;
            }

            m_pHandL = dynamic_cast<CBoss_Hand_L*>(pObj);
        }
    }
    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_Boss_Hand_R(const _wstring& tag)
{
    for (auto& obj : m_SceneObjects)
    {
        if ((EObjectType)obj.type == EObjectType::BOSS_HAND_R)
        {
            CBoss_Hand_R::Boss_Hand_R_DESC d{};
            d.type = static_cast<EObjectType>(obj.type);
            d.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
            d.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
            d.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

            auto* pObj = static_cast<CGameObject*>(
                m_pGameInstance->Clone_Prototype(PROTOTYPE::GAMEOBJECT,
                    ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_Boss_Hand_R"), &d));
            if (!pObj) { OutputDebugStringW(L"[SCENE] Boss_Hand_R Clone 실패!\n"); continue; }

            if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
                ENUM_CLASS(LEVEL::BOSS), tag, pObj)))
            {
                OutputDebugStringW(L"[SCENE] Boss_Hand_R Add 실패!\n"); Safe_Release(pObj); continue;
            }

            m_pHandR = dynamic_cast<CBoss_Hand_R*>(pObj);
        }
    }
    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_Boss_Mask(const _wstring& tag)
{
    for (auto& obj : m_SceneObjects)
    {
        if ((EObjectType)obj.type == EObjectType::BOSS_MASK)
        {
            CBoss_Mask::Boss_Mask d{};
            d.type = static_cast<EObjectType>(obj.type);
            d.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
            d.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
            d.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

            auto* pObj = static_cast<CGameObject*>(
                m_pGameInstance->Clone_Prototype(PROTOTYPE::GAMEOBJECT,
                    ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_Boss_Mask"), &d));
            if (!pObj) { OutputDebugStringW(L"[SCENE] Boss_Mask Clone 실패!\n"); continue; }

            if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
                ENUM_CLASS(LEVEL::BOSS), tag, pObj)))
            {
                OutputDebugStringW(L"[SCENE] Boss_Mask Add 실패!\n"); Safe_Release(pObj); continue;
            }

            m_pMask = dynamic_cast<CBoss_Mask*>(pObj);
        }
    }
    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_Boss_Controller(const _wstring& tag)
{
    if (!m_pHandL || !m_pHandR || !m_pMask)
    {
        OutputDebugStringW(L"[BOSS] 파츠 포인터가 비어 컨트롤러를 생성할 수 없습니다.\n");
        return E_FAIL;
    }

    CBoss_Controller::DESC cd{};
    cd.handL = m_pHandL;
    cd.handR = m_pHandR;
    cd.mask = m_pMask;
    cd.maxHP = 3000.f;
    cd.phaseChangeHP = 0.5f;

    if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
        ENUM_CLASS(LEVEL::BOSS), tag,
        ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_BossController"), &cd)))
        return E_FAIL;

    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_MapObjects(const _wstring& tag)
{
    for (const auto& obj : m_SceneObjects)
    {
        if ((EObjectType)obj.type == EObjectType::MOD_BOSSROOM_CEILING_AA ||
            (EObjectType)obj.type == EObjectType::MOD_BOSSROOM_GROUND_AA ||
            (EObjectType)obj.type == EObjectType::MOD_BOSSROOM_GROUNDFENCE_AA ||
            (EObjectType)obj.type == EObjectType::MOD_BOSSROOM_GROUNDFENCE_AB ||
            (EObjectType)obj.type == EObjectType::MOD_BOSSROOM_PILLAR_AA ||
            (EObjectType)obj.type == EObjectType::MOD_BOSSROOM_PILLAR_AB ||
            (EObjectType)obj.type == EObjectType::MOD_BOSSROOM_WALL_AA ||
            (EObjectType)obj.type == EObjectType::MOD_BOSSROOM_WALL_AB ||
            (EObjectType)obj.type == EObjectType::MOD_BOSSROOM_WALL_AC ||
            (EObjectType)obj.type == EObjectType::MOD_GUARDRAIL_AB)
        {
            CMapObject_Boss::MAPOBJECT_DESC d{};
            d.type = static_cast<EObjectType>(obj.type);
            d.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
            d.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
            d.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

            if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
                ENUM_CLASS(LEVEL::BOSS), tag,
                ENUM_CLASS(LEVEL::BOSS), TEXT("Prototype_GameObject_MapObject_Boss"), &d)))
            {
                OutputDebugStringW(L"[SCENE] MapObject Add 실패!\n");
            }
        }
    }
    return S_OK;
}

HRESULT CLevel_Boss::Ready_Layer_Effect(const _wstring& /*tag*/)
{
    return S_OK;
}

// ===================== Create/Free =====================

CLevel_Boss* CLevel_Boss::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    auto* pInstance = new CLevel_Boss(pDevice, pContext);
    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX(TEXT("Failed to Created : CLevel_Boss"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CLevel_Boss::Free()
{
    __super::Free();
    CObject_Pool_Manager::GetInstance()->Clear_All();
}
