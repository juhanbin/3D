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

_float4x4* CModel::Get_BoneMatrix(const _char* pBoneName)
{
    auto    iter = find_if(m_Bones.begin(), m_Bones.end(), [&](CBone* pBone) {
        if (true == pBone->Compare_Name(pBoneName))
            return true;
        return false;
        });

    if (iter == m_Bones.end())
        return nullptr;

    return (*iter)->Get_CombinedTransformationMatrixPtr();
}

HRESULT CModel::Initialize_Prototype(MODELTYPE eModelType, FILETYPE eFileType, const _char* pModelFilePath, _fmatrix PreTransformMatrix)
{
    m_eModelType = eModelType;
    XMStoreFloat4x4(&m_PreTransformMatrix, PreTransformMatrix);


    if (eFileType == FILETYPE::BIN) // Bin파일일때
    {
        if (eModelType == MODELTYPE::NONANIM)
        {
            ifstream ifs(pModelFilePath, std::ios::binary);
            if (!ifs) {
                OutputDebugStringA("[BIN] 파일 열기 실패!\n");
                return E_FAIL;
            }

            // --- 1. 메시 정보/버텍스/인덱스 로드 ---
            if (FAILED(Ready_Meshes(ifs, eModelType))) {
                OutputDebugStringA("[BIN] Ready_Meshes 실패 (NONANIM)\n");
                return E_FAIL;
            }

            // --- 2. 머티리얼 정보 로드 ---
            uint32_t materialCount = 0;
            ifs.read((char*)&materialCount, sizeof(materialCount));
            vector<MaterialInfoBin2> binMaterials(materialCount);
            ifs.read((char*)binMaterials.data(), sizeof(MaterialInfoBin2) * materialCount);

            if (FAILED(Ready_Materials(pModelFilePath, binMaterials))) {
                OutputDebugStringA("[BIN] Ready_Materials 실패 (NONANIM)\n");
                return E_FAIL;
            }
        }

        else
        {
            ifstream ifs(pModelFilePath, std::ios::binary);
            if (!ifs) {
                OutputDebugStringA("[BIN] 파일 열기 실패!\n");
                return E_FAIL;
            }

            // 1. Bone 정보 읽기
            uint32_t boneCount = 0;
            ifs.read((char*)&boneCount, sizeof(boneCount));
            vector<BoneInfoBin> binBones(boneCount);
            ifs.read((char*)binBones.data(), sizeof(BoneInfoBin) * boneCount);

            if (FAILED(Ready_Bones(binBones, -1)))
                return E_FAIL;

            // 2. Mesh 정보 읽기
            if (FAILED(Ready_Meshes(ifs, eModelType))) // 파일 스트림 넘김
                return E_FAIL;

            // 3. Material 정보 읽기
            uint32_t materialCount = 0;
            ifs.read((char*)&materialCount, sizeof(materialCount));
            vector<MaterialInfoBin2> binMaterials(materialCount);
            ifs.read((char*)binMaterials.data(), sizeof(MaterialInfoBin2) * materialCount);

            // AnimInfoBin ChannelInfoBin  tagKeyFrame 
            if (FAILED(Ready_Materials(pModelFilePath, binMaterials))) {
                OutputDebugStringA("[BIN] Ready_Materials 실패 (NONANIM)\n");
                return E_FAIL;
            }

            //4. Animation 정보 읽기
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

HRESULT CModel::Bind_Materials_Bin(CShader* pShader, const _char* pConstantName, _uint iMeshIndex, int texType, _uint iIndex)
{
    if (iMeshIndex >= m_iNumMeshes)
        return E_FAIL;

    _uint iMaterialIndex = m_Meshes[iMeshIndex]->Get_MaterialIndex();

    if (m_iNumMaterials <= iMaterialIndex)
        return E_FAIL;

    return m_Materials[iMaterialIndex]->Bind_Resources_Bin(pShader, pConstantName, texType, iIndex);
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

    if (m_iCurrentAnimIndex < 0 || m_iCurrentAnimIndex >= m_Animations.size()) {
        OutputDebugStringA("애니메이션 인덱스 오류! (out of range)\n");
        return false;
    }

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

void CModel::Set_Animation(_uint iIndex, _bool isLoop, bool forceRestart)
{
    if (iIndex >= m_iNumAnimations) return;

    const bool changing = (iIndex != m_iCurrentAnimIndex) || forceRestart;

    m_iCurrentAnimIndex = iIndex;
    m_isLoop = isLoop;

    if (changing) {
        m_isFinished = false;
        if (m_iCurrentAnimIndex >= 0 && m_iCurrentAnimIndex < (int)m_Animations.size())
            m_Animations[m_iCurrentAnimIndex]->ResetTimeToZero();  // ★ 여기서 0프레임으로 리셋
    }
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
        CAnimation* pAnimation = CAnimation::Create(m_pAIScene->mAnimations[i], m_Bones);   

        if (nullptr == pAnimation)
            return E_FAIL;

        m_Animations.push_back(pAnimation);
    }

    return S_OK;
}

HRESULT CModel::Ready_Meshes(ifstream& ifs, MODELTYPE eModelType)
{
    // 1. 메시 개수 읽기
    uint32_t meshCount;
    ifs.read((char*)&meshCount, sizeof(meshCount));

    m_iNumMeshes = meshCount;

    if (eModelType == MODELTYPE::NONANIM)
    {
        // 2. 메시 정보 구조체 배열 읽기
        vector<MeshInfoBin> meshInfos(meshCount);
        ifs.read((char*)meshInfos.data(), sizeof(MeshInfoBin) * m_iNumMeshes);

        // 3. 각 메시 생성
        for (uint32_t i = 0; i < m_iNumMeshes; ++i) {

            vector<VTXMESH> verts(meshInfos[i].NumVertices);
            ifs.read((char*)verts.data(), sizeof(VTXMESH) * meshInfos[i].NumVertices);

            vector<uint32_t> indices(meshInfos[i].NumIndices);
            ifs.read((char*)indices.data(), sizeof(uint32_t) * meshInfos[i].NumIndices);

            // **구조체와 데이터 한 번에 넘기기**
            CMesh* pMesh = CMesh::Create(m_pDevice, m_pContext, m_eModelType, meshInfos[i], verts, indices, m_Bones, XMLoadFloat4x4(&m_PreTransformMatrix));
            m_Meshes.push_back(pMesh);
        }
    
    }
    else
    {
        // ★ 애니메이션: 파일에 쓴 순서대로 "메시 단위"로 읽는다
        for (uint32_t i = 0; i < meshCount; ++i) {
            MeshInfoBin info{};
            ifs.read(reinterpret_cast<char*>(&info), sizeof(MeshInfoBin));

            // verts
            vector<VTXANIMMESH> verts(info.NumVertices);
            if (info.NumVertices)
                ifs.read(reinterpret_cast<char*>(verts.data()), sizeof(VTXANIMMESH) * info.NumVertices);

            // indices
            vector<uint32_t> indices(info.NumIndices);
            if (info.NumIndices)
                ifs.read(reinterpret_cast<char*>(indices.data()), sizeof(uint32_t) * info.NumIndices);

            // mesh bone slots
            uint32_t boneSlotCount = 0;
            ifs.read(reinterpret_cast<char*>(&boneSlotCount), sizeof(boneSlotCount));

            vector<MeshBoneRaw> meshBones(boneSlotCount);
            if (boneSlotCount)
                ifs.read(reinterpret_cast<char*>(meshBones.data()), sizeof(MeshBoneRaw) * boneSlotCount);

            // 생성
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
        CMeshMaterial* pMeshMaterial = CMeshMaterial::Create(m_pDevice, m_pContext, pModelFilePath, binMaterials[i]);
        if (!pMeshMaterial)
            return E_FAIL;
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
        if (!pBone)
            return E_FAIL;
        m_Bones.push_back(pBone);
    }
    return S_OK;
}

HRESULT CModel::Ready_Animations(std::ifstream& ifs)
{
    // 1. 애니메이션 개수 읽기
    uint32_t numAnims = 0;
    ifs.read((char*)&numAnims, sizeof(numAnims));
    m_iNumAnimations = numAnims;

    /*char buf[256];
    sprintf_s(buf, "[BIN 로드] 애니메이션 개수 = %d\n", numAnims);
    OutputDebugStringA(buf);*/
    
    // 2. 애니메이션 루프
    for (size_t i = 0; i < m_iNumAnimations; i++)
    {
        // AnimInfoBin 읽기
        AnimInfoBin animBin = {};
        ifs.read((char*)&animBin, sizeof(AnimInfoBin));

        //char buf[256];
        //sprintf_s(buf, "  [CModel] 애니[%zu] 이름='%s', 채널수=%d, 지속시간=%.2f\n", i, animBin.name, animBin.channelCount, animBin.duration);
        //OutputDebugStringA(buf);

        // Animation 생성
        CAnimation* pAnimation = CAnimation::Create(ifs,animBin, m_Bones);

        if (nullptr == pAnimation)
            return E_FAIL;

        m_Animations.push_back(pAnimation);
    }
    return S_OK;
}




CModel* CModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eModelType, FILETYPE eFileType, const _char* pModelFilePath, _fmatrix PreTransformMatrix)
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
