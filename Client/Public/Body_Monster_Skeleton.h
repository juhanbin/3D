#pragma once

#include "Client_Defines.h"
#include "PartObject.h"
#include "Monster_Skeleton.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CBody_Monster_Skeleton final : public CPartObject
{
public:
    typedef struct tagBodyMonster_Skeleton_Desc : public CPartObject::PARTOBJECT_DESC
    {
        MONSTER* pState = { nullptr };
    } BODY_MONSTER_SKELETON_DESC;

private:
    CBody_Monster_Skeleton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CBody_Monster_Skeleton(const CBody_Monster_Skeleton& Prototype);
    virtual ~CBody_Monster_Skeleton() = default;

public:
    _float4x4* Get_BoneMatrix(const _char* pBoneName);
    Engine::CModel* GetModel() const { return m_pModelCom; }   // ★ 추가: 바디 모델 포인터 노출

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    virtual void Priority_Update(_float fTimeDelta);
    virtual void Update(_float fTimeDelta);
    virtual void Late_Update(_float fTimeDelta);
    virtual HRESULT Render();

public:
    void SetClipSmart(int animIndex, bool loop, _float blendDur, bool forceRestart = false);

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

private:
    CShader* m_pShaderCom = { nullptr };
    CModel* m_pModelCom = { nullptr };

private:
    MONSTER* m_pParentState = { nullptr };

    // Current clip state
    int   m_iCurAnim = -1;
    bool  m_bCurLoop = true;
    float m_fCurBlend = 0.f;

    MONSTER m_prevState = MONSTER::SPEARE_IDLE;
    float   m_AttackCooldown = 0.f;
    float   m_AttackRepeatGap = 0.15f; // 연속 공격 텀

public:
    static CBody_Monster_Skeleton* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END
