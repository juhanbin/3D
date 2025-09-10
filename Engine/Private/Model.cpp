#include "Model.h"

#include "Mesh.h"
#include "Bone.h"
#include "MeshMaterial.h"
#include "Animation.h"
#include <fstream>
#include <cfloat>

using namespace Engine;

CModel::CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent{ pDevice ,pContext } {
}

CModel::CModel(const CModel& Prototype)
    : CComponent{ Prototype }
    , m_iNumMeshes{ Prototype.m_iNumMeshes }
    , m_Meshes{ Prototype.m_Meshes }
    , m_eModelType{ Prototype.m_eModelType }
    , m_PreTransformMatrix{ Prototype.m_PreTransformMatrix }
    , m_iNumMaterials{ Prototype.m_iNumMaterials }
    , m_Materials{ Prototype.m_Materials }
    , m_iNumAnimations{ Prototype.m_iNumAnimations }
    , m_bFromBin{ Prototype.m_bFromBin }
    , m_prevAnim01{ 0.f }
    , m_curAnim01{ 0.f }
{
    for (auto& pPrototypeAnimation : Prototype.m_Animations)
        m_Animations.push_back(pPrototypeAnimation->Clone());

    for (auto& pPrototypeBone : Prototype.m_Bones)
        m_Bones.push_back(pPrototypeBone->Clone());

    for (auto& pMesh : m_Meshes)      Safe_AddRef(pMesh);
    for (auto& pMaterial : m_Materials) Safe_AddRef(pMaterial);
}

_float4x4* CModel::Get_BoneMatrix(const _char* pBoneName)
{
    auto iter = find_if(m_Bones.begin(), m_Bones.end(),
        [&](CBone* pBone) { return pBone->Compare_Name(pBoneName); });

    if (iter == m_Bones.end()) return nullptr;
    return (*iter)->Get_CombinedTransformationMatrixPtr();
}

HRESULT CModel::Initialize_Prototype(MODELTYPE eModelType, FILETYPE eFileType,
    const _char* pModelFilePath, _fmatrix PreTransformMatrix)
{
    m_eModelType = eModelType;
    XMStoreFloat4x4(&m_PreTransformMatrix, PreTransformMatrix);
    m_bFromBin = (eFileType == FILETYPE::BIN);

    if (eFileType == FILETYPE::BIN)
    {
        if (eModelType == MODELTYPE::NONANIM)
        {
            ifstream ifs(pModelFilePath, std::ios::binary);
            if (!ifs) { OutputDebugStringA("[BIN] 파일 열기 실패!\n"); return E_FAIL; }

            if (FAILED(Ready_Meshes(ifs, eModelType))) { OutputDebugStringA("[BIN] Ready_Meshes 실패 (NONANIM)\n"); return E_FAIL; }

            uint32_t materialCount = 0;
            ifs.read((char*)&materialCount, sizeof(materialCount));
            vector<MaterialInfoBin2> binMaterials(materialCount);
            ifs.read((char*)binMaterials.data(), sizeof(MaterialInfoBin2) * materialCount);

            if (FAILED(Ready_Materials(pModelFilePath, binMaterials))) { OutputDebugStringA("[BIN] Ready_Materials 실패 (NONANIM)\n"); return E_FAIL; }
        }
        else
        {
            ifstream ifs(pModelFilePath, std::ios::binary);
            if (!ifs) { OutputDebugStringA("[BIN] 파일 열기 실패!\n"); return E_FAIL; }

            uint32_t boneCount = 0;
            ifs.read((char*)&boneCount, sizeof(boneCount));
            vector<BoneInfoBin> binBones(boneCount);
            ifs.read((char*)binBones.data(), sizeof(BoneInfoBin) * boneCount);

            if (FAILED(Ready_Bones(binBones, -1))) return E_FAIL;
            if (FAILED(Ready_Meshes(ifs, eModelType))) return E_FAIL;

            uint32_t materialCount = 0;
            ifs.read((char*)&materialCount, sizeof(materialCount));
            vector<MaterialInfoBin2> binMaterials(materialCount);
            ifs.read((char*)binMaterials.data(), sizeof(MaterialInfoBin2) * materialCount);

            if (FAILED(Ready_Materials(pModelFilePath, binMaterials))) {
                OutputDebugStringA("[BIN] Ready_Materials 실패 (ANIM)\n");
                return E_FAIL;
            }
            if (FAILED(Ready_Animations(ifs))) return E_FAIL;
        }
    }
    else
    {
        _uint iFlag = { aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast };
        if (MODELTYPE::NONANIM == m_eModelType) iFlag |= aiProcess_PreTransformVertices;

        m_pAIScene = m_Importer.ReadFile(pModelFilePath, iFlag);
        if (nullptr == m_pAIScene) return E_FAIL;

        if (FAILED(Ready_Bones(m_pAIScene->mRootNode, -1))) return E_FAIL;
        if (FAILED(Ready_Meshes())) return E_FAIL;
        if (FAILED(Ready_Materials(pModelFilePath))) return E_FAIL;
        if (FAILED(Ready_Animations())) return E_FAIL;
    }
    return S_OK;
}

HRESULT CModel::Initialize(void* pArg) { return S_OK; }

HRESULT CModel::Bind_Materials(CShader* pShader, const _char* pConstantName,
    _uint iMeshIndex, aiTextureType eTextureType, _uint iIndex)
{
    if (iMeshIndex >= m_iNumMeshes) return E_FAIL;
    _uint iMaterialIndex = m_Meshes[iMeshIndex]->Get_MaterialIndex();
    if (m_iNumMaterials <= iMaterialIndex) return E_FAIL;
    return m_Materials[iMaterialIndex]->Bind_Resources(pShader, pConstantName, eTextureType, iIndex);
}

HRESULT CModel::Bind_Materials_Bin(CShader* pShader, const _char* pConstantName,
    _uint iMeshIndex, int texType, _uint iIndex)
{
    if (iMeshIndex >= m_iNumMeshes) return E_FAIL;
    _uint iMaterialIndex = m_Meshes[iMeshIndex]->Get_MaterialIndex();
    if (m_iNumMaterials <= iMaterialIndex) return E_FAIL;
    return m_Materials[iMaterialIndex]->Bind_Resources_Bin(pShader, pConstantName, texType, iIndex);
}

HRESULT CModel::Bind_BoneMatrices(CShader* pShader, const _char* pConstantName, _uint iMeshIndex)
{
    if (iMeshIndex >= m_iNumMeshes) return E_FAIL;
    return m_Meshes[iMeshIndex]->Bind_BoneMatrices(pShader, pConstantName, m_Bones);
}

_bool CModel::Play_Animation(_float fTimeDelta)
{
    m_isFinished = false;

    if (m_iCurrentAnimIndex < 0 || m_iCurrentAnimIndex >= (int)m_Animations.size()) {
        OutputDebugStringA("애니메이션 인덱스 오류! (out of range)\n");
        return false;
    }

    // ---- 이벤트용: 업데이트 '전' 진행도 샘플(비블렌딩 구간 기준) ----
    if (!m_inTransition) {
        m_prevAnim01 = m_Animations[m_iCurrentAnimIndex]->Get_Progress01();
    }

    if (!m_inTransition)
    {
        m_Animations[m_iCurrentAnimIndex]->Update_TransformationMatrices(
            m_Bones, m_isLoop, &m_isFinished, fTimeDelta);
    }
    else
    {
        if (m_iNextAnimIndex < 0 || m_iNextAnimIndex >= (int)m_Animations.size())
        {
            m_inTransition = false;
            m_Animations[m_iCurrentAnimIndex]->Update_TransformationMatrices(
                m_Bones, m_isLoop, &m_isFinished, fTimeDelta);
        }
        else
        {
            CAnimation* cur = m_Animations[m_iCurrentAnimIndex];
            CAnimation* next = m_Animations[m_iNextAnimIndex];

            cur->Advance_Time(m_isLoop, &m_isFinished, fTimeDelta);
            next->Advance_Time(true, nullptr, fTimeDelta);

            cur->EvaluatePose(m_Bones, m_poseCur, m_hasCur);
            next->EvaluatePose(m_Bones, m_poseNext, m_hasNext);

            m_blendAcc += fTimeDelta;
            float a = m_blendDur > 0.f ? std::min(1.f, m_blendAcc / m_blendDur) : 1.f;

            const _vector origin = XMVectorSet(0.f, 0.f, 0.f, 1.f);

            for (size_t i = 0; i < m_Bones.size(); ++i)
            {
                if (!m_hasCur[i] && !m_hasNext[i]) continue;

                const TRS& A = m_hasCur[i] ? m_poseCur[i] : m_poseNext[i];
                const TRS& B = m_hasNext[i] ? m_poseNext[i] : m_poseCur[i];

                _vector sA = XMLoadFloat3(&A.vScale);
                _vector sB = XMLoadFloat3(&B.vScale);
                _vector s = XMVectorLerp(sA, sB, a);

                _vector qA = XMLoadFloat4(&A.vRotation);
                _vector qB = XMLoadFloat4(&B.vRotation);
                _vector q = XMQuaternionSlerp(qA, qB, a);

                _vector pA = XMLoadFloat3(&A.vTranslation);
                _vector pB = XMLoadFloat3(&B.vTranslation);
                _vector p = XMVectorLerp(pA, pB, a);

                _matrix M = XMMatrixAffineTransformation(s, origin, q, p);
                m_Bones[i]->Set_TransformationMatrix(M);
            }

            if (m_blendAcc >= m_blendDur) {
                m_iCurrentAnimIndex = m_iNextAnimIndex;
                m_iNextAnimIndex = -1;
                m_inTransition = false;
            }
        }
    }

    for (auto& pBone : m_Bones)
        pBone->Update_CombinedTransformationMatrix(m_PreTransformMatrix, m_Bones);

    // ---- 이벤트용: 업데이트 '후' 진행도 샘플(비블렌딩 구간 기준) ----
    if (!m_inTransition) {
        m_curAnim01 = m_Animations[m_iCurrentAnimIndex]->Get_Progress01();
    }

    return m_isFinished;
}

// 클라에서: model->AnimCrossedNormalized(60.f/76.f) 처럼 사용
bool CModel::AnimCrossedNormalized(float t01) const
{
    if (m_iCurrentAnimIndex < 0 || m_iCurrentAnimIndex >= (int)m_Animations.size())
        return false;
    // 블렌딩 중엔 보통 트리거를 지연/무시 (원하면 제거)
    if (m_inTransition) return false;

    return CAnimation::Crossed_Normalized(t01, m_prevAnim01, m_curAnim01);
}

HRESULT CModel::Render(_uint iMeshIndex)
{
    if (FAILED(m_Meshes[iMeshIndex]->Bind_Resources())) return E_FAIL;
    if (FAILED(m_Meshes[iMeshIndex]->Render()))         return E_FAIL;
    return S_OK;
}

void CModel::Set_Animation(_uint iIndex, _bool isLoop, float blendDuration, bool forceRestart)
{
    if (iIndex >= m_iNumAnimations) return;

    const bool changing = (iIndex != m_iCurrentAnimIndex) || forceRestart;
    if (!changing) { m_isLoop = isLoop; return; }

    m_isFinished = false;
    m_isLoop = isLoop;

    if (m_iCurrentAnimIndex < 0 || blendDuration <= 0.f)
    {
        m_iCurrentAnimIndex = (_int)iIndex;
        if (forceRestart) m_Animations[m_iCurrentAnimIndex]->ResetTimeToZero();
        m_inTransition = false;
        m_iNextAnimIndex = -1;
        // 이벤트 창 리셋
        m_prevAnim01 = m_curAnim01 = 0.f;
        return;
    }

    m_iNextAnimIndex = (_int)iIndex;
    m_blendDur = max(0.0001f, blendDuration);
    m_blendAcc = 0.f;
    m_inTransition = true;

    if (forceRestart) m_Animations[m_iNextAnimIndex]->ResetTimeToZero();
    // 전환 시작 시 이벤트 창 리셋
    m_prevAnim01 = m_curAnim01 = 0.f;
}

HRESULT CModel::Ready_Meshes()
{
    m_iNumMeshes = m_pAIScene->mNumMeshes;
    for (size_t i = 0; i < m_iNumMeshes; i++)
    {
        CMesh* pMesh = CMesh::Create(m_pDevice, m_pContext, m_eModelType,
            m_pAIScene->mMeshes[i], m_Bones,
            XMLoadFloat4x4(&m_PreTransformMatrix));
        if (nullptr == pMesh) return E_FAIL;
        m_Meshes.push_back(pMesh);
    }
    return S_OK;
}

HRESULT CModel::Ready_Materials(const _char* pModelFilePath)
{
    m_iNumMaterials = m_pAIScene->mNumMaterials;
    for (size_t i = 0; i < m_iNumMaterials; i++)
    {
        CMeshMaterial* pMeshMaterial =
            CMeshMaterial::Create(m_pDevice, m_pContext, pModelFilePath, m_pAIScene->mMaterials[i]);
        if (nullptr == pMeshMaterial) return E_FAIL;
        m_Materials.push_back(pMeshMaterial);
    }
    return S_OK;
}

HRESULT CModel::Ready_Bones(const aiNode* pAINode, _int iParentIndex)
{
    CBone* pBone = CBone::Create(pAINode, iParentIndex);
    if (nullptr == pBone) return E_FAIL;

    m_Bones.push_back(pBone);
    _int iIndex = (int)m_Bones.size() - 1;

    for (size_t i = 0; i < pAINode->mNumChildren; i++)
        Ready_Bones(pAINode->mChildren[i], iIndex);

    return S_OK;
}

HRESULT CModel::Ready_Animations()
{
    m_iNumAnimations = m_pAIScene->mNumAnimations;
    for (size_t i = 0; i < m_iNumAnimations; i++)
    {
        CAnimation* pAnimation = CAnimation::Create(m_pAIScene->mAnimations[i], m_Bones);
        if (nullptr == pAnimation) return E_FAIL;
        m_Animations.push_back(pAnimation);
    }
    return S_OK;
}

HRESULT CModel::Ready_Meshes(ifstream& ifs, MODELTYPE eModelType)
{
    uint32_t meshCount = 0;
    ifs.read((char*)&meshCount, sizeof(meshCount));
    m_iNumMeshes = meshCount;

    if (eModelType == MODELTYPE::NONANIM)
    {
        std::vector<MeshInfoBin> infos(meshCount);
        ifs.read((char*)infos.data(), sizeof(MeshInfoBin) * meshCount);

        std::vector<std::vector<VTXMESH>> allVerts(meshCount);
        for (uint32_t i = 0; i < meshCount; ++i) {
            allVerts[i].resize(infos[i].NumVertices);
            if (infos[i].NumVertices)
                ifs.read((char*)allVerts[i].data(), sizeof(VTXMESH) * infos[i].NumVertices);
        }

        std::vector<std::vector<uint32_t>> allIdx(meshCount);
        for (uint32_t i = 0; i < meshCount; ++i) {
            allIdx[i].resize(infos[i].NumIndices);
            if (infos[i].NumIndices)
                ifs.read((char*)allIdx[i].data(), sizeof(uint32_t) * infos[i].NumIndices);
        }

        for (uint32_t i = 0; i < meshCount; ++i) {
            CMesh* pMesh = CMesh::Create(
                m_pDevice, m_pContext, m_eModelType,
                infos[i], allVerts[i], allIdx[i],
                m_Bones, XMLoadFloat4x4(&m_PreTransformMatrix));
            if (!pMesh) return E_FAIL;
            m_Meshes.push_back(pMesh);
        }
    }
    else
    {
        for (uint32_t i = 0; i < meshCount; ++i) {
            MeshInfoBin info{};
            ifs.read(reinterpret_cast<char*>(&info), sizeof(MeshInfoBin));

            vector<VTXANIMMESH> verts(info.NumVertices);
            if (info.NumVertices)
                ifs.read(reinterpret_cast<char*>(verts.data()), sizeof(VTXANIMMESH) * info.NumVertices);

            vector<uint32_t> indices(info.NumIndices);
            if (info.NumIndices)
                ifs.read(reinterpret_cast<char*>(indices.data()), sizeof(uint32_t) * info.NumIndices);

            uint32_t boneSlotCount = 0;
            ifs.read(reinterpret_cast<char*>(&boneSlotCount), sizeof(boneSlotCount));

            vector<MeshBoneRaw> meshBones(boneSlotCount);
            if (boneSlotCount)
                ifs.read(reinterpret_cast<char*>(meshBones.data()), sizeof(MeshBoneRaw) * boneSlotCount);

            CMesh* pMesh = CMesh::Create(
                m_pDevice, m_pContext, m_eModelType,
                info, verts, indices, meshBones,
                m_Bones, XMLoadFloat4x4(&m_PreTransformMatrix));
            if (!pMesh) return E_FAIL;
            m_Meshes.push_back(pMesh);
        }
    }
    return S_OK;
}

HRESULT CModel::Ready_Materials(const _char* pModelFilePath, const vector<MaterialInfoBin2>& binMaterials)
{
    m_iNumMaterials = static_cast<_uint>(binMaterials.size());
    m_Materials.reserve(m_iNumMaterials);

    for (size_t i = 0; i < m_iNumMaterials; ++i)
    {
        CMeshMaterial* pMeshMaterial =
            CMeshMaterial::Create(m_pDevice, m_pContext, pModelFilePath, binMaterials[i]);
        if (!pMeshMaterial) return E_FAIL;
        m_Materials.push_back(pMeshMaterial);
    }
    return S_OK;
}

HRESULT CModel::Ready_Bones(const vector<BoneInfoBin>& binBones, _int iParentIndex)
{
    m_Bones.clear();
    for (size_t i = 0; i < binBones.size(); ++i)
    {
        CBone* pBone = CBone::Create(binBones[i]);
        if (!pBone) return E_FAIL;
        m_Bones.push_back(pBone);
    }
    return S_OK;
}

HRESULT CModel::Ready_Animations(std::ifstream& ifs)
{
    uint32_t numAnims = 0;
    ifs.read((char*)&numAnims, sizeof(numAnims));
    m_iNumAnimations = numAnims;

    for (size_t i = 0; i < m_iNumAnimations; i++)
    {
        AnimInfoBin animBin = {};
        ifs.read((char*)&animBin, sizeof(AnimInfoBin));

        CAnimation* pAnimation = CAnimation::Create(ifs, animBin, m_Bones);
        if (nullptr == pAnimation) return E_FAIL;
        m_Animations.push_back(pAnimation);
    }
    return S_OK;
}

CModel* CModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
    MODELTYPE eModelType, FILETYPE eFileType,
    const _char* pModelFilePath, _fmatrix PreTransformMatrix)
{
    CModel* pInstance = new CModel(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(eModelType, eFileType, pModelFilePath, PreTransformMatrix)))
    {
        MSG_BOX(TEXT("Failed to Created : CModel"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

CComponent* CModel::Clone(void* pArg)
{
    CModel* pInstance = new CModel(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Created : CModel"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CModel::Free()
{
    __super::Free();

    for (auto& pAnimation : m_Animations) Safe_Release(pAnimation);
    m_Animations.clear();

    for (auto& pBone : m_Bones) Safe_Release(pBone);
    m_Bones.clear();

    for (auto& pMesh : m_Meshes) Safe_Release(pMesh);
    m_Meshes.clear();

    for (auto& pMaterial : m_Materials) Safe_Release(pMaterial);
    m_Materials.clear();

    m_Importer.FreeScene();
}
