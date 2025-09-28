#pragma once

#include "Client_Defines.h"
#include "UIObject.h"

NS_BEGIN(Engine)
class CShader;
class CTexture;
class CVIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class CPlayer_Hp final : public CUIObject
{
private:
    CPlayer_Hp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CPlayer_Hp(const CPlayer_Hp& Prototype);
    virtual ~CPlayer_Hp() = default;

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    virtual void   Priority_Update(_float fTimeDelta);
    virtual void   Update(_float fTimeDelta);       // ← PM에서 hp 읽어와 보간만 수행
    virtual void   Late_Update(_float fTimeDelta);
    virtual HRESULT Render();                       // ← 렌더만

private:
    CShader* m_pShaderCom = nullptr;
    CTexture* m_pTextureCom = nullptr;      // 마스크로 쓰는 텍스처
    CVIBuffer_Rect* m_pVIBufferCom = nullptr;

private:
    HRESULT Ready_Components();

    // HP → 채움비율 (0~1)
    float m_TargetFill = 1.f;   // PM에서 바로 계산된 목표값
    float m_VisualFill = 1.f;   // 화면 보간용

    // 셰이더 파라미터(필요 시 노출해서 튜닝)
    float   m_EdgeSoft = 0.8f;
    _float4 m_FillL = { 1.00f, 0.40f, 0.20f, 1.f };
    _float4 m_FillR = { 1.00f, 0.10f, 0.10f, 1.f };
    _float4 m_EmptyL = { 0.18f, 0.18f, 0.18f, 1.f };
    _float4 m_EmptyR = { 0.05f, 0.05f, 0.05f, 1.f };

    template <typename T>
    inline T Clamp01(T v) const { return (v < (T)0) ? (T)0 : (v > (T)1 ? (T)1 : v); }

public:
    static CPlayer_Hp* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END
