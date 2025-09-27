#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
class CCollider;
NS_END

NS_BEGIN(Client)

class CBoss_Hand_R final : public CGameObject
{
public:
    struct Boss_Hand_R_DESC {
        EObjectType type;
        _float3 vScale;
        _float3 vRot;
        _float3 vPos;
    };

public:
    CBoss_Hand_R(ID3D11Device*, ID3D11DeviceContext*);
    CBoss_Hand_R(const CBoss_Hand_R& rhs);
    virtual ~CBoss_Hand_R() = default;

    static CBoss_Hand_R* Create(ID3D11Device*, ID3D11DeviceContext*);
    virtual CGameObject* Clone(void* pArg) override;

    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* pArg) override;
    void    Priority_Update(_float) override {}
    void    Update(_float dt) override;
    void    Late_Update(_float dt) override;
    HRESULT Render() override;

    // 패턴: 내려찍기만 사용
    void StartSlam(const _float3& targetWorld);

    // 컨트롤러에서 선회 덮어쓰지 않도록 상태 조회
    bool IsBusy() const { return m_slam != SLAM::NONE; }

    class CTransform* Get_Transform() const { return m_pTransformCom; }

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    _float4x4 GetWorld() const;

    void TickSlam(_float dt);

    static inline DirectX::XMVECTOR AsPos(const _float3& p)
    {
        return DirectX::XMVectorSet(p.x, p.y, p.z, 1.f);
    }
    static inline DirectX::XMVECTOR AsPos(DirectX::FXMVECTOR v)
    {
        return DirectX::XMVectorSet(DirectX::XMVectorGetX(v), DirectX::XMVectorGetY(v), DirectX::XMVectorGetZ(v), 1.f);
    }

private:
    class CShader* m_pShaderCom = nullptr;
    class CModel* m_pModelCom = nullptr;
    class CCollider* m_pCollider = nullptr;

    enum class SLAM { NONE, RISE, DOWN, RECOVER };
    SLAM    m_slam = SLAM::NONE;

    _float3 m_slamTarget{};
    _float3 m_slamStart{};
    _float  m_slamT = 0.f;

    _float  m_slamRiseH = 4.f;
    _float  m_slamSpeed = 12.f;
    _float  m_recoverSpeed = 14.f;

    _uint   m_eType = 0;
};

NS_END
