#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
#include <DirectXMath.h>

NS_BEGIN(Engine)
class CCollider;
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CPlayer_Speare final : public CGameObject
{
public:
    struct DESC {
        XMFLOAT3 pos;
        XMFLOAT3 dir;     // normalized
        float speed = 45.f;
        float gravity = -9.8f;
        float maxLife = 3.0f;
        void* owner = nullptr;
    };

private:
    CPlayer_Speare(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    CPlayer_Speare(const CPlayer_Speare& rhs);
    virtual ~CPlayer_Speare() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float dt) override;
    virtual void    Update(_float dt) override;
    virtual void    Late_Update(_float dt) override;
    virtual HRESULT Render() override;

    // Ç® Àç»ç¿ë ÈÅ
    virtual void    Reuse_Begin(void* pArg) override;
    virtual void    Reuse_End() override;

    void FaceDir(const XMFLOAT3& dir);
private:
    void Tick_Move(float dt);
    bool Check_Hit();
    void ReturnToPool();

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

private:
    CCollider* m_pColliderCom = { nullptr };
    CShader* m_pShaderCom = { nullptr };
    CModel* m_pModelCom = { nullptr };

private:
    float             m_life = 0.f;
    XMFLOAT3            m_vel{ 0,0,0 };
    DESC              m_desc{};

public:
    static CPlayer_Speare* Create(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    virtual CGameObject* Clone(void* pArg) override; 
    virtual void Free() override; 
};

NS_END
