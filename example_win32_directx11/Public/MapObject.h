#pragma once
#include "Edit_Defines.h"
#include "GameObject.h"
#include <DirectXCollision.h> // BoundingBox 사용
#include <vector>
#include "Model.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Edit)
class CMapObject final :public CGameObject
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
    virtual void Priority_Update(_float fTimeDelta);
    virtual void Update(_float fTimeDelta);
    virtual void Late_Update(_float fTimeDelta);
    virtual HRESULT Render();

private:
    CShader* m_pShaderCom = { nullptr };
    CModel* m_pModelCom = { nullptr };
    EObjectType m_eType = EObjectType::MONSTER;

    DirectX::BoundingBox m_LocalBox; // 모델 기준 바운딩박스 (로컬)
    bool m_bSelected = false; // 선택 표시

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    DirectX::BoundingBox GetWorldBoundingBox() const
    {
        DirectX::BoundingBox outBox;
        m_LocalBox.Transform(outBox, m_pTransformCom->Get_WorldMatrix());
        return outBox;
    }

    // 모델 로딩 후 AABB 자동 세팅
    void InitBoundingBoxFromModel()
    {
        if (m_pModelCom)
            m_pModelCom->ComputeBoundingBox(m_LocalBox); // 모델 전체 버텍스 min/max (구현 필요)
        else
            m_LocalBox = DirectX::BoundingBox(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
    }

public:
    static CMapObject* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};
NS_END
