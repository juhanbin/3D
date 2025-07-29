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

HRESULT CModel::Initialize_Prototype(MODELTYPE eModelType,FILETYPE eFileType, const _char* pModelFilePath, _fmatrix PreTransformMatrix)
{
    m_eModelType = eModelType;
    XMStoreFloat4x4(&m_PreTransformMatrix, PreTransformMatrix);


    if (eFileType == FILETYPE::BIN) // Bin파일일때
    {
        std::ifstream ifs(pModelFilePath, std::ios::binary);
        if (!ifs) {
            OutputDebugStringA("[BIN] 파일 열기 실패!\n");
            return E_FAIL;
        }
        if (!ifs)
            return E_FAIL;

        if (eModelType == MODELTYPE::NONANIM)
        {
            // 2. Mesh 정보 읽기
            if (FAILED(Ready_Meshes(ifs))) { // 파일 스트림 넘김!
                OutputDebugStringA("[BIN] Ready_Meshes 실패 (NONANIM)\n");
                return E_FAIL;
            }

            // 3. Material 정보 읽기
            uint32_t materialCount = 0;
            ifs.read((char*)&materialCount, sizeof(materialCount));
            {
                char dbg[128];
                sprintf_s(dbg, "BIN NONANIM materialCount = %u\n", materialCount);
                OutputDebugStringA(dbg);
            }
            vector<MaterialInfoBin> binMaterials(materialCount);
            ifs.read((char*)binMaterials.data(), sizeof(MaterialInfoBin) * materialCount);
            for (size_t i = 0; i < binMaterials.size(); ++i) {
                char dbg[256];
                sprintf_s(dbg, "BIN Material[%zu] basecolor=%s, normal=%s, arm=%s\n",
                    i, binMaterials[i].basecolor, binMaterials[i].normal, binMaterials[i].arm);
                OutputDebugStringA(dbg);
            }

            if (FAILED(Ready_Materials(pModelFilePath, binMaterials))) {
                OutputDebugStringA("[BIN] Ready_Materials 실패 (NONANIM)\n");
                return E_FAIL;
            }
        }
        else
        {
            // 1. Bone 정보 읽기
            uint32_t boneCount = 0;
            ifs.read((char*)&boneCount, sizeof(boneCount));
            vector<BoneInfoBin> binBones(boneCount);
            ifs.read((char*)binBones.data(), sizeof(BoneInfoBin) * boneCount);

            if (FAILED(Ready_Bones(binBones, -1)))
                return E_FAIL;

            // 2. Mesh 정보 읽기
            if (FAILED(Ready_Meshes(ifs))) // 파일 스트림 넘김!
                return E_FAIL;

            // 3. Material 정보 읽기
            uint32_t materialCount = 0;
            ifs.read((char*)&materialCount, sizeof(materialCount));
            vector<MaterialInfoBin> binMaterials(materialCount);
            ifs.read((char*)binMaterials.data(), sizeof(MaterialInfoBin) * materialCount);

            if (FAILED(Ready_Materials(pModelFilePath, binMaterials)))
                return E_FAIL;

            // 4. Animation 정보 읽기
            if (FAILED(Ready_Animations(ifs)))
                return E_FAIL;
        }
    }

    else // fbx파일일때
    {
        _uint           iFlag = { aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast };

        if (MODELTYPE::NONANIM == m_eModelType)
            iFlag |= aiProcess_PreTransformVertices;

        m_pAIScene = m_Importer.ReadFile(pModelFilePath, iFlag);
        if (nullptr == m_pAIScene)
            return E_FAIL;

        if (FAILED(Ready_Bones(m_pAIScene->mRootNode, -1)))
            return E_FAIL;

        if (FAILED(Ready_Meshes()))
            return E_FAIL;

        //XMMatrixRotationQuaternion();
        //XMMatrixRotationRollPitchYaw();

        if (FAILED(Ready_Materials(pModelFilePath)))
            return E_FAIL;

        if (FAILED(Ready_Animations()))
            return E_FAIL;
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

    _uint       iMaterialIndex = m_Meshes[iMeshIndex]->Get_MaterialIndex();

    if (m_iNumMaterials <= iMaterialIndex)
        return E_FAIL;

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
    for (size_t i = 0; i < m_iNumMeshes; ++i)
    {
        // --- BIN에서 vertex, index 읽어오기 ---
        uint32_t vtxCount = 0, idxCount = 0;
        ifs.read((char*)&vtxCount, sizeof(vtxCount));
        ifs.read((char*)&idxCount, sizeof(idxCount));
        vector<SimpleVertexBin> vertices(vtxCount);
        vector<uint32_t> indices(idxCount);
        ifs.read((char*)vertices.data(), sizeof(SimpleVertexBin) * vtxCount);
        ifs.read((char*)indices.data(), sizeof(uint32_t) * idxCount);

        // --- CMesh 생성 ---
        CMesh* pMesh = CMesh::Create(
            m_pDevice, m_pContext,
            m_eModelType,
            vertices, indices, m_Bones,XMLoadFloat4x4(&m_PreTransformMatrix)
        );
        if (nullptr == pMesh)
            return E_FAIL;
        m_Meshes.push_back(pMesh);
    }
    return S_OK;
}

HRESULT CModel::Ready_Materials(const _char* pModelFilePath,const vector<MaterialInfoBin>& binMaterials)
{
    m_iNumMaterials = static_cast<_uint>(binMaterials.size());
    m_Materials.reserve(m_iNumMaterials);

    for (size_t i = 0; i < m_iNumMaterials; ++i)
    {
        // BIN용 생성자 또는 별도 Create 함수 필요 (경로만 전달)
        CMeshMaterial* pMeshMaterial = CMeshMaterial::Create(m_pDevice, m_pContext, pModelFilePath, binMaterials[i]);
        if (nullptr == pMeshMaterial)
            return E_FAIL;

        m_Materials.push_back(pMeshMaterial);
    }

    return S_OK;
}

HRESULT CModel::Ready_Bones(const vector<BoneInfoBin>& binBones, _int iParentIndex)
{
    m_Bones.clear();
    m_Bones.reserve(binBones.size());

    // 1. 본들 모두 생성
    for (size_t i = 0; i < binBones.size(); ++i)
    {
        // CBone 생성자/팩토리에서 BoneInfoBin을 받아서 모든 정보 셋팅하도록 구현
        CBone* pBone = CBone::Create(binBones[i], iParentIndex);
        if (nullptr == pBone)
            return E_FAIL;
        m_Bones.push_back(pBone);
    }
}

HRESULT CModel::Ready_Animations(std::ifstream& ifs)
{
    uint32_t numAnims = 0;
    ifs.read((char*)&numAnims, sizeof(numAnims));
    m_iNumAnimations = numAnims;
    m_Animations.reserve(numAnims);

    for (uint32_t i = 0; i < numAnims; ++i)
    {
        AnimInfoBin animBin = {};
        ifs.read((char*)&animBin, sizeof(AnimInfoBin));

        // --- 채널 정보 읽기 ---
        std::vector<ChannelInfoBin> channelBins(animBin.channelCount);
        ifs.read((char*)channelBins.data(), sizeof(ChannelInfoBin) * animBin.channelCount);

        // --- 키프레임 전체 읽기 ---
        std::vector<KeyFrameBin> keyframeBins;
        keyframeBins.reserve(0);
        for (uint32_t c = 0; c < animBin.channelCount; ++c) {
            uint32_t kfCount = channelBins[c].keyframeCount;
            size_t oldSize = keyframeBins.size();
            keyframeBins.resize(oldSize + kfCount);
            ifs.read((char*)&keyframeBins[oldSize], sizeof(KeyFrameBin) * kfCount);
        }

        // --- BIN용 애니 생성 ---
        CAnimation* pAnim = CAnimation::Create(animBin, channelBins, keyframeBins, m_Bones);
        if (!pAnim)
            return E_FAIL;
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
