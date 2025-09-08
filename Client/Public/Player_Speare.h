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
        DirectX::XMFLOAT3 pos{};
        DirectX::XMFLOAT3 dir{}; // normalized
        float  speed = 45.f;
        float  gravity = -9.8f;
        float  maxLife = 3.0f;
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

    // 풀 재사용 훅
    virtual void    Reuse_Begin(void* pArg) override;
    virtual void    Reuse_End() override;

private:
    void    Tick_Move(float dt);
    bool    Check_Hit();
    void    ReturnToPool();

    // 진행방향(속도)에 맞춰 월드 회전 정렬 (스케일/위치 보존)
    void    AlignToVelocity();

    // 기존 유틸(원하면 유지)
    void    FaceDir(const DirectX::XMFLOAT3& dir);

    // 현재 Transform의 스케일 길이 추출
    DirectX::XMFLOAT3 GetCurrentScale() const;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

private:
    CCollider* m_pColliderCom = nullptr;
    CShader* m_pShaderCom = nullptr;
    CModel* m_pModelCom = nullptr;

    float              m_life = 0.f;
    DirectX::XMFLOAT3  m_vel{ 0,0,0 };
    DESC               m_desc{};

public:
    static CPlayer_Speare* Create(ID3D11Device* dev, ID3D11DeviceContext* ctx);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void           Free() override;
};

NS_END
