#include "Mesh.h"
#include "Bone.h"
#include "Shader.h"

CMesh::CMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CVIBuffer{ pDevice, pContext }
{
}

CMesh::CMesh(const CMesh& Prototype)
    : CVIBuffer{ Prototype }
{
}

HRESULT CMesh::Initialize_Prototype(MODELTYPE eType, const aiMesh* pAIMesh, const vector<class CBone*>& Bones, _fmatrix PreTransformMatrix)
{
    strcpy_s(m_szName, pAIMesh->mName.data);

    m_iMaterialIndex = pAIMesh->mMaterialIndex;
    m_iNumVertices = pAIMesh->mNumVertices;
    m_iNumIndices = pAIMesh->mNumFaces * 3;
    m_iIndexStride = 4;
    m_iNumVertexBuffers = 1;
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    HRESULT hr = (MODELTYPE::NONANIM == eType) ?
        Ready_Vertices_For_NonAnim(pAIMesh, PreTransformMatrix) :
        Ready_Vertices_For_Anim(pAIMesh, Bones);

    if (FAILED(hr)) return E_FAIL;

    // 인덱스 버퍼 생성 + ★ CPU 보관
    D3D11_BUFFER_DESC IBDesc{};
    IBDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
    IBDesc.Usage = D3D11_USAGE_DEFAULT;
    IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IBDesc.StructureByteStride = m_iIndexStride;

    _uint* pIndices = new _uint[m_iNumIndices];
    _uint iWrite = 0;
    m_vecIndices.clear();
    m_vecIndices.reserve(m_iNumIndices);

    for (size_t i = 0; i < pAIMesh->mNumFaces; i++)
    {
        aiFace AIFace = pAIMesh->mFaces[i];
        pIndices[iWrite] = AIFace.mIndices[0]; m_vecIndices.push_back(pIndices[iWrite++]);
        pIndices[iWrite] = AIFace.mIndices[1]; m_vecIndices.push_back(pIndices[iWrite++]);
        pIndices[iWrite] = AIFace.mIndices[2]; m_vecIndices.push_back(pIndices[iWrite++]);
    }

    D3D11_SUBRESOURCE_DATA IBInitialData{};
    IBInitialData.pSysMem = pIndices;

    if (FAILED(m_pDevice->CreateBuffer(&IBDesc, &IBInitialData, &m_pIB)))
        return E_FAIL;

    Safe_Delete_Array(pIndices);
    return S_OK;
}

HRESULT CMesh::Initialize_Prototype(MODELTYPE eType, MeshInfoBin& meshInfos, const std::vector<VTXMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix)
{
    strcpy_s(m_szName, meshInfos.Name);

    m_iMaterialIndex = meshInfos.MaterialIndex;
    m_iNumVertices = meshInfos.NumVertices;
    m_iNumIndices = meshInfos.NumIndices;
    m_iIndexStride = 4;
    m_iNumVertexBuffers = 1;
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // 정점 버퍼(PreTransform 적용) + ★ CPU 포지션 저장
    if (FAILED(Ready_Vertices_For_NonAnim(verts, PreTransformMatrix)))
        return E_FAIL;

    // 인덱스 버퍼 + ★ CPU 인덱스 저장
    {
        D3D11_BUFFER_DESC IBDesc{};
        IBDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
        IBDesc.Usage = D3D11_USAGE_DEFAULT;
        IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        IBDesc.StructureByteStride = m_iIndexStride;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = indices.data();

        if (FAILED(m_pDevice->CreateBuffer(&IBDesc, &init, &m_pIB)))
            return E_FAIL;

        m_vecIndices = indices; // ★ CPU 보관
    }

    return S_OK;
}

HRESULT CMesh::Initialize_Prototype(MODELTYPE eType, MeshInfoBin& meshInfo,
    const std::vector<VTXANIMMESH>& verts, const vector<uint32_t>& indices,
    const std::vector<MeshBoneRaw>& meshBones,
    const vector<CBone*>& Bones, _fmatrix /*PreTransformMatrix*/)
{
    strcpy_s(m_szName, meshInfo.Name);

    m_iMaterialIndex = meshInfo.MaterialIndex;
    m_iNumVertices = meshInfo.NumVertices;
    m_iNumIndices = meshInfo.NumIndices;
    m_iIndexStride = 4;
    m_iNumVertexBuffers = 1;
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // 정점 버퍼(애니 정점) 생성
    {
        m_iVertexStride = sizeof(VTXANIMMESH);

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = verts.data();

        if (FAILED(m_pDevice->CreateBuffer(&vbDesc, &init, &m_pVB)))
            return E_FAIL;

        // ★ CPU 포지션 보관(로컬)
        m_vecPositions.clear();
        m_vecPositions.reserve(verts.size());
        for (const auto& v : verts) m_vecPositions.push_back(v.vPosition);
    }

    // 인덱스 버퍼
    {
        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = indices.data();

        if (FAILED(m_pDevice->CreateBuffer(&ibDesc, &init, &m_pIB)))
            return E_FAIL;

        m_vecIndices = indices; // ★ CPU 보관
    }

    // 팔레트(전역 인덱스 + 오프셋)
    m_iNumBones = static_cast<_uint>(meshBones.size());
    m_BoneIndices.reserve(meshBones.size());
    m_OffsetMatrices.reserve(meshBones.size());

    for (const auto& mb : meshBones)
    {
        _uint globalIdx = (mb.GlobalIndex >= 0) ? static_cast<_uint>(mb.GlobalIndex) : 0;
        m_BoneIndices.push_back(globalIdx);

        _float4x4 off{};
        memcpy(&off, mb.Offset, sizeof(float) * 16);
        XMStoreFloat4x4(&off, XMMatrixTranspose(XMLoadFloat4x4(&off))); // 행렬 전치
        m_OffsetMatrices.push_back(off);
    }

    if (m_iNumBones == 0) {
        _float4x4 I; XMStoreFloat4x4(&I, XMMatrixIdentity());
        m_OffsetMatrices.push_back(I);
        m_BoneIndices.push_back(0);
        m_iNumBones = 1;
    }

    return S_OK;
}

HRESULT CMesh::Initialize(void* pArg) { return S_OK; }

HRESULT CMesh::Bind_BoneMatrices(CShader* pShader, const _char* pConstantName, const vector<class CBone*>& Bones)
{
    for (size_t i = 0; i < m_iNumBones; i++)
    {
        XMStoreFloat4x4(&m_BoneMatrices[i],
            XMLoadFloat4x4(&m_OffsetMatrices[i]) * Bones[m_BoneIndices[i]]->Get_CombinedTransformationMatrix());
    }
    return pShader->Bind_Matrices(pConstantName, m_BoneMatrices, m_iNumBones);
}

HRESULT CMesh::Ready_Vertices_For_NonAnim(const aiMesh* pAIMesh, _fmatrix PreTransformMatrix)
{
    m_iVertexStride = sizeof(VTXMESH);

    D3D11_BUFFER_DESC VBDesc{};
    VBDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
    VBDesc.Usage = D3D11_USAGE_DEFAULT;
    VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VBDesc.StructureByteStride = m_iVertexStride;

    VTXMESH* pVertices = new VTXMESH[m_iNumVertices];

    m_vecPositions.clear();
    m_vecPositions.reserve(m_iNumVertices);

    for (size_t i = 0; i < m_iNumVertices; i++)
    {
        memcpy(&pVertices[i].vPosition, &pAIMesh->mVertices[i], sizeof(_float3));
        XMStoreFloat3(&pVertices[i].vPosition, XMVector3TransformCoord(XMLoadFloat3(&pVertices[i].vPosition), PreTransformMatrix));

        // ★ CPU 포지션 보관(이미 PreTransform 적용된 로컬좌표)
        m_vecPositions.push_back(pVertices[i].vPosition);

        memcpy(&pVertices[i].vNormal, &pAIMesh->mNormals[i], sizeof(_float3));
        XMStoreFloat3(&pVertices[i].vNormal, XMVector3TransformNormal(XMLoadFloat3(&pVertices[i].vNormal), PreTransformMatrix));

        memcpy(&pVertices[i].vTangent, &pAIMesh->mTangents[i], sizeof(_float3));
        memcpy(&pVertices[i].vBinormal, &pAIMesh->mBitangents[i], sizeof(_float3));

        memcpy(&pVertices[i].vTexcoord, &pAIMesh->mTextureCoords[0][i], sizeof(_float2));
    }

    D3D11_SUBRESOURCE_DATA VBInitialData{};
    VBInitialData.pSysMem = pVertices;

    if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
        return E_FAIL;

    Safe_Delete_Array(pVertices);
    return S_OK;
}

HRESULT CMesh::Ready_Vertices_For_NonAnim(const vector<VTXMESH>& verts, _fmatrix PreTransformMatrix)
{
    m_iVertexStride = sizeof(VTXMESH);

    vector<VTXMESH> transformedVerts = verts; // 복사

    for (auto& v : transformedVerts)
    {
        XMStoreFloat3(&v.vPosition, XMVector3TransformCoord(XMLoadFloat3(&v.vPosition), PreTransformMatrix));
        XMStoreFloat3(&v.vNormal, XMVector3TransformNormal(XMLoadFloat3(&v.vNormal), PreTransformMatrix));
        XMStoreFloat3(&v.vTangent, XMVector3TransformNormal(XMLoadFloat3(&v.vTangent), PreTransformMatrix));
        XMStoreFloat3(&v.vBinormal, XMVector3TransformNormal(XMLoadFloat3(&v.vBinormal), PreTransformMatrix));
    }

    D3D11_BUFFER_DESC VBDesc{};
    VBDesc.ByteWidth = static_cast<UINT>(transformedVerts.size() * sizeof(VTXMESH));
    VBDesc.Usage = D3D11_USAGE_DEFAULT;
    VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VBDesc.StructureByteStride = m_iVertexStride;

    D3D11_SUBRESOURCE_DATA VBInitialData{};
    VBInitialData.pSysMem = transformedVerts.data();

    if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
        return E_FAIL;

    // ★ CPU 포지션 보관
    m_vecPositions.clear();
    m_vecPositions.reserve(transformedVerts.size());
    for (const auto& v : transformedVerts) m_vecPositions.push_back(v.vPosition);

    return S_OK;
}

HRESULT CMesh::Ready_Vertices_For_Anim(const aiMesh* pAIMesh, const vector<CBone*>& Bones)
{
    m_iVertexStride = sizeof(VTXANIMMESH);

    D3D11_BUFFER_DESC VBDesc{};
    VBDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
    VBDesc.Usage = D3D11_USAGE_DEFAULT;
    VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VBDesc.StructureByteStride = m_iVertexStride;

    VTXANIMMESH* pVertices = new VTXANIMMESH[m_iNumVertices];
    ZeroMemory(pVertices, sizeof(VTXANIMMESH) * m_iNumVertices);

    m_vecPositions.clear();
    m_vecPositions.reserve(m_iNumVertices);

    for (size_t i = 0; i < m_iNumVertices; i++)
    {
        memcpy(&pVertices[i].vPosition, &pAIMesh->mVertices[i], sizeof(_float3));
        memcpy(&pVertices[i].vNormal, &pAIMesh->mNormals[i], sizeof(_float3));
        memcpy(&pVertices[i].vTangent, &pAIMesh->mTangents[i], sizeof(_float3));
        memcpy(&pVertices[i].vBinormal, &pAIMesh->mBitangents[i], sizeof(_float3));
        memcpy(&pVertices[i].vTexcoord, &pAIMesh->mTextureCoords[0][i], sizeof(_float2));

        // ★ CPU 포지션 보관(로컬)
        m_vecPositions.push_back(pVertices[i].vPosition);
    }

    m_iNumBones = pAIMesh->mNumBones;

    for (size_t i = 0; i < m_iNumBones; i++)
    {
        aiBone* pAIBone = pAIMesh->mBones[i];

        _float4x4 OffsetMatrix;
        memcpy(&OffsetMatrix, &pAIBone->mOffsetMatrix, sizeof(_float4x4));
        XMStoreFloat4x4(&OffsetMatrix, XMMatrixTranspose(XMLoadFloat4x4(&OffsetMatrix)));
        m_OffsetMatrices.push_back(OffsetMatrix);

        _uint iBoneIndex = { 0 };
        auto  iter = find_if(Bones.begin(), Bones.end(), [&](CBone* pBone)->_bool {
            if (pBone->Compare_Name(pAIBone->mName.data)) return true;
            iBoneIndex++; return false;
            });

        m_BoneIndices.push_back(iBoneIndex);

        for (size_t j = 0; j < pAIBone->mNumWeights; j++)
        {
            aiVertexWeight w = pAIBone->mWeights[j];
            if (0.f == pVertices[w.mVertexId].vBlendWeight.x) { pVertices[w.mVertexId].vBlendIndex.x = (UINT)i; pVertices[w.mVertexId].vBlendWeight.x = w.mWeight; }
            else if (0.f == pVertices[w.mVertexId].vBlendWeight.y) { pVertices[w.mVertexId].vBlendIndex.y = (UINT)i; pVertices[w.mVertexId].vBlendWeight.y = w.mWeight; }
            else if (0.f == pVertices[w.mVertexId].vBlendWeight.z) { pVertices[w.mVertexId].vBlendIndex.z = (UINT)i; pVertices[w.mVertexId].vBlendWeight.z = w.mWeight; }
            else { pVertices[w.mVertexId].vBlendIndex.w = (UINT)i; pVertices[w.mVertexId].vBlendWeight.w = w.mWeight; }
        }
    }

    if (0 == m_iNumBones)
    {
        m_iNumBones = 1;

        _uint iBoneIndex = { 0 };
        auto  iter = find_if(Bones.begin(), Bones.end(), [&](CBone* pBone)->_bool {
            if (pBone->Compare_Name(m_szName)) return true;
            iBoneIndex++; return false;
            });

        m_BoneIndices.push_back(iBoneIndex);

        _float4x4 I;
        XMStoreFloat4x4(&I, XMMatrixIdentity());
        m_OffsetMatrices.push_back(I);
    }

    D3D11_SUBRESOURCE_DATA VBInitialData{};
    VBInitialData.pSysMem = pVertices;

    if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
        return E_FAIL;

    Safe_Delete_Array(pVertices);
    return S_OK;
}

HRESULT CMesh::Ready_Vertices_For_Anim(const std::vector<VTXANIMMESH>& verts, const std::vector<CBone*>& Bones)
{
    m_iVertexStride = sizeof(VTXANIMMESH);
    m_iNumVertices = static_cast<uint32_t>(verts.size());

    D3D11_BUFFER_DESC VBDesc{};
    VBDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
    VBDesc.Usage = D3D11_USAGE_DEFAULT;
    VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VBDesc.StructureByteStride = m_iVertexStride;

    D3D11_SUBRESOURCE_DATA VBInitialData{};
    VBInitialData.pSysMem = verts.data();

    if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
        return E_FAIL;

    m_OffsetMatrices.clear();
    m_BoneIndices.clear();

    m_iNumBones = static_cast<UINT>(Bones.size());
    for (size_t i = 0; i < Bones.size(); ++i)
    {
        m_OffsetMatrices.push_back(Bones[i]->Get_OffsetMatrix());
        m_BoneIndices.push_back(static_cast<_uint>(i));
    }

    m_vecPositions.clear();
    m_vecPositions.reserve(verts.size());
    for (const auto& v : verts) m_vecPositions.push_back(v.vPosition);

    return S_OK;
}

CMesh* CMesh::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eType, const aiMesh* pAIMesh, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix)
{
    CMesh* pInstance = new CMesh(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(eType, pAIMesh, Bones, PreTransformMatrix)))
    {
        MSG_BOX(TEXT("Failed to Created : CMesh"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

CMesh* CMesh::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eType, MeshInfoBin& meshInfos, const vector<VTXMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix)
{
    CMesh* pInstance = new CMesh(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(eType, meshInfos, verts, indices, Bones, PreTransformMatrix)))
    {
        MSG_BOX(TEXT("Failed to Created : CMesh(Bin)"));
        Safe_Release(pInstance);
        return nullptr;
    }
    return pInstance;
}

CMesh* CMesh::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
    MODELTYPE eType, MeshInfoBin& meshInfo,
    const vector<VTXANIMMESH>& verts, const vector<uint32_t>& indices,
    const vector<MeshBoneRaw>& meshBones,
    const vector<CBone*>& Bones, _fmatrix PreTransformMatrix)
{
    CMesh* pInstance = new CMesh(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype(eType, meshInfo, verts, indices, meshBones, Bones, PreTransformMatrix)))
    {
        MSG_BOX(TEXT("Failed to Created : CMesh(Bin, Anim)"));
        Safe_Release(pInstance);
        return nullptr;
    }
    return pInstance;
}

CComponent* CMesh::Clone(void* pArg)
{
    CMesh* pInstance = new CMesh(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX(TEXT("Failed to Cloned : CMesh"));
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CMesh::Free()
{
    __super::Free();
    // (GPU 버퍼는 CVIBuffer가 해제)
}
