#pragma once
#include "Client_Defines.h"
#include "UIObject.h"

NS_BEGIN(Engine)
class CShader;
class CTexture;
class CVIBuffer_Rect;
NS_END

NS_BEGIN(Client)
class CBoss_Frame final : public CUIObject
{
private:
    CBoss_Frame(ID3D11Device*, ID3D11DeviceContext*);
    CBoss_Frame(const CBoss_Frame&);

public:
    static CBoss_Frame* Create(ID3D11Device*, ID3D11DeviceContext*);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float) override {}
    virtual void    Update(_float) override {}
    virtual void    Late_Update(_float) override;
    virtual HRESULT Render() override;

private:
    HRESULT Ready_Components();

private:
    Engine::CShader* m_pShaderCom = nullptr;  // HPBar_OneTex
    Engine::CVIBuffer_Rect* m_pVIBufferCom = nullptr;
    Engine::CTexture* m_pMaskTex = nullptr;  // t0: 한 장짜리 마스크

    _float4 m_FrameColor = { 0.10f, 0.10f, 0.12f, 1.0f }; // 프레임/배경 색
    float   m_EdgeSoft = 0.01f;
};
NS_END
