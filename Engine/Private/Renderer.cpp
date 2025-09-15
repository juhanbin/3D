#include "Renderer.h"
#include "GameObject.h"

#include "GameInstance.h"

CRenderer::CRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
    , m_pGameInstance{ CGameInstance::GetInstance() }
{
    Safe_AddRef(m_pGameInstance);
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);

}

HRESULT CRenderer::Initialize()
{
    _uint       iNumViewports = { 1 };
    D3D11_VIEWPORT      ViewportDesc{};

    m_pContext->RSGetViewports(&iNumViewports, &ViewportDesc);

    /* For.Target_Diffuse */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Diffuse"), ViewportDesc.Width, ViewportDesc.Height, DXGI_FORMAT_R8G8B8A8_UNORM, _float4(1.f, 1.f, 1.f, 0.f))))
        return E_FAIL;

    /* For.Target_Normal */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Normal"), ViewportDesc.Width, ViewportDesc.Height, DXGI_FORMAT_R16G16B16A16_UNORM, _float4(1.f, 1.f, 1.f, 1.f))))
        return E_FAIL;

    /* For.Target_Depth */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Depth"), ViewportDesc.Width, ViewportDesc.Height, DXGI_FORMAT_R32G32B32A32_FLOAT, _float4(0.f, 0.f, 0.f, 0.f))))
        return E_FAIL;

    /* For.Target_Shade */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Shade"), ViewportDesc.Width, ViewportDesc.Height, DXGI_FORMAT_R16G16B16A16_UNORM, _float4(0.f, 0.f, 0.f, 0.f))))
        return E_FAIL;

    /* For.Target_Specular */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_Specular"), ViewportDesc.Width, ViewportDesc.Height, DXGI_FORMAT_R16G16B16A16_UNORM, _float4(0.f, 0.f, 0.f, 0.f))))
        return E_FAIL;

    /* For.Target_LightDepth */
    if (FAILED(m_pGameInstance->Add_RenderTarget(TEXT("Target_LightDepth"), ViewportDesc.Width, ViewportDesc.Height, DXGI_FORMAT_R32G32B32A32_FLOAT, _float4(0.f, 0.f, 0.f, 0.f))))
        return E_FAIL;

    /* For.MRT_GameObjects : 게임 오브젝트들의 정보를 저장받기위한 타겟들 */
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObjects"), TEXT("Target_Diffuse"))))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObjects"), TEXT("Target_Normal"))))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_GameObjects"), TEXT("Target_Depth"))))
        return E_FAIL;

    /* For.MRT_LightAcc : 빛들의 연산 결과를 누적한다. */
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_LightAcc"), TEXT("Target_Shade"))))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_LightAcc"), TEXT("Target_Specular"))))
        return E_FAIL;

    /* For.MRT_Shadow : 광원기준으로 보여지는 장면을 그려준다.  */
    if (FAILED(m_pGameInstance->Add_MRT(TEXT("MRT_Shadow"), TEXT("Target_LightDepth"))))
        return E_FAIL;

    m_pVIBuffer = CVIBuffer_Rect::Create(m_pDevice, m_pContext);
    if (nullptr == m_pVIBuffer)
        return E_FAIL;

    m_pShader = CShader::Create(m_pDevice, m_pContext, TEXT("../Bin/ShaderFiles/Engine_Shader_Deferred.hlsl"), VTXPOSTEX::Elements, VTXPOSTEX::iNumElements);
    if (nullptr == m_pShader)
        return E_FAIL;

    XMStoreFloat4x4(&m_WorldMatrix, XMMatrixScaling(ViewportDesc.Width, ViewportDesc.Height, 1.f));
    XMStoreFloat4x4(&m_ViewMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&m_ProjMatrix, XMMatrixOrthographicLH(ViewportDesc.Width, ViewportDesc.Height, 0.f, 1.f));

#ifdef _DEBUG
    if (FAILED(m_pGameInstance->Ready_RT_Debug(TEXT("Target_Diffuse"), 100.0f, 100.0f, 200.f, 200.f)))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Ready_RT_Debug(TEXT("Target_Normal"), 100.0f, 300.0f, 200.f, 200.f)))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Ready_RT_Debug(TEXT("Target_Shade"), 300.0f, 100.0f, 200.f, 200.f)))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Ready_RT_Debug(TEXT("Target_Specular"), 300.0f, 300.0f, 200.f, 200.f)))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Ready_RT_Debug(TEXT("Target_LightDepth"), ViewportDesc.Width - 150.0f, 150.0f, 300.f, 300.f)))
        return E_FAIL;
#endif

    return S_OK;
}

HRESULT CRenderer::Add_RenderGroup(RENDERGROUP eRenderGroup, CGameObject* pRenderObject)
{
    if (nullptr == pRenderObject)
        return E_FAIL;

    m_RenderObjects[ENUM_CLASS(eRenderGroup)].push_back(pRenderObject);

    Safe_AddRef(pRenderObject);

    return S_OK;
}

HRESULT CRenderer::Draw()
{

    if (FAILED(Render_Priority()))
        return E_FAIL;

    if (FAILED(Render_NonBlend()))
        return E_FAIL;

    if (FAILED(Render_Lights()))
        return E_FAIL;

    if (FAILED(Render_Combined()))
        return E_FAIL;

    if (FAILED(Render_NonLight()))
        return E_FAIL;

    if (FAILED(Render_Blend()))
        return E_FAIL;

    if (FAILED(Render_UI()))
        return E_FAIL;


#ifdef _DEBUG
    if (FAILED(Render_Debug()))
        return E_FAIL;
#endif

    return S_OK;
}

#ifdef _DEBUG
HRESULT CRenderer::Add_DebugComponent(CComponent* pComponent)
{
    m_DebugComponent.push_back(pComponent);

    Safe_AddRef(pComponent);

    return S_OK;
}
#endif

HRESULT CRenderer::Render_Priority()
{
    for (auto& pRenderObject : m_RenderObjects[ENUM_CLASS(RENDERGROUP::PRIORITY)])
    {
        if (nullptr != pRenderObject)
            pRenderObject->Render();

        Safe_Release(pRenderObject);
    }

    m_RenderObjects[ENUM_CLASS(RENDERGROUP::PRIORITY)].clear();

    return S_OK;
}

HRESULT CRenderer::Render_Shadow()
{
    /* LightDepth */
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_Shadow"))))
        return E_FAIL;

    for (auto& pRenderObject : m_RenderObjects[ENUM_CLASS(RENDERGROUP::SHADOW)])
    {
        if (nullptr != pRenderObject)
            pRenderObject->Render();

        Safe_Release(pRenderObject);
    }

    m_RenderObjects[ENUM_CLASS(RENDERGROUP::SHADOW)].clear();

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_NonBlend()
{
    /* Diffuse + Normal */
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_GameObjects"))))
        return E_FAIL;

    for (auto& pRenderObject : m_RenderObjects[ENUM_CLASS(RENDERGROUP::NONBLEND)])
    {
        if (nullptr != pRenderObject)
            pRenderObject->Render();

        Safe_Release(pRenderObject);
    }

    m_RenderObjects[ENUM_CLASS(RENDERGROUP::NONBLEND)].clear();

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}



HRESULT CRenderer::Render_Lights()
{
    /* Shade */
    if (FAILED(m_pGameInstance->Begin_MRT(TEXT("MRT_LightAcc"))))
        return E_FAIL;

    /* Shade타겟을 채워줄 수 있는 리소스를 그려주자 == 쉐이드타겟의 모든 픽셀에 값을 채우낟. */
    /* Shade타겟 (wsx * wsy) */
    /* 삭가형버퍼 -> 직교투영 -> (wsx * wsy) */
    if (FAILED(m_pShader->Bind_Matrix("g_WorldMatrix", &m_WorldMatrix)))
        return E_FAIL;
    if (FAILED(m_pShader->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix)))
        return E_FAIL;
    if (FAILED(m_pShader->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix)))
        return E_FAIL;
    if (FAILED(m_pShader->Bind_Matrix("g_ViewMatrixInv", m_pGameInstance->Get_Transform_Float4x4_Inverse(D3DTS::VIEW))))
        return E_FAIL;
    if (FAILED(m_pShader->Bind_Matrix("g_ProjMatrixInv", m_pGameInstance->Get_Transform_Float4x4_Inverse(D3DTS::PROJ))))
        return E_FAIL;

    if (FAILED(m_pShader->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4))))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Normal"), m_pShader, "g_NormalTexture")))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Depth"), m_pShader, "g_DepthTexture")))
        return E_FAIL;

    m_pGameInstance->Render_Lights(m_pShader, m_pVIBuffer);

    if (FAILED(m_pGameInstance->End_MRT()))
        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::Render_Combined()
{
    /* 백버퍼 */

    if (FAILED(m_pShader->Bind_Matrix("g_WorldMatrix", &m_WorldMatrix)))
        return E_FAIL;
    if (FAILED(m_pShader->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix)))
        return E_FAIL;
    if (FAILED(m_pShader->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix)))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Diffuse"), m_pShader, "g_DiffuseTexture")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Shade"), m_pShader, "g_ShadeTexture")))
        return E_FAIL;

    if (FAILED(m_pGameInstance->Bind_RT_ShaderResource(TEXT("Target_Specular"), m_pShader, "g_SpecularTexture")))
        return E_FAIL;

    m_pShader->Begin(3);

    m_pVIBuffer->Bind_Resources();
    m_pVIBuffer->Render();

    return S_OK;
}

HRESULT CRenderer::Render_NonLight()
{
    for (auto& pRenderObject : m_RenderObjects[ENUM_CLASS(RENDERGROUP::NONLIGHT)])
    {
        if (nullptr != pRenderObject)
            pRenderObject->Render();

        Safe_Release(pRenderObject);
    }

    m_RenderObjects[ENUM_CLASS(RENDERGROUP::NONLIGHT)].clear();

    return S_OK;
}


HRESULT CRenderer::Render_Blend()
{
    /*  m_RenderObjects[ENUM_CLASS(RENDERGROUP::RG_BLEND)].sort([](CGameObject* pSour, CGameObject* pDest)->_bool
      {
          return static_cast<CBlendObject*>(pSour)->Get_Depth() > static_cast<CBlendObject*>(pDest)->Get_Depth();
      });*/

    for (auto& pRenderObject : m_RenderObjects[ENUM_CLASS(RENDERGROUP::BLEND)])
    {
        if (nullptr != pRenderObject)
            pRenderObject->Render();

        Safe_Release(pRenderObject);
    }

    m_RenderObjects[ENUM_CLASS(RENDERGROUP::BLEND)].clear();

    return S_OK;
}

HRESULT CRenderer::Render_UI()
{
    // 1. 기존 상태 저장
    ID3D11DepthStencilState* pPrevDSState = nullptr;
    UINT prevStencilRef = 0;
    m_pContext->OMGetDepthStencilState(&pPrevDSState, &prevStencilRef);

    // 2. UI용(깊이 꺼짐) 상태 적용
    m_pContext->OMSetDepthStencilState(m_pUIDepthStencilState, 0);

    float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    UINT sampleMask = 0xffffffff;

    m_pContext->OMSetBlendState(m_pAlphaBlendState, blendFactor, sampleMask);

    // --- UI 오브젝트 렌더링 ---
    for (auto& pRenderObject : m_RenderObjects[ENUM_CLASS(RENDERGROUP::UI)])
    {
        if (nullptr != pRenderObject)
            pRenderObject->Render();

        Safe_Release(pRenderObject);
    }
    m_RenderObjects[ENUM_CLASS(RENDERGROUP::UI)].clear();

    m_pContext->OMSetBlendState(nullptr, blendFactor, sampleMask);

    // 3. 원래 상태로 복원 (꼭 해야 함!)
    m_pContext->OMSetDepthStencilState(pPrevDSState, prevStencilRef);
    Safe_Release(pPrevDSState);

    return S_OK;
}

HRESULT CRenderer::Render_Fade()
{
    // 1. 기존 상태 저장
    ID3D11DepthStencilState* pPrevDSState = nullptr;
    UINT prevStencilRef = 0;
    m_pContext->OMGetDepthStencilState(&pPrevDSState, &prevStencilRef);

    // 2. UI용(깊이 꺼짐) 상태 적용
    m_pContext->OMSetDepthStencilState(m_pUIDepthStencilState, 0);

    float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    UINT sampleMask = 0xffffffff;

    m_pContext->OMSetBlendState(m_pAlphaBlendState, blendFactor, sampleMask);

    // --- UI 오브젝트 렌더링 ---
    for (auto& pRenderObject : m_RenderObjects[ENUM_CLASS(RENDERGROUP::FADE)])
    {
        if (nullptr != pRenderObject)
            pRenderObject->Render();

        Safe_Release(pRenderObject);
    }
    m_RenderObjects[ENUM_CLASS(RENDERGROUP::FADE)].clear();

    m_pContext->OMSetBlendState(nullptr, blendFactor, sampleMask);

    // 3. 원래 상태로 복원 (꼭 해야 함!)
    m_pContext->OMSetDepthStencilState(pPrevDSState, prevStencilRef);
    Safe_Release(pPrevDSState);

    return S_OK;
}

#ifdef _DEBUG

HRESULT CRenderer::Render_Debug()
{
    for (auto& pDebugCom : m_DebugComponent)
    {
        if (nullptr != pDebugCom)
            pDebugCom->Render();

        Safe_Release(pDebugCom);
    }
    m_DebugComponent.clear();


    if (FAILED(m_pShader->Bind_Matrix("g_ViewMatrix", &m_ViewMatrix)))
        return E_FAIL;

    if (FAILED(m_pShader->Bind_Matrix("g_ProjMatrix", &m_ProjMatrix)))
        return E_FAIL;

    m_pGameInstance->Render_RT_Debug(m_pShader, m_pVIBuffer);

    return S_OK;
}

#endif

CRenderer* CRenderer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CRenderer* pInstance = new CRenderer(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX(TEXT("Failed to Created : CRenderer"));
        Safe_Release(pInstance);
    }

    return pInstance;
}


void CRenderer::Free()
{
    __super::Free();

#ifdef _DEBUG
    for (auto& pDebugComponent : m_DebugComponent)
        Safe_Release(pDebugComponent);
    m_DebugComponent.clear();
#endif
    for (size_t i = 0; i < ENUM_CLASS(RENDERGROUP::END); i++)
    {
        for (auto& pRenderObject : m_RenderObjects[i])
            Safe_Release(pRenderObject);

        m_RenderObjects[i].clear();
    }

    Safe_Release(m_pShader);
    Safe_Release(m_pVIBuffer);

    Safe_Release(m_pGameInstance);
    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);

}
