#pragma once

#include "Client_Defines.h"
#include "UIObject.h"

NS_BEGIN(Engine)
class CShader;
class CTexture;
class CVIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class CPlayer_Hp_Eye final : public CUIObject
{
private:
    CPlayer_Hp_Eye(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CPlayer_Hp_Eye(const CPlayer_Hp_Eye& Prototype);
    virtual ~CPlayer_Hp_Eye() = default;

public:
    // CGameObject
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

private:
    // Components
    CShader* m_pShaderCom = nullptr;
    CTexture* m_pTextureCom = nullptr;
    CVIBuffer_Rect* m_pVIBufferCom = nullptr;

private:
    HRESULT Ready_Components();

private:
    float   m_Time = 0.f;
    int     m_Cols = 8;   // 512/64 = 8
    int     m_Rows = 8;
    float   m_FPS = 12.f; // 원하는 재생 속도
    int     m_Start = 0;    // 시작 프레임 index
    int     m_Count = 64;   // 사용할 프레임 수

    _float2 m_UVScale = _float2(1.f, 1.f);
    _float2 m_UVOffset = _float2(0.f, 0.f);

public:
    static CPlayer_Hp_Eye* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void           Free() override;
};

NS_END
