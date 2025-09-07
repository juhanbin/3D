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
    HERO,
    SPEAR,
    SPEAR_STATIC,
    MONSTER_SPEAR,
    MONSTER_BOW,
    BRIDGE,
    CAVE,
    SKELETON_SPEAR,
    SKELETON_BOW,
    BOSS_EYE_MID,
    BOSS_EYE_TOP,
    BOSS_FIRE,
    BOSS_HAND_L,
    BOSS_HAND_R,
    BOSS_MASK,
    EYESPAWNER,
    MONSTER_EYE,
    MUSHROOM,
    SMALLMUSHROOM,
    PARASIT_EYE,
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
    case EObjectType::MONSTER:          return "Monster";
    case EObjectType::ROCK_AA:          return "Rock_AA";
    case EObjectType::HERO:             return "Hero";
    case EObjectType::SPEAR:            return "Spear";
    case EObjectType::SPEAR_STATIC:     return "Spear_Static";
    case EObjectType::MONSTER_SPEAR:    return "Monster_Spear";
    case EObjectType::MONSTER_BOW:      return "Monster_Bow";
    case EObjectType::BRIDGE:           return "Bridge";
    case EObjectType::CAVE:             return "Cave";
    case EObjectType::SKELETON_SPEAR:   return "Skeleton_Spear";
    case EObjectType::SKELETON_BOW:     return "Skeleton_Bow";
    case EObjectType::BOSS_EYE_MID:     return "Boss_Eye_Mid";
    case EObjectType::BOSS_EYE_TOP:     return "Boss_Eye_Top";
    case EObjectType::BOSS_FIRE:        return "Boss_Fire";
    case EObjectType::BOSS_HAND_L:      return "Boss_Hand_L";
    case EObjectType::BOSS_HAND_R:      return "Boss_Hand_R";
    case EObjectType::BOSS_MASK:        return "Boss_Mask";
    case EObjectType::EYESPAWNER:       return "EyeSpawner";
    case EObjectType::MONSTER_EYE:      return "Monster_Eye";
    case EObjectType::MUSHROOM:         return "Mushroom";
    case EObjectType::SMALLMUSHROOM:    return "SmallMushroom";
    case EObjectType::PARASIT_EYE:      return "Parasit_Eye";
    default:                            return "Unknown";

    }
}
static constexpr int NumObjectTypes = static_cast<int>(EObjectType::END);

inline uint32_t max3(uint32_t a, uint32_t b, uint32_t c)
{
    return max(max(a, b), c);
}
enum class FILETYPE { FBX, BIN };
