#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
class CCollider;
NS_END

NS_BEGIN(Client)

class CBoss_Hand_L final : public CGameObject
{
public:
    struct DESC {
        EObjectType type;
        _float3 vScale;
        _float3 vRot;
        _float3 vPos;
    };

public:
    CBoss_Hand_L(ID3D11Device*, ID3D11DeviceContext*);
    CBoss_Hand_L(const CBoss_Hand_L& rhs);
    virtual ~CBoss_Hand_L() = default;

    static CBoss_Hand_L* Create(ID3D11Device*, ID3D11DeviceContext*);
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

    // 외부에서 변환 접근
    class CTransform* Get_Transform() const { return m_pTransformCom; }

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    // 유틸
    _float4x4 GetWorld() const;

    // 슬램 내부 틱
    void TickSlam(_float dt);

    // 벡터 w=1 보장
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

    _float3 m_slamTarget{};   // 목표 지점(플레이어 위치 스냅샷)
    _float3 m_slamStart{};    // 슬램 시작 위치(복귀용)
    _float  m_slamT = 0.f;

    // 파라미터
    _float  m_slamRiseH = 4.f;   // RISE 단계 상승 높이(초당 *2)
    _float  m_slamSpeed = 12.f;  // 목표로 이동 속도
    _float  m_recoverSpeed = 14.f;  // 원위치 복귀 속도

    _uint   m_eType = 0;
};

NS_END
