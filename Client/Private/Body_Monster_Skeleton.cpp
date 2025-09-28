#include "Body_Monster_Skeleton.h"
#include "GameInstance.h"

CBody_Monster_Skeleton::CBody_Monster_Skeleton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPartObject{ pDevice, pContext }
{
}

CBody_Monster_Skeleton::CBody_Monster_Skeleton(const CBody_Monster_Skeleton& Prototype)
	: CPartObject{ Prototype }
{
}

_float4x4* CBody_Monster_Skeleton::Get_BoneMatrix(const _char* pBoneName)
{
	return m_pModelCom->Get_BoneMatrix(pBoneName);
}

HRESULT CBody_Monster_Skeleton::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CBody_Monster_Skeleton::Initialize(void* pArg)
{
	if (!pArg) return E_FAIL;

	BODY_MONSTER_SKELETON_DESC* pDesc = static_cast<BODY_MONSTER_SKELETON_DESC*>(pArg);
	m_pParentState = pDesc->pState;

	if (FAILED(__super::Initialize(pArg))) return E_FAIL;
	if (FAILED(Ready_Components()))       return E_FAIL;

	//m_pModelCom->Set_Animation(17, true);

	return S_OK;
}

void CBody_Monster_Skeleton::Priority_Update(_float fTimeDelta)
{
}

// Body_Monster_Skeleton::Update
void CBody_Monster_Skeleton::Update(_float fTimeDelta)
{
	if (!m_pParentState) return;

	if (m_AttackCooldown > 0.f) m_AttackCooldown -= fTimeDelta;

	// 이번 프레임에 현재 클립이 끝났는지
	const bool finished = m_pModelCom->Play_Animation(fTimeDelta);

	constexpr int ANIM_IDLE = 7;
	constexpr int ANIM_WALK = 19;
	constexpr int ANIM_ATTACK = 5;
	constexpr int ANIM_BOW_IDLE = 1;
	constexpr int ANIM_BOW_ATTACK = 2;
	constexpr int ANIM_DIE_START = 17;
	constexpr int ANIM_DIE_END = 18;

	const MONSTER cur = *m_pParentState;
	const bool stateChanged = (cur != m_prevState);

	switch (cur)
	{
	case MONSTER::HIT:
		if (stateChanged) {
			SetClipSmart(ANIM_DIE_START, false, 0.08f, true);
		}
		else {
			if (finished && m_iCurAnim == ANIM_DIE_START) {
				SetClipSmart(ANIM_DIE_END, false, 0.0f, true);
			}
		}
		break;

	case MONSTER::SPEARE_IDLE:
		SetClipSmart(ANIM_IDLE, true, 0.15f, stateChanged);
		break;

	case MONSTER::WALK:
		SetClipSmart(ANIM_WALK, true, 0.10f, stateChanged);
		break;

	case MONSTER::SPEARE_ATTACK:
		if (stateChanged) {
			SetClipSmart(ANIM_ATTACK, false, 0.05f, true);
			m_AttackCooldown = m_AttackRepeatGap;
		}
		else {
			if (finished && m_AttackCooldown <= 0.f) {
				SetClipSmart(ANIM_ATTACK, false, 0.0f, true);
				m_AttackCooldown = m_AttackRepeatGap;
			}
		}
		break;
	case MONSTER::Bow_IDLE:
		SetClipSmart(ANIM_BOW_IDLE, true, 0.15f, stateChanged);
		break;

	case MONSTER::BOW_ATTACK:
		if (stateChanged) {
			SetClipSmart(ANIM_BOW_ATTACK, false, 0.05f, true);
			m_AttackCooldown = m_AttackRepeatGap;
		}
		else {
			if (finished && m_AttackCooldown <= 0.f) {
				SetClipSmart(ANIM_BOW_ATTACK, false, 0.0f, true);
				m_AttackCooldown = m_AttackRepeatGap;
			}
		}
	}

	m_prevState = cur;

	Update_CombinedMatrix();
}


void CBody_Monster_Skeleton::Late_Update(_float fTimeDelta)
{
	if (FAILED(m_pGameInstance->Add_RenderGroup(RENDERGROUP::NONBLEND, this)))
		return;

}

HRESULT CBody_Monster_Skeleton::Render()
{
	if (FAILED(Bind_ShaderResources()))
		return E_FAIL;

	_uint iNumMeshes = m_pModelCom->Get_NumMeshes();
	for (size_t i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->Bind_Materials_Bin(
			m_pShaderCom, "g_DiffuseTexture", i, ENUM_CLASS(TextureType::DIFFUSE), 0)))
		{
			OutputDebugStringA("Monster_body_머티리얼 바인딩 실패!\n");
			return E_FAIL;
		}

		if (FAILED(m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i)))
			return E_FAIL;

		m_pShaderCom->Begin(0);
		m_pModelCom->Render(i);
	}
	return S_OK;
}

void CBody_Monster_Skeleton::SetClipSmart(int animIndex, bool loop, _float blendDur, bool forceRestart)
{
	// 1) 다른 인덱스면 무조건 세팅(재시작)
	if (animIndex != m_iCurAnim) {
		m_pModelCom->Set_Animation(animIndex, loop, blendDur, true);
		m_iCurAnim = animIndex; m_bCurLoop = loop; m_fCurBlend = blendDur;
		return;
	}

	// 2) 같은 인덱스지만 강제 재시작이면 리스타트
	if (forceRestart) {
		m_pModelCom->Set_Animation(animIndex, loop, blendDur, true);
		m_bCurLoop = loop; m_fCurBlend = blendDur;
		return;
	}

	// 3) 같은 인덱스 + loop/fade 변경만 → 리스타트 없이 반영
	if (loop != m_bCurLoop || std::fabs(blendDur - m_fCurBlend) > 1e-3f) {
		m_pModelCom->Set_Animation(animIndex, loop, blendDur, false);
		m_bCurLoop = loop; m_fCurBlend = blendDur;
		return;
	}
}

HRESULT CBody_Monster_Skeleton::Ready_Components()
{
	if (FAILED(CGameObject::Add_Component(
		ENUM_CLASS(LEVEL::BRIDGE), TEXT("Prototype_Component_Shader_VtxAnimMesh"),
		TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom), nullptr)))
		return E_FAIL;

	if (FAILED(CGameObject::Add_Component(
		ENUM_CLASS(LEVEL::BRIDGE), TEXT("Prototype_Component_Model_Monster_Skeleton"),
		TEXT("Com_Model"), reinterpret_cast<CComponent**>(&m_pModelCom), nullptr)))
		return E_FAIL;

	return S_OK;
}

HRESULT CBody_Monster_Skeleton::Bind_ShaderResources()
{
	if (FAILED(m_pShaderCom->Bind_Matrix("g_WorldMatrix", &m_CombinedWorldMatrix)))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Bind_Matrix("g_ViewMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::VIEW))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Bind_Matrix("g_ProjMatrix", m_pGameInstance->Get_Transform_Float4x4(D3DTS::PROJ))))
		return E_FAIL;

	/*const LIGHT_DESC* pLightDesc = m_pGameInstance->Get_LightDesc(0);
	if (!pLightDesc) return E_FAIL;

	if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDir", &pLightDesc->vDirection, sizeof(_float4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightDiffuse", &pLightDesc->vDiffuse, sizeof(_float4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightAmbient", &pLightDesc->vAmbient, sizeof(_float4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Bind_RawValue("g_vLightSpecular", &pLightDesc->vSpecular, sizeof(_float4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Bind_RawValue("g_vCamPosition", m_pGameInstance->Get_CamPosition(), sizeof(_float4)))) return E_FAIL;*/

	return S_OK;
}

CBody_Monster_Skeleton* CBody_Monster_Skeleton::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBody_Monster_Skeleton* pInstance = new CBody_Monster_Skeleton(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype())) {
		MSG_BOX(TEXT("Failed to Created : CBody_Monster_Skeleton"));
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CBody_Monster_Skeleton::Clone(void* pArg)
{
	CBody_Monster_Skeleton* pInstance = new CBody_Monster_Skeleton(*this);
	if (FAILED(pInstance->Initialize(pArg))) {
		MSG_BOX(TEXT("Failed to Clone : CBody_Monster_Skeleton"));
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CBody_Monster_Skeleton::Free()
{
	__super::Free();
	Safe_Release(m_pModelCom);
	Safe_Release(m_pShaderCom);
}
