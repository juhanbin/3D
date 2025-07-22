#pragma once
#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)
enum class EObjectType : int
{
    MONSTER ,
    ROCK_AA ,
    // ...추가 오브젝트 타입
};

class CMapObject final : public CGameObject
{
public:
    struct MAPOBJECT_DESC : public CGameObject::GAMEOBJECT_DESC
    {
        EObjectType type{ EObjectType::MONSTER };
        _float3 vScale{ 1.f, 1.f, 1.f };
        _float3 vRot{ 0.f, 0.f, 0.f };
        _float3 vPos{ 0.f, 0.f, 0.f };
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
    CShader* m_pShaderCom = nullptr;
    CModel* m_pModelCom = nullptr;
    EObjectType m_eType = EObjectType::MONSTER;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

public:
    static CMapObject* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};
NS_END
