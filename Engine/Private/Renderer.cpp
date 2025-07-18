#include "Renderer.h"
#include "GameObject.h"

CRenderer::CRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice { pDevice }
    , m_pContext { pContext }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);

}

HRESULT CRenderer::Initialize()
{
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = FALSE; // 깊이테스트 OFF
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.StencilEnable = FALSE; // 스텐실은 여기선 OFF지만 필요하면 ON으로

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = m_pDevice->CreateBlendState(&blendDesc, &m_pAlphaBlendState);
    if (FAILED(hr))
        return hr;

    HRESULT hhr = m_pDevice->CreateDepthStencilState(&dsDesc, &m_pUIDepthStencilState);
    if (FAILED(hhr))
        return hhr;

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
    if (FAILED(Render_Blend()))
        return E_FAIL;
    if (FAILED(Render_UI()))
        return E_FAIL;
    if (FAILED(Render_Fade()))
        return E_FAIL;
    return S_OK;
}

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

HRESULT CRenderer::Render_NonBlend()
{
    for (auto& pRenderObject : m_RenderObjects[ENUM_CLASS(RENDERGROUP::NONBLEND)])
    {
        if (nullptr != pRenderObject)
            pRenderObject->Render();

        Safe_Release(pRenderObject);
    }

    m_RenderObjects[ENUM_CLASS(RENDERGROUP::NONBLEND)].clear();

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

    for (size_t i = 0; i < ENUM_CLASS(RENDERGROUP::END); i++)
    {
        for (auto& pRenderObject : m_RenderObjects[i])
            Safe_Release(pRenderObject);

        m_RenderObjects[i].clear();
    }

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);

}
