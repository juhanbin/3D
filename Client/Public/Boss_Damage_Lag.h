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
class CBoss_Damage_Lag final : public CUIObject
{
public:
    struct DESC {
        std::function<float(void)> fnGetActualRatio; // 실제 HP(0~1)를 돌려주는 콜백
        float edgeSoft = 0.01f;                 // 경계 소프트
        float catchUpPerSec = 0.60f;                 // 데미지 시 초당 감소 속도
    };

private:
    CBoss_Damage_Lag(ID3D11Device*, ID3D11DeviceContext*);
    CBoss_Damage_Lag(const CBoss_Damage_Lag&);

public:
    static CBoss_Damage_Lag* Create(ID3D11Device*, ID3D11DeviceContext*);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;

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
    // 렌더 컴포넌트
    Engine::CShader* m_pShaderCom = nullptr;
    Engine::CVIBuffer_Rect* m_pVIBufferCom = nullptr;
    Engine::CTexture* m_pMaskTex = nullptr;   // 한 장만 사용(g_Tex)

    // 데이터
    std::function<float(void)> m_fnGetActual; // 실제 HP 비율(0~1)
    float m_target = 1.0f;   // 실제값(목표)
    float m_visual = 1.0f;   // 표시값(느리게 따라감)
    float m_catchPerSec = 0.60f;  // 데미지 딜레이 감소 속도
    float m_edge = 0.01f;  // 경계 소프트(픽셀 단위는 셰이더에서 처리)

    // 색상(지연바/빈칸)
    _float4 m_LagColor = _float4(0.45f, 0.10f, 0.10f, 1.0f); // 어두운 적색
    _float4 m_EmptyColor = _float4(0.08f, 0.08f, 0.08f, 1.0f); // 짙은 회색
};
NS_END
