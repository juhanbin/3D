#include "Level_Edit.h"
#include "GameInstance.h"
#include "Camera_Free.h"
#include "MapObject.h"    // 단일 MapObject 클래스
#include <fstream>

CEdit::CEdit(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CLevel{ pDevice, pContext }
{
}

HRESULT CEdit::Initialize()
{
    if (FAILED(Ready_Lights()))
        return E_FAIL;

    if (FAILED(Ready_Layer_Camera(TEXT("Layer_Camera"))))
        return E_FAIL;

    // 여러 타입 오브젝트를 한꺼번에 불러오기
    //if (FAILED(Ready_Layer_MapObjects(TEXT("Layer_MapObject"))))
    //    return E_FAIL;

    return S_OK;
}

void CEdit::Update(_float fTimeDelta)
{
    
}

HRESULT CEdit::Render()
{
    SetWindowText(g_hWnd, TEXT("에디터레벨입니다."));
    return S_OK;
}

HRESULT CEdit::Ready_Lights()
{
    LIGHT_DESC LightDesc{};

    LightDesc.eType = LIGHT_DESC::TYPE::DIRECTIONAL;
    LightDesc.vDirection = _float4(1.f, -1.f, 1.f, 0.f);
    LightDesc.vDiffuse = _float4(1.f, 1.f, 1.f, 1.f);
    LightDesc.vAmbient = _float4(0.4f, 0.4f, 0.4f, 1.f);
    LightDesc.vSpecular = _float4(1.f, 1.f, 1.f, 1.f);

    if (FAILED(m_pGameInstance->Add_Light(LightDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CEdit::Ready_Layer_Camera(const _wstring& strLayerTag)
{
    CCamera_Free::CAMERA_FREE_DESC CameraDesc{};

    CameraDesc.vEye = _float4(0.f, 20.f, -15.f, 1.f);
    CameraDesc.vAt = _float4(0.f, 0.f, 0.f, 1.f);
    CameraDesc.fFovy = XMConvertToRadians(60.0f);
    CameraDesc.fNear = 0.1f;
    CameraDesc.fFar = 500.f;
    CameraDesc.fSpeedPerSec = 10.f;
    CameraDesc.fRotationPerSec = XMConvertToRadians(90.0f);
    CameraDesc.fMouseSensor = 0.2f;

    if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::EDIT), strLayerTag,
        ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_GameObject_Camera_Free"), &CameraDesc)))
        return E_FAIL;

    return S_OK;
}

HRESULT CEdit::Ready_Layer_MapObjects(const _wstring& strLayerTag)
{
    std::ifstream ifs("../../Mapdata/scene.txt");
    if (!ifs.is_open())
    {
        OutputDebugStringW(L"[SCENE] scene.txt 파일을 열 수 없습니다.\n");
        return S_OK;
    }

    struct SceneObjRaw {
        int id;
        int type; // EObjectType
        float size[3], rot[3], pos[3];
    } obj;

    while (ifs >> obj.id >> obj.type
        >> obj.size[0] >> obj.size[1] >> obj.size[2]
        >> obj.rot[0] >> obj.rot[1] >> obj.rot[2]
        >> obj.pos[0] >> obj.pos[1] >> obj.pos[2])
    {
        CMapObject::MAPOBJECT_DESC desc{};
        desc.type = static_cast<EObjectType>(obj.type);
        desc.vScale = _float3(obj.size[0], obj.size[1], obj.size[2]);
        desc.vRot = _float3(obj.rot[0], obj.rot[1], obj.rot[2]);
        desc.vPos = _float3(obj.pos[0], obj.pos[1], obj.pos[2]);

        if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(
            ENUM_CLASS(LEVEL::EDIT), strLayerTag,
            ENUM_CLASS(LEVEL::EDIT), TEXT("Prototype_GameObject_MapObject"), &desc)))
        {
            OutputDebugStringW(L"[SCENE] MapObject Add 실패!\n");
        }
    }
    return S_OK;
}

CEdit* CEdit::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CEdit* pInstance = new CEdit(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX(TEXT("Failed to Created : CEdit"));
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CEdit::Free()
{
    __super::Free();
}
