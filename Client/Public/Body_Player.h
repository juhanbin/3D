#pragma once

#include "Client_Defines.h"
#include "PartObject.h"
#include "Player.h"   // MOVING, ATTACK

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CBody_Player final : public CPartObject
{
public:
    struct BODY_DESC : public CPartObject::PARTOBJECT_DESC
    {
        _uint* pState = nullptr;
        MOVING* pMoving = nullptr;
        ATTACK* pAttack = nullptr;
    };

private:
    CBody_Player(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CBody_Player(const CBody_Player& Prototype);
    virtual ~CBody_Player() = default;

public:
    _float4x4* Get_BoneMatrix(const _char* pBoneName);

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual void    Priority_Update(_float fTimeDelta) override;
    virtual void    Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

public:
    void SetClipSmart(int animIndex, bool loop, _float blendDur, bool forceRestart = false);

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

private:
    CShader* m_pShaderCom = nullptr;
    CModel* m_pModelCom = nullptr;
    CCollider* m_pColliderCom = { nullptr };

    _uint* m_pParentState = nullptr;
    MOVING* m_pMoving = nullptr;
    ATTACK* m_pAttack = nullptr;

    int   m_iCurAnim = -1;
    bool  m_bCurLoop = true;
    float m_fCurBlend = 0.f;

    bool  m_bDashPlaying = false;
    bool  m_bGroundPlaying = false;

    int   m_LastLocoAnim = -1;   
    float m_LocoHold = 0.f;   
    const float LOCO_MIN_HOLD = 0.03f;

    float m_fDashFinishBlock = 0.f;

public:
    static CBody_Player* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void         Free() override;
};

NS_END
