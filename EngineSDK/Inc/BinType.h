#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct SimpleVertex {
    float pos[3];
    float normal[3];
    float uv[2];
    int   blendIndex[4];
    float blendWeight[4];
};

struct MaterialInfo {
    char basecolor[260];
    char normal[260];
    char arm[260];
};

struct BoneInfo {
    char name[64];
    int parentIdx;
    float offset[16];
    float transform[16];
};

struct KeyFrame {
    double time;
    float scale[3];
    float rotation[4];
    float translation[3];
};

struct ChannelInfo {
    char boneName[64];
    uint32_t keyframeCount;
};

struct AnimInfo {
    char name[64];
    double duration;
    double ticksPerSecond;
    uint32_t channelCount;
};
#pragma pack(pop)


static_assert(sizeof(AnimInfo) == 84, "AnimInfo size mismatch!");
static_assert(sizeof(ChannelInfo) == 68, "ChannelInfo size mismatch!");
static_assert(sizeof(KeyFrame) == 48, "KeyFrame size mismatch!");
