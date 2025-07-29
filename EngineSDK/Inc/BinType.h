#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct SimpleVertexBin {
    float pos[3];
    float normal[3];
    float uv[2];
    int   blendIndex[4];
    float blendWeight[4];
};

struct MaterialInfoBin {
    char basecolor[260];
    char normal[260];
    char arm[260];
};

struct BoneInfoBin {
    char name[64];
    int parentIdx;
    float offset[16];
    float transform[16];
};

struct KeyFrameBin {
    double time;
    float scale[3];
    float rotation[4];
    float translation[3];
};

struct ChannelInfoBin {
    char boneName[64];
    uint32_t keyframeCount;
};

struct AnimInfoBin {
    char name[64];
    double duration;
    double ticksPerSecond;
    uint32_t channelCount;
};
#pragma pack(pop)
