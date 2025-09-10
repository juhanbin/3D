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

class CBoss_Fire final : public CGameObject
{
public:
    struct DESC {
        DirectX::XMFLOAT3 pos{};   // 발사 위치
        DirectX::XMFLOAT3 dir{};   // 정규화된 진행방향 (0이면 자동으로 플레이어 향함)
        float  speed = 35.f;
        float  gravity = -9.8f;    // 아래로(-)
        float  maxLife = 3.0f;     // 수명(초)
        void* owner = nullptr;  // 필요시
    };

private:
    CBoss_Fire(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    CBoss_Fire(const CBoss_Fire& rhs);
    virtual ~CBoss_Fire() = default;

public:
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float dt) override;
    virtual void    Update(_float dt) override;
    virtual void    Late_Update(_float dt) override;
    virtual HRESULT Render() override;

    // 풀 재사용 훅
    virtual void    Reuse_Begin(void* pArg) override;
    virtual void    Reuse_End() override;

private:
    void    Tick_Move(float dt);
    bool    Check_Hit();      // 필요 시 자체 히트 처리 (지금은 Player 쪽에서 처리)
    void    ReturnToPool();

    void    AlignToVelocity();
    void    FaceDir(const DirectX::XMFLOAT3& dir);
    DirectX::XMFLOAT3 GetCurrentScale() const;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

private:
    Engine::CCollider* m_pColliderCom = nullptr;
    Engine::CShader* m_pShaderCom = nullptr;
    Engine::CModel* m_pModelCom = nullptr;

    float              m_life = 0.f;
    DirectX::XMFLOAT3  m_vel{ 0,0,0 };
    DESC               m_desc{};

public:
    static CBoss_Fire* Create(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void           Free() override;
};

NS_END
