#pragma once
#include "Client_Defines.h"
#include "UIObject.h"
#include <functional>

NS_BEGIN(Engine)
class CShader;
class CTexture;
class CVIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class CBoss_Fill final : public CUIObject
{
public:
    struct DESC {
        std::function<float(void)> fnGetRatio;     // 0~1 (보스 HP/Max)
        float edgeSoft = 0.002f;                   // UV 기준 작게 (예: 1024px면 1~2px 수준)
        _float4 mainColor = { 1.0f, 0.22f, 0.22f, 1.f }; // 채움색
        _float4 emptyColor = { 0.08f, 0.08f, 0.08f, 1.f }; // 빈색
    };

private:
    CBoss_Fill(ID3D11Device* d, ID3D11DeviceContext* c);
    CBoss_Fill(const CBoss_Fill& rhs);

public:
    static CBoss_Fill* Create(ID3D11Device* d, ID3D11DeviceContext* c);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void         Free() override;

    // CGameObject
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Update(_float dt) override;
    virtual void    Late_Update(_float) override;
    virtual HRESULT Render() override;

private:
    HRESULT Ready_Components();

    template<typename T>
    static inline T Clamp(T v, T lo, T hi)
    {
        return (v < lo) ? lo : (v > hi ? hi : v);
    }

    template<typename T>
    static inline T Clamp01(T v)
    {
        return Clamp<T>(v, static_cast<T>(0), static_cast<T>(1));
    }
private:
    // 컴포넌트
    Engine::CShader* m_pShaderCom = nullptr;   // Shader_VtxPosTex_Boss_HP (g_Tex/g_Mode 등)
    Engine::CVIBuffer_Rect* m_pVIBufferCom = nullptr;
    Engine::CTexture* m_pMaskTex = nullptr;   // 단일 텍스처 (지금 가진 bmp)

    // 데이터
    std::function<float(void)> m_fnGetRatio;  // 외부에서 실비율(0~1)을 주고 싶을 때
    float     m_Fill = 1.0f;                 // 최종 바인딩되는 값
    float     m_Edge = 0.002f;               // 경계 소프트
    _float4   m_MainColor = { 1.0f, 0.22f, 0.22f, 1.f };
    _float4   m_EmptyColor = { 0.08f, 0.08f, 0.08f, 1.f };
};

NS_END
