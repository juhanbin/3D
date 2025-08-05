#pragma once

#ifdef new
#undef new
#endif

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "../Default/framework.h"
#include <process.h>
#include <vector>
#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>

/* 공용 윈도우/엔진 전역 */
namespace Edit
{
    const unsigned int g_iWinSizeX = 1280;
    const unsigned int g_iWinSizeY = 720;

    enum class LEVEL { STATIC, LOADING, EDIT, END };
}
extern HWND g_hWnd;
extern HINSTANCE g_hInst;
using namespace Edit;

// ---- 오브젝트 타입 enum 및 변환 ----
enum class EObjectType : int
{
    MONSTER = 0,
    ROCK_AA,
    END
};

// --- 맵 오브젝트 구조체 (BIN 파일명 포함) ---
// "씬 에디터"에 배치/메타데이터 용도
#pragma pack(push,1)
struct MapObject
{
    int id;
    EObjectType type;
    float size[3];
    float rot[3];
    float pos[3];
    char fbxPath[260];
    char binPath[260];
};
#pragma pack(pop)

inline const char* ToObjectTypeString(EObjectType type)
{
    switch (type)
    {
    case EObjectType::MONSTER:  return "Monster";
    case EObjectType::ROCK_AA:  return "Rock_AA";
    default:                    return "Unknown";
    }
}
static constexpr int NumObjectTypes = static_cast<int>(EObjectType::END);

inline uint32_t max3(uint32_t a, uint32_t b, uint32_t c)
{
    return max(max(a, b), c);
}
enum class FILETYPE { FBX, BIN };

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
