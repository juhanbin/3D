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

struct BoneInfoBin {
    char name[64];
    int parentIdx;
    float offset[16];
    float transform[16];
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
