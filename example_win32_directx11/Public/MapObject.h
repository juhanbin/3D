#pragma once
#include "Edit_Defines.h"
#include "GameObject.h"
#include <DirectXCollision.h>
#include <vector>
#include "Model.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Edit)
class CMapObject final : public CGameObject
{
public:
    struct MAPOBJECT_DESC : public CGameObject::GAMEOBJECT_DESC
    {
        EObjectType type = EObjectType::MONSTER;
        _float3 vScale{ 1.f,1.f,1.f };
        _float3 vRot{ 0.f,0.f,0.f };
        _float3 vPos{ 0.f,0.f,0.f };
    };

private:
    CMapObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CMapObject(const CMapObject& Prototype);
    virtual ~CMapObject() = default;

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    virtual void    Priority_Update(_float fTimeDelta);
    virtual void    Update(_float fTimeDelta);
    virtual void    Late_Update(_float fTimeDelta);
    virtual HRESULT Render();

private:
    CShader* m_pShaderCom = nullptr;
    CModel* m_pModelCom = nullptr;
    EObjectType m_eType = EObjectType::MONSTER;

    // 모델 로컬 공간 AABB (모델 버텍스 기준)
    DirectX::BoundingBox m_LocalBox{};
    bool m_bSelected = false;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    // 모델 로딩 후 로컬 AABB 계산
    void InitBoundingBoxFromModel()
    {
        //if (m_pModelCom) m_pModelCom->ComputeBoundingBox(m_LocalBox);   // CModel에서 min/max로 채워주기
        //else             m_LocalBox = DirectX::BoundingBox(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(1, 1, 1));
    }

public:
    // 월드 OBB 얻기(로컬 AABB를 월드행렬로 변환)
    DirectX::BoundingOrientedBox GetWorldOBB() const
    {
        using namespace DirectX;
        BoundingOrientedBox obbLocal(
            m_LocalBox.Center, m_LocalBox.Extents, XMFLOAT4(0, 0, 0, 1));
        BoundingOrientedBox obbWorld;
        obbLocal.Transform(obbWorld, m_pTransformCom->Get_WorldMatrix());
        return obbWorld;
    }

    // 월드 레이와 교차 테스트
    bool RaycastBounds(DirectX::FXMVECTOR rayPosW, DirectX::FXMVECTOR rayDirW,
        float& outT, DirectX::XMFLOAT3& outHitW) const
    {
        using namespace DirectX;
        auto obb = GetWorldOBB();
        XMVECTOR dir = XMVector3Normalize(rayDirW);
        float t;
        if (!obb.Intersects(rayPosW, dir, t)) return false;
        XMStoreFloat3(&outHitW, rayPosW + dir * t);
        outT = t;
        return true;
    }

    // ===== 전역 레지스트리(엔진 수정 없이 현재 씬 오브젝트 열람) =====
    static const std::vector<CMapObject*>& All() { return s_All; }

public:
    static CMapObject* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void         Free() override;

private:
    static std::vector<CMapObject*> s_All;
};
NS_END
