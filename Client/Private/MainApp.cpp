#include "MainApp.h"
#include "GameInstance.h"

#include "Level_Loading.h"
#include"Player_Speare.h"

CMainApp::CMainApp()
	: m_pGameInstance{ CGameInstance::GetInstance() }
{
	//D3D11_SAMPLER_DESC

	Safe_AddRef(m_pGameInstance);
}

HRESULT CMainApp::Initialize()
{
	ENGINE_DESC		EngineDesc{};

	EngineDesc.hInst = g_hInst;
	EngineDesc.hWnd = g_hWnd;
	EngineDesc.eWinMode = WINMODE::WIN;
	EngineDesc.iWinSizeX = g_iWinSizeX;
	EngineDesc.iWinSizeY = g_iWinSizeY;
	EngineDesc.iNumLevels = ENUM_CLASS(LEVEL::END);

	if (FAILED(m_pGameInstance->Initialize_Engine(EngineDesc, &m_pDevice, &m_pContext)))
		return E_FAIL;

	if (FAILED(Ready_Gara()))
		return E_FAIL;

	if (FAILED(Ready_Prototype_ForStatic()))
		return E_FAIL;

	if (FAILED(Start_Level(LEVEL::LOGO)))
		return E_FAIL;

	return S_OK;
}

void CMainApp::Update(_float fTimeDelta)
{
	m_pGameInstance->Update_Engine(fTimeDelta);

#ifdef _DEBUG
	m_fTimeAcc += fTimeDelta;

#endif
}

HRESULT CMainApp::Render()
{
	_float4		vClearColor = _float4(0.f, 0.f, 0.f, 1.f);

	m_pGameInstance->Render_Begin(&vClearColor);

	m_pGameInstance->Draw();

#ifdef _DEBUG
	++m_iRenderCount;

	if (m_fTimeAcc >= 1.f)
	{
		wsprintf(m_szFPS, TEXT("FPS:%d"), m_iRenderCount);
		m_fTimeAcc = 0.f;
		m_iRenderCount = 0;
	}
	//m_pGameInstance->DrawText(TEXT("Font_153"), m_szFPS, _float2(100.f, 0.f), XMVectorSet(1.f, 0.f, 0.f, 1.f));

	/*m_pGameInstance->DrawText(TEXT("Font_153"), (TEXT("발견안됌")), _float2(200.f, 200.f), XMVectorSet(1.f, 0.f, 0.f, 1.f));*/
#endif

	m_pGameInstance->Render_End();

	return S_OK;
}


HRESULT CMainApp::Ready_Gara()
{
	//ID3D11Texture2D* pTexture2D = { nullptr };
	//
	//D3D11_TEXTURE2D_DESC	TextureDesc;
	//ZeroMemory(&TextureDesc, sizeof(D3D11_TEXTURE2D_DESC));
	//
	///* 깊이 버퍼의 픽셀은 백버퍼의 픽셀과 갯수가 동일해야만 깊이 텍스트가 가능해진다. */
	///* 픽셀의 수가 다르면 아에 렌더링을 못함. */
	//TextureDesc.Width = 256;
	//TextureDesc.Height = 256;
	//TextureDesc.MipLevels = 1;
	//TextureDesc.ArraySize = 1;
	//TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//
	//TextureDesc.SampleDesc.Quality = 0;
	//TextureDesc.SampleDesc.Count = 1;
	//
	//
	//TextureDesc.Usage = D3D11_USAGE_STAGING/* 정적 */;
	///* 추후에 어떤 용도로 바인딩 될 수 있는 View타입의 텍스쳐를 만들기위한 Texture2D입니까? */
	//TextureDesc.BindFlags = 0;
	//TextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	//TextureDesc.MiscFlags = 0;
	//
	//_uint* pPixels = new _uint[256 * 256];
	//
	//for (size_t i = 0; i < 256; i++)
	//{
	//	for (size_t j = 0; j < 256; j++)
	//	{
	//		_uint		iIndex = i * 256 + j;
	//
	//		/* a , b, g, r */
	//		pPixels[iIndex] = 0xff0000ff;
	//
	//	}
	//
	//}
	//
	//D3D11_SUBRESOURCE_DATA			InitialDesc{};
	//InitialDesc.pSysMem = pPixels;
	//InitialDesc.SysMemPitch = sizeof(_uint) * 256;
	//
	//if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, &InitialDesc, &pTexture2D)))
	//	return E_FAIL;
	//
	//SaveDDSTextureToFile(m_pContext, pTexture2D, TEXT("../Bin/Test.dds"));

	_ulong		dwByte = {};
	HANDLE		hFile = CreateFile(TEXT("../Bin/DataFiles/Navigation.dat"), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	_float3		vPoints[3];

	vPoints[0] = _float3(0.f, 0.f, 10.f);
	vPoints[1] = _float3(10.f, 0.f, 0.f);
	vPoints[2] = _float3(0.f, 3.f, 0.f);
	WriteFile(hFile, vPoints, sizeof(_float3) * 3, &dwByte, 0);

	vPoints[0] = _float3(0.f, 0.f, 10.f);
	vPoints[1] = _float3(10.f, 0.f, 10.f);
	vPoints[2] = _float3(10.f, 0.f, 0.f);
	WriteFile(hFile, vPoints, sizeof(_float3) * 3, &dwByte, 0);

	vPoints[0] = _float3(0.f, 0.f, 20.f);
	vPoints[1] = _float3(10.f, 0.f, 10.f);
	vPoints[2] = _float3(0.f, 0.f, 10.f);
	WriteFile(hFile, vPoints, sizeof(_float3) * 3, &dwByte, 0);

	vPoints[0] = _float3(10.f, 0.f, 10.f);
	vPoints[1] = _float3(20.f, 0.f, 0.f);
	vPoints[2] = _float3(10.f, 0.f, 0.f);
	WriteFile(hFile, vPoints, sizeof(_float3) * 3, &dwByte, 0);

	CloseHandle(hFile);


	return S_OK;
}

HRESULT CMainApp::Ready_Prototype_ForStatic()
{
	/*D3D11_INPUT_ELEMENT_DESC Elements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};*/
	if (FAILED(m_pGameInstance->Add_Font(TEXT("Font_153"), TEXT("../Bin/Resources/Fonts/153ex.SpriteFont"))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_BG"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex_BG.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxPosTex_Logo"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex_Logo.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements))))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_Fade"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxPosTex_Fade.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_VIBuffer_Rect"),
		CVIBuffer_Rect::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxMesh_Spear"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Shader_VtxMesh_Spear.hlsl"),
			VTXMESH::Elements, VTXMESH::iNumElements))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Collider_Sphere"),
		CCollider::Create(m_pDevice, m_pContext, COLLIDER::SPHERE))))
		return E_FAIL;

	_matrix Pre = XMMatrixScaling(0.01f, 0.01f, 0.01f)
		* XMMatrixRotationY(XMConvertToRadians(180.f));
	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_Component_Model_Spear_Static"),
		CModel::Create(m_pDevice, m_pContext, MODELTYPE::NONANIM, Engine::FILETYPE::BIN,
			"../../Mapdata/Spear_Static.bin", Pre))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(
		ENUM_CLASS(LEVEL::STATIC), TEXT("Prototype_GameObject_Spear"),
		CPlayer_Speare::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	return S_OK;
}

HRESULT CMainApp::Start_Level(LEVEL eStartLevelID)
{
	if (FAILED(m_pGameInstance->Open_Level(static_cast<_uint>(LEVEL::LOADING), CLevel_Loading::Create(m_pDevice, m_pContext, eStartLevelID))))
		return E_FAIL;

	return S_OK;
}

CMainApp* CMainApp::Create()
{
	CMainApp* pInstance = new CMainApp();

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX(TEXT("Failed to Created : CMainApp"));
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMainApp::Free()
{
	__super::Free();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

	m_pGameInstance->Release_Engine();

	Safe_Release(m_pGameInstance);
}