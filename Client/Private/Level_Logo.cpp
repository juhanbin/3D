#include "Level_Logo.h"

#include "GameInstance.h"

#include "Level_Loading.h"

CLevel_Logo::CLevel_Logo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CLevel { pDevice, pContext }
{
}

HRESULT CLevel_Logo::Initialize()
{

	/* ���� ������ �������ֱ� ���� ��ü���� �����Ѵ�. */
	if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_Logo(TEXT("Layer_Logo"))))
		return E_FAIL;

	///////////////////////////
	if (FAILED(Ready_Layer_StartButton(TEXT("Layer_Button"))))
		return E_FAIL;

	m_pGameInstance->Register_Event(static_cast<_uint>(LEVEL::LOGO), TEXT("StartButton"),
		[this](void*) {
			TCHAR szMsg[256];

			//int someValue = 42; // ���� ����. ���� ����ϰ� ���� ������ ��ü
			//float someFloat = 3.14f; // ����
			//// �޽��� ���� (TCHAR, �����ڵ� ���)
			//_stprintf_s(szMsg, 256, TEXT("[�����] StartButton �ݹ� ȣ��! someValue = %d, someFloat = %.2f\n"), someValue, someFloat);
			//OutputDebugString(szMsg);

			_stprintf_s(szMsg, 256, TEXT("[�����] StartButton �ݹ� ȣ��!\n"));
			OutputDebugString(szMsg);
			//m_pGameInstance->Open_Level(static_cast<_uint>(LEVEL::LOADING), CLevel_Loading::Create(m_pDevice, m_pContext, LEVEL::GAMEPLAY));
			//m_pGameInstance->Clear_Event_Level(static_cast<_uint>(LEVEL::LOGO));
		});

	//////////////////////////////////
	return S_OK;
}

void CLevel_Logo::Update(_float fTimeDelta)
{
	if (GetKeyState(VK_RETURN) & 0x8000)
	{
		if (FAILED(m_pGameInstance->Open_Level(static_cast<_uint>(LEVEL::LOADING), CLevel_Loading::Create(m_pDevice, m_pContext, LEVEL::GAMEPLAY))))
			return;
	}

	return;
}

HRESULT CLevel_Logo::Render()
{
	SetWindowText(g_hWnd, TEXT("�ΰ����Դϴ�."));

	return S_OK;
}

HRESULT CLevel_Logo::Ready_Layer_BackGround(const _wstring& strLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::LOGO), strLayerTag,
		ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_GameObject_BackGround"))))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_Logo::Ready_Layer_Logo(const _wstring& strLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::LOGO), strLayerTag,
		ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_GameObject_Logo"))))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_Logo::Ready_Layer_StartButton(const _wstring& strLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject_ToLayer(ENUM_CLASS(LEVEL::LOGO), strLayerTag,
		ENUM_CLASS(LEVEL::LOGO), TEXT("Prototype_GameObject_StartButton"))))
		return E_FAIL;

	return S_OK;
}


CLevel_Logo* CLevel_Logo::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLevel_Logo* pInstance = new CLevel_Logo(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX(TEXT("Failed to Created : CLevel_Logo"));
		Safe_Release(pInstance);
	}

	return pInstance;
}


void CLevel_Logo::Free()
{
	__super::Free();



}
