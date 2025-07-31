#include "Model.h"

#include "Mesh.h"
#include "Bone.h"
#include "MeshMaterial.h"
#include "Animation.h"
#include <fstream>

CModel::CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent{ pDevice ,pContext }
{
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
{
    for (auto& pPrototypeAnimation : Prototype.m_Animations)
        m_Animations.push_back(pPrototypeAnimation->Clone());

    for (auto& pPrototypeBone : Prototype.m_Bones)
        m_Bones.push_back(pPrototypeBone->Clone());

    for (auto& pMesh : m_Meshes)
        Safe_AddRef(pMesh);

    for (auto& pMaterial : m_Materials)
        Safe_AddRef(pMaterial);
}

HRESULT CModel::Initialize_Prototype(MODELTYPE eModelType, FILETYPE eFileType, const _char* pModelFilePath, _fmatrix PreTransformMatrix)
{
    m_eModelType = eModelType;
    XMStoreFloat4x4(&m_PreTransformMatrix, PreTransformMatrix);

    char dbg[256];
    sprintf_s(dbg, "[DEBUG] sizeof AnimInfo=%zu ChannelInfo=%zu KeyFrame=%zu\n",
        sizeof(AnimInfo), sizeof(ChannelInfo), sizeof(KeyFrame));
    OutputDebugStringA(dbg);

    if (eFileType == FILETYPE::BIN)
    {
        std::ifstream ifs(pModelFilePath, std::ios::binary);
        if (!ifs) {
            OutputDebugStringA("[BIN] 파일 열기 실패!\n");
            return E_FAIL;
        }

        // ✅ Mesh Count 출력
        if (eModelType == MODELTYPE::NONANIM)
        {
            if (FAILED(Ready_Meshes(ifs)))
                return E_FAIL;

            uint32_t materialCount = 0;
            ifs.read((char*)&materialCount, sizeof(materialCount));
            sprintf_s(dbg, "[LOAD] MaterialCount=%u\n", materialCount);
            OutputDebugStringA(dbg);

            vector<MaterialInfo> binMaterials(materialCount);
            ifs.read((char*)binMaterials.data(), sizeof(MaterialInfo) * materialCount);
            if (FAILED(Ready_Materials(pModelFilePath, binMaterials)))
                return E_FAIL;
        }
        else
        {
            // ✅ Mesh 읽기
            if (FAILED(Ready_Meshes(ifs))) return E_FAIL;

            // ✅ Material 읽기
            uint32_t materialCount = 0;
            ifs.read((char*)&materialCount, sizeof(materialCount));
            sprintf_s(dbg, "[LOAD] MaterialCount=%u\n", materialCount);
            OutputDebugStringA(dbg);

            vector<MaterialInfo> binMaterials(materialCount);
            ifs.read((char*)binMaterials.data(), sizeof(MaterialInfo) * materialCount);

            // ✅ Bone 읽기
            uint32_t boneCount = 0;
            ifs.read((char*)&boneCount, sizeof(boneCount));
            sprintf_s(dbg, "[LOAD] BoneCount=%u sizeof(BoneInfo)=%zu\n", boneCount, sizeof(BoneInfo));
            OutputDebugStringA(dbg);

            vector<BoneInfo> binBones(boneCount);
            ifs.read((char*)binBones.data(), sizeof(BoneInfo) * boneCount);
            if (FAILED(Ready_Bones(binBones, -1))) return E_FAIL;

            // ✅ Animation 읽기
            if (FAILED(Ready_Animations(ifs))) return E_FAIL;
        }
    }
    else
    {
        OutputDebugStringA("[FBX] FBX 파일 로드 시작\n");

        _uint iFlag = { aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast };
        if (MODELTYPE::NONANIM == m_eModelType)
            iFlag |= aiProcess_PreTransformVertices;

        m_pAIScene = m_Importer.ReadFile(pModelFilePath, iFlag);
        if (!m_pAIScene)
            return E_FAIL;

        sprintf_s(dbg, "[FBX] Meshes=%u Materials=%u Animations=%u\n",
            m_pAIScene->mNumMeshes, m_pAIScene->mNumMaterials, m_pAIScene->mNumAnimations);
        OutputDebugStringA(dbg);

        if (FAILED(Ready_Bones(m_pAIScene->mRootNode, -1))) return E_FAIL;
        if (FAILED(Ready_Meshes())) return E_FAIL;
        if (FAILED(Ready_Materials(pModelFilePath))) return E_FAIL;
        if (FAILED(Ready_Animations())) return E_FAIL;
    }

    return S_OK;
}


HRESULT CModel::Initialize(void* pArg)
{
    return S_OK;
}

HRESULT CModel::Bind_Materials(class CShader* pShader, const _char* pConstantName, _uint iMeshIndex, aiTextureType eTextureType, _uint iIndex)
{
    if (iMeshIndex >= m_iNumMeshes)
        return E_FAIL;

    _uint iMaterialIndex = m_Meshes[iMeshIndex]->Get_MaterialIndex();
    if (m_iNumMaterials <= iMaterialIndex)
        return E_FAIL;

    // 실제 바인딩 전 텍스처 정보 로그
    //char dbg[256];
    //sprintf_s(dbg, "Bind_Materials: MeshIndex=%u, MaterialIndex=%u, TextureType=%d, ConstantName=%s\n", iMeshIndex, iMaterialIndex, eTextureType, pConstantName);
    //OutputDebugStringA(dbg);

    char dbg[512];
    const char* fileName = "";

    if (eTextureType == aiTextureType_DIFFUSE)
        fileName = m_Materials[iMaterialIndex]->GetDiffuseFileName().c_str();
    else if (eTextureType == aiTextureType_NORMALS)
        fileName = m_Materials[iMaterialIndex]->GetNormalFileName().c_str();

    //sprintf_s(dbg,
    //    "[Bind_Materials] Mesh=%u MatIndex=%u Slot=%s Type=%d File=%s\n",
    //    iMeshIndex, iMaterialIndex, pConstantName, eTextureType, fileName);

    return m_Materials[iMaterialIndex]->Bind_Resources(pShader, pConstantName, eTextureType, iIndex);
}


HRESULT CModel::Bind_BoneMatrices(CShader* pShader, const _char* pConstantName, _uint iMeshIndex)
{
    if (iMeshIndex >= m_iNumMeshes)
        return E_FAIL;

    return m_Meshes[iMeshIndex]->Bind_BoneMatrices(pShader, pConstantName, m_Bones);
}

_bool CModel::Play_Animation(_float fTimeDelta)
{
    m_isFinished = false;

    /* 현재 시간에 맞는 뼈의 상태대로 특정 뼈들의 TransformationMatrix를 갱신해준다. */
    m_Animations[m_iCurrentAnimIndex]->Update_TransformationMatrices(m_Bones, m_isLoop, &m_isFinished, fTimeDelta);


    /* 바꿔야할 뼈들의 Transforemation행렬이 갱신되었다면, 정점들에게 직접 전달되야할 CombindTransformationMatrix를 만들어준다. */
    for (auto& pBone : m_Bones)
    {
        pBone->Update_CombinedTransformationMatrix(m_PreTransformMatrix, m_Bones);
    }

    return m_isFinished;
}

void CModel::ComputeBoundingBox(DirectX::BoundingBox& outBox) const
{
    using namespace DirectX;

    bool first = true;
    XMFLOAT3 vMin{}, vMax{};
    // 모든 메시 순회 (m_Meshes 컨테이너 등)
    for (const auto& mesh : m_Meshes)  // m_Meshes는 메시 리스트(vector 등)
    {
        const auto& positions = mesh->GetPositions();
        // 메시의 버텍스 배열 순회 (여기선 float3 포맷 가정)
        for (const auto& v : positions)
        {
            if (first) {
                vMin = vMax = v;
                first = false;
            }
            else {
                vMin.x = min(vMin.x, v.x);
                vMin.y = min(vMin.y, v.y);
                vMin.z = min(vMin.z, v.z);
                vMax.x = max(vMax.x, v.x);
                vMax.y = max(vMax.y, v.y);
                vMax.z = max(vMax.z, v.z);
            }
        }
    }
    // min/max에서 박스 생성
    XMFLOAT3 center = { (vMin.x + vMax.x) * 0.5f, (vMin.y + vMax.y) * 0.5f, (vMin.z + vMax.z) * 0.5f };
    XMFLOAT3 extents = { (vMax.x - vMin.x) * 0.5f, (vMax.y - vMin.y) * 0.5f, (vMax.z - vMin.z) * 0.5f };
    outBox = BoundingBox(center, extents);
}

HRESULT CModel::Render(_uint iMeshIndex)
{
    if (FAILED(m_Meshes[iMeshIndex]->Bind_Resources()))
        return E_FAIL;

    if (FAILED(m_Meshes[iMeshIndex]->Render()))
        return E_FAIL;

    return S_OK;
}

void CModel::Set_Animation(_uint iIndex, _bool isLoop)
{
    if (iIndex >= m_iNumAnimations)
        return;

    m_isLoop = isLoop;
    m_iCurrentAnimIndex = iIndex;
}

HRESULT CModel::Ready_Meshes()
{
    m_iNumMeshes = m_pAIScene->mNumMeshes;

    for (size_t i = 0; i < m_iNumMeshes; i++)
    {
        CMesh* pMesh = CMesh::Create(m_pDevice, m_pContext, m_eModelType, m_pAIScene->mMeshes[i], m_Bones, XMLoadFloat4x4(&m_PreTransformMatrix));
        if (nullptr == pMesh)
            return E_FAIL;

        m_Meshes.push_back(pMesh);
    }

    return S_OK;
}

HRESULT CModel::Ready_Materials(const _char* pModelFilePath)
{
    m_iNumMaterials = m_pAIScene->mNumMaterials;

    for (size_t i = 0; i < m_iNumMaterials; i++)
    {

        CMeshMaterial* pMeshMaterial = CMeshMaterial::Create(m_pDevice, m_pContext, pModelFilePath, m_pAIScene->mMaterials[i]);
        if (nullptr == pMeshMaterial)
            return E_FAIL;

        m_Materials.push_back(pMeshMaterial);
    }


    return S_OK;
}

HRESULT CModel::Ready_Bones(const aiNode* pAINode, _int iParentIndex)
{
    CBone* pBone = CBone::Create(pAINode, iParentIndex);
    if (nullptr == pBone)
        return E_FAIL;

    m_Bones.push_back(pBone);

    _int   iIndex = m_Bones.size() - 1;

    for (size_t i = 0; i < pAINode->mNumChildren; i++)
    {
        Ready_Bones(pAINode->mChildren[i], iIndex);
    }

    return S_OK;
}

HRESULT CModel::Ready_Animations()
{
    /* 시간에 따라 내 뼈들이 어떻게 움직여야하는가? 에 대한 정보가 필요하다.  */
    /* 대기동작을 위해서는 뼈들이 어떤 시간대에 어떤 상태를 취하는가? */
    /* 공격동작을 위해서는 뼈들이 어떤 시간대에 어떤 상태를 취하는가? */
    m_iNumAnimations = m_pAIScene->mNumAnimations;

    for (size_t i = 0; i < m_iNumAnimations; i++)
    {
        CAnimation* pAnimation = CAnimation::Create(m_pAIScene->mAnimations[i], m_Bones);        if (nullptr == pAnimation)
            return E_FAIL;

        m_Animations.push_back(pAnimation);
    }

    return S_OK;
}

HRESULT CModel::Ready_Meshes(ifstream& ifs)
{
    ifs.read((char*)&m_iNumMeshes, sizeof(m_iNumMeshes));
    char dbg[128];
    sprintf_s(dbg, "[LOAD] MeshCount=%u\n", m_iNumMeshes);
    OutputDebugStringA(dbg);

    for (size_t i = 0; i < m_iNumMeshes; ++i)
    {
        uint32_t vtxCount = 0, idxCount = 0;
        ifs.read((char*)&vtxCount, sizeof(vtxCount));
        ifs.read((char*)&idxCount, sizeof(idxCount));
        sprintf_s(dbg, "  [LOAD] Mesh[%zu]: vtx=%u idx=%u\n", i, vtxCount, idxCount);
        OutputDebugStringA(dbg);

        std::vector<SimpleVertex> vertices(vtxCount);
        std::vector<uint32_t> indices(idxCount);
        ifs.read((char*)vertices.data(), sizeof(SimpleVertex) * vtxCount);
        ifs.read((char*)indices.data(), sizeof(uint32_t) * idxCount);

        CMesh* pMesh = CMesh::Create(m_pDevice, m_pContext, m_eModelType, vertices, indices, m_Bones, XMLoadFloat4x4(&m_PreTransformMatrix));
        if (!pMesh) return E_FAIL;
        m_Meshes.push_back(pMesh);
    }
    return S_OK;
}


HRESULT CModel::Ready_Materials(const _char* pModelFilePath, const vector<MaterialInfo>& binMaterials)
{
    m_iNumMaterials = static_cast<_uint>(binMaterials.size());
    m_Materials.reserve(m_iNumMaterials);

    for (size_t i = 0; i < m_iNumMaterials; ++i)
    {
        CMeshMaterial* pMeshMaterial = CMeshMaterial::Create(
            m_pDevice, m_pContext, pModelFilePath, binMaterials[i]);
        if (!pMeshMaterial) return E_FAIL;
        m_Materials.push_back(pMeshMaterial);
    }
    return S_OK;
}

HRESULT CModel::Ready_Bones(const vector<BoneInfo>& binBones, _int iParentIndex)
{
    m_Bones.clear();
    m_Bones.reserve(binBones.size());

    for (size_t i = 0; i < binBones.size(); ++i)
    {
        CBone* pBone = CBone::Create(binBones[i], iParentIndex);
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

    char dbg[256];
    sprintf_s(dbg, "[LOAD] Animations=%u sizeof AnimInfo=%zu ChannelInfo=%zu KeyFrame=%zu\n",
        numAnims, sizeof(AnimInfo), sizeof(ChannelInfo), sizeof(KeyFrame));
    OutputDebugStringA(dbg);

    for (uint32_t i = 0; i < numAnims; ++i)
    {
        AnimInfo animBin{};
        ifs.read((char*)&animBin, sizeof(AnimInfo));

        sprintf_s(dbg, "[LOAD] Anim[%u]: name=%s Channels=%u Duration=%f TPS=%f\n",
            i, animBin.name, animBin.channelCount, animBin.duration, animBin.ticksPerSecond);
        OutputDebugStringA(dbg);

        std::vector<ChannelInfo> channelBins(animBin.channelCount);
        ifs.read((char*)channelBins.data(), sizeof(ChannelInfo) * animBin.channelCount);

        uint32_t totalKeyFrames = 0;
        for (uint32_t c = 0; c < animBin.channelCount; ++c)
        {
            channelBins[c].boneName[63] = 0;
            sprintf_s(dbg, "  [LOAD] Channel[%u]: bone=%s keyframes=%u\n",
                c, channelBins[c].boneName, channelBins[c].keyframeCount);
            OutputDebugStringA(dbg);
            totalKeyFrames += channelBins[c].keyframeCount;
        }

        std::vector<KeyFrame> keyframes(totalKeyFrames);
        ifs.read((char*)keyframes.data(), sizeof(KeyFrame) * totalKeyFrames);

        sprintf_s(dbg, "  [LOAD-DEBUG] Anim[%u] totalKeyFrames=%u\n", i, totalKeyFrames);
        OutputDebugStringA(dbg);

        CAnimation* pAnim = CAnimation::Create(animBin, channelBins, keyframes, m_Bones);
        if (!pAnim) return E_FAIL;
        m_Animations.push_back(pAnim);
    }

    return S_OK;
}








CModel* CModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eModelType, FILETYPE eFileType,const _char* pModelFilePath, _fmatrix PreTransformMatrix)
{
    CModel* pInstance = new CModel(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(eModelType, eFileType ,pModelFilePath, PreTransformMatrix)))
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

    for (auto& pAnimation : m_Animations)
        Safe_Release(pAnimation);

    m_Animations.clear();

    for (auto& pBone : m_Bones)
        Safe_Release(pBone);

    m_Bones.clear();

    for (auto& pMesh : m_Meshes)
        Safe_Release(pMesh);

    m_Meshes.clear();

    for (auto& pMaterial : m_Materials)
        Safe_Release(pMaterial);

    m_Materials.clear();


    m_Importer.FreeScene();



}
