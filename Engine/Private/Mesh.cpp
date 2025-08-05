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

	HRESULT			hr = MODELTYPE::NONANIM == eType ?
		Ready_Vertices_For_NonAnim(pAIMesh, PreTransformMatrix) :
		Ready_Vertices_For_Anim(pAIMesh, Bones);

	if (FAILED(hr))
		return E_FAIL;


	D3D11_BUFFER_DESC		IBDesc{};
	IBDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
	IBDesc.Usage = D3D11_USAGE_DEFAULT;
	IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IBDesc.CPUAccessFlags = 0;
	IBDesc.MiscFlags = 0;
	IBDesc.StructureByteStride = m_iIndexStride;

	_uint* pIndices = new _uint[m_iNumIndices];

	_uint	iNumIndices = {};

	for (size_t i = 0; i < pAIMesh->mNumFaces; i++)
	{
		aiFace AIFace = pAIMesh->mFaces[i];

		pIndices[iNumIndices++] = AIFace.mIndices[0];
		pIndices[iNumIndices++] = AIFace.mIndices[1];
		pIndices[iNumIndices++] = AIFace.mIndices[2];
	}

	D3D11_SUBRESOURCE_DATA	IBInitialData{};
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
	m_iNumIndices = meshInfos.NumFaces * 3;
	m_iIndexStride = 4;
	m_iNumVertexBuffers = 1;
	m_eIndexFormat = DXGI_FORMAT_R32_UINT;
	m_ePrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	Ready_Vertices_For_NonAnim(verts, PreTransformMatrix);

	D3D11_BUFFER_DESC		IBDesc{};
	IBDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
	IBDesc.Usage = D3D11_USAGE_DEFAULT;
	IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IBDesc.CPUAccessFlags = 0;
	IBDesc.MiscFlags = 0;
	IBDesc.StructureByteStride = m_iIndexStride;


	D3D11_SUBRESOURCE_DATA	IBInitialData{};
	IBInitialData.pSysMem = indices.data();

	if (FAILED(m_pDevice->CreateBuffer(&IBDesc, &IBInitialData, &m_pIB)))
		return E_FAIL;

	return S_OK;

}
HRESULT CMesh::Initialize_Prototype(MODELTYPE eType, MeshInfoBin& meshInfos, const std::vector<VTXANIMMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix)
{
	strcpy_s(m_szName, meshInfos.Name);

	m_iMaterialIndex = meshInfos.MaterialIndex;
	m_iNumVertices = meshInfos.NumVertices;
	m_iNumIndices = meshInfos.NumFaces * 3;
	m_iIndexStride = 4;
	m_iNumVertexBuffers = 1;
	m_eIndexFormat = DXGI_FORMAT_R32_UINT;
	m_ePrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


	Ready_Vertices_For_Anim(verts, Bones);

	D3D11_BUFFER_DESC		IBDesc{};
	IBDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
	IBDesc.Usage = D3D11_USAGE_DEFAULT;
	IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IBDesc.CPUAccessFlags = 0;
	IBDesc.MiscFlags = 0;
	IBDesc.StructureByteStride = m_iIndexStride;


	D3D11_SUBRESOURCE_DATA	IBInitialData{};
	IBInitialData.pSysMem = indices.data();

	if (FAILED(m_pDevice->CreateBuffer(&IBDesc, &IBInitialData, &m_pIB)))
		return E_FAIL;

	return S_OK;

}

HRESULT CMesh::Initialize(void* pArg)
{
	return S_OK;
}

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

	D3D11_BUFFER_DESC		VBDesc{};
	VBDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
	VBDesc.Usage = D3D11_USAGE_DEFAULT;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.CPUAccessFlags = 0;
	VBDesc.MiscFlags = 0;
	VBDesc.StructureByteStride = m_iVertexStride;

	VTXMESH* pVertices = new VTXMESH[m_iNumVertices];

	for (size_t i = 0; i < m_iNumVertices; i++)
	{
		memcpy(&pVertices[i].vPosition, &pAIMesh->mVertices[i], sizeof(_float3));
		XMStoreFloat3(&pVertices[i].vPosition, XMVector3TransformCoord(XMLoadFloat3(&pVertices[i].vPosition), PreTransformMatrix));

		memcpy(&pVertices[i].vNormal, &pAIMesh->mNormals[i], sizeof(_float3));
		XMStoreFloat3(&pVertices[i].vNormal, XMVector3TransformNormal(XMLoadFloat3(&pVertices[i].vNormal), PreTransformMatrix));

		memcpy(&pVertices[i].vTangent, &pAIMesh->mTangents[i], sizeof(_float3));
		memcpy(&pVertices[i].vBinormal, &pAIMesh->mBitangents[i], sizeof(_float3));

		memcpy(&pVertices[i].vTexcoord, &pAIMesh->mTextureCoords[0][i], sizeof(_float2));
	}

	D3D11_SUBRESOURCE_DATA	VBInitialData{};
	VBInitialData.pSysMem = pVertices;

	if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
		return E_FAIL;

	Safe_Delete_Array(pVertices);

	return S_OK;
}

HRESULT CMesh::Ready_Vertices_For_NonAnim(const vector<VTXMESH>& verts, _fmatrix PreTransformMatrix)
{
	m_iVertexStride = sizeof(VTXMESH);

	// 변환된 정점 버퍼 준비
	vector<VTXMESH> transformedVerts = verts; // 복사

	for (auto& v : transformedVerts)
	{
		// 위치에 변환 (예: 스케일, 회전, 이동)
		XMStoreFloat3(&v.vPosition, XMVector3TransformCoord(XMLoadFloat3(&v.vPosition), PreTransformMatrix));

		// 노멀, 탄젠트, 바이노멀도 변환해야 함 (회전/스케일만, 이동X)
		XMStoreFloat3(&v.vNormal, XMVector3TransformNormal(XMLoadFloat3(&v.vNormal), PreTransformMatrix));
		XMStoreFloat3(&v.vTangent, XMVector3TransformNormal(XMLoadFloat3(&v.vTangent), PreTransformMatrix));
		XMStoreFloat3(&v.vBinormal, XMVector3TransformNormal(XMLoadFloat3(&v.vBinormal), PreTransformMatrix));
		// UV는 변환X
	}

	D3D11_BUFFER_DESC VBDesc{};
	VBDesc.ByteWidth = static_cast<UINT>(transformedVerts.size() * sizeof(VTXMESH));
	VBDesc.Usage = D3D11_USAGE_DEFAULT;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.CPUAccessFlags = 0;
	VBDesc.MiscFlags = 0;
	VBDesc.StructureByteStride = m_iVertexStride;

	D3D11_SUBRESOURCE_DATA VBInitialData{};
	VBInitialData.pSysMem = transformedVerts.data();

	if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
		return E_FAIL;

	m_vecPositions.clear();
	for (const auto& v : transformedVerts)
		m_vecPositions.push_back(v.vPosition);

	return S_OK;
}


HRESULT CMesh::Ready_Vertices_For_Anim(const aiMesh* pAIMesh, const vector<CBone*>& Bones)
{
	m_iVertexStride = sizeof(VTXANIMMESH);

	D3D11_BUFFER_DESC		VBDesc{};
	VBDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
	VBDesc.Usage = D3D11_USAGE_DEFAULT;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.CPUAccessFlags = 0;
	VBDesc.MiscFlags = 0;
	VBDesc.StructureByteStride = m_iVertexStride;

	VTXANIMMESH* pVertices = new VTXANIMMESH[m_iNumVertices];
	ZeroMemory(pVertices, sizeof(VTXANIMMESH) * m_iNumVertices);

	for (size_t i = 0; i < m_iNumVertices; i++)
	{
		memcpy(&pVertices[i].vPosition, &pAIMesh->mVertices[i], sizeof(_float3));
		memcpy(&pVertices[i].vNormal, &pAIMesh->mNormals[i], sizeof(_float3));
		memcpy(&pVertices[i].vTangent, &pAIMesh->mTangents[i], sizeof(_float3));
		memcpy(&pVertices[i].vBinormal, &pAIMesh->mBitangents[i], sizeof(_float3));
		memcpy(&pVertices[i].vTexcoord, &pAIMesh->mTextureCoords[0][i], sizeof(_float2));
	}

	m_iNumBones = pAIMesh->mNumBones;

	/*이 메시에 영향을 주는 뼈들을 하나씩 순회한다. */
	for (size_t i = 0; i < m_iNumBones; i++)
	{
		/* i번째 뼈가 영향을 주는 정점의 갯수 */
		aiBone* pAIBone = pAIMesh->mBones[i];

		_float4x4	OffsetMatrix;

		memcpy(&OffsetMatrix, &pAIBone->mOffsetMatrix, sizeof(_float4x4));

		XMStoreFloat4x4(&OffsetMatrix, XMMatrixTranspose(XMLoadFloat4x4(&OffsetMatrix)));

		m_OffsetMatrices.push_back(OffsetMatrix);

		_uint	iBoneIndex = { 0 };

		auto	iter = find_if(Bones.begin(), Bones.end(), [&](CBone* pBone)->_bool
			{
				if (true == pBone->Compare_Name(pAIBone->mName.data))
					return true;

				iBoneIndex++;

				return false;
			});

		m_BoneIndices.push_back(iBoneIndex);

		for (size_t j = 0; j < pAIBone->mNumWeights; j++)
		{
			aiVertexWeight	AIVertexWeight = pAIBone->mWeights[j];

			/* i번째 뼈가 영향을 주는 j번째 정점의 정점버퍼상의 인덱스 */
			if (0.f == pVertices[AIVertexWeight.mVertexId].vBlendWeight.x)
			{
				pVertices[AIVertexWeight.mVertexId].vBlendIndex.x = i;
				pVertices[AIVertexWeight.mVertexId].vBlendWeight.x = AIVertexWeight.mWeight;
			}

			else if (0.f == pVertices[AIVertexWeight.mVertexId].vBlendWeight.y)
			{
				pVertices[AIVertexWeight.mVertexId].vBlendIndex.y = i;
				pVertices[AIVertexWeight.mVertexId].vBlendWeight.y = AIVertexWeight.mWeight;
			}
			else if (0.f == pVertices[AIVertexWeight.mVertexId].vBlendWeight.z)
			{
				pVertices[AIVertexWeight.mVertexId].vBlendIndex.z = i;
				pVertices[AIVertexWeight.mVertexId].vBlendWeight.z = AIVertexWeight.mWeight;
			}

			else
			{
				pVertices[AIVertexWeight.mVertexId].vBlendIndex.w = i;
				pVertices[AIVertexWeight.mVertexId].vBlendWeight.w = AIVertexWeight.mWeight;
			}
		}
	}

	if (0 == m_iNumBones)
	{
		m_iNumBones = 1;

		_uint	iBoneIndex = { 0 };

		auto	iter = find_if(Bones.begin(), Bones.end(), [&](CBone* pBone)->_bool
			{
				if (true == pBone->Compare_Name(m_szName))
					return true;

				iBoneIndex++;

				return false;
			});

		m_BoneIndices.push_back(iBoneIndex);

		_float4x4		OffsetMatrix;
		XMStoreFloat4x4(&OffsetMatrix, XMMatrixIdentity());

		m_OffsetMatrices.push_back(OffsetMatrix);
	}

	D3D11_SUBRESOURCE_DATA	VBInitialData{};
	VBInitialData.pSysMem = pVertices;

	if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
		return E_FAIL;

	Safe_Delete_Array(pVertices);

	return S_OK;
}

HRESULT CMesh::Ready_Vertices_For_Anim(const std::vector<VTXANIMMESH>& verts, const std::vector<CBone*>& Bones)
{
	m_iVertexStride = sizeof(VTXANIMMESH);
	m_iNumVertices = static_cast<UINT>(verts.size());

	// 1. 정점 버퍼 생성
	D3D11_BUFFER_DESC VBDesc{};
	VBDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
	VBDesc.Usage = D3D11_USAGE_DEFAULT;
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.CPUAccessFlags = 0;
	VBDesc.MiscFlags = 0;
	VBDesc.StructureByteStride = m_iVertexStride;

	D3D11_SUBRESOURCE_DATA VBInitialData{};
	VBInitialData.pSysMem = verts.data();

	if (FAILED(m_pDevice->CreateBuffer(&VBDesc, &VBInitialData, &m_pVB)))
		return E_FAIL;

	// 2. 뼈 오프셋 행렬, 인덱스 세팅
	m_OffsetMatrices.clear();
	m_BoneIndices.clear();

	// BIN에서는 CBone에 오프셋 행렬(m_OffsetMatrix) 저장되어 있음
	m_iNumBones = static_cast<UINT>(Bones.size());
	for (size_t i = 0; i < Bones.size(); ++i)
	{
		// 각 본의 오프셋 행렬 가져오기
		m_OffsetMatrices.push_back(Bones[i]->Get_OffsetMatrix());
		m_BoneIndices.push_back(static_cast<_uint>(i));
	}

	// 3. 포지션 정보 복사
	m_vecPositions.clear();
	for (const auto& v : verts)
		m_vecPositions.push_back(v.vPosition);

	for (size_t i = 0; i < std::min<size_t>(10, verts.size()); ++i) {
		const auto& v = verts[i];
		
	}

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

CMesh* CMesh::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODELTYPE eType, MeshInfoBin& meshInfos, const vector<VTXANIMMESH>& verts, const vector<uint32_t>& indices, const vector<CBone*>& Bones, _fmatrix PreTransformMatrix)
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


}
