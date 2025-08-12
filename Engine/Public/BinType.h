#pragma once

#include <cstdint>

#pragma pack(push, 1)

struct MeshInfoBin {
    char Name[64];              // 메시 이름
    uint32_t MaterialIndex;     // 머티리얼 인덱스
    uint32_t NumVertices;       // 버텍스 수
    uint32_t NumIndices;        //인덱스 수
    uint32_t NumFaces;          //폴리곤의 수
};

struct MeshBoneRaw {
    char  Name[64];
    float Offset[16];   // aiBone::mOffsetMatrix (row-major로 저장 규약 고정)
    int   GlobalIndex;  // GatherBones()로 만든 m_Bones[]의 전역 인덱스. 없으면 -1
};

enum class TextureType : int { DIFFUSE, NORMAL };

struct TextureSlotBin {
    int type;            // TextureType (int로 저장)
    char path[260];
};

struct MaterialInfoBin2 {
    TextureSlotBin textures[8];  // 여러개 저장 가능
    int numTextures;
};

struct MaterialInfoBin {
    char basecolor[260];        //Diffuse 경로
    char normal[260];           //nomal맵 경로
};

struct BoneInfoBin
{
    char   Name[64];        // 본의 이름 (문자열, 최대 63자 + '\0')
    int    ParentIndex;     // 부모 본 인덱스 (-1이면 루트)
    float Transform[16];   // 4x4 트랜스폼 행렬 (float[16], row-major 권장)
};

struct ChannelInfoBin {
    char boneName[64];       // 이 채널이 적용되는 본 이름
    uint32_t keyframeCount;  // 이 채널의 키프레임 개수
};

struct AnimInfoBin {
    char name[64];           // 애니메이션 이름
    double duration;         // 전체 길이
    double ticksPerSecond;   // 1초당 tick 수(샘플레이트)
    uint32_t channelCount;   // 채널(뼈) 개수
};
#pragma pack(pop)
