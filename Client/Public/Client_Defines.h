#pragma once

#include "../Default/framework.h"
#include <process.h>

/* 클라이언트에서 사용할 수 있는 공통적인 정의를 모아놓은 파일 */
namespace Client
{
	const unsigned int			g_iWinSizeX = 1280;
	const unsigned int			g_iWinSizeY = 720;

	enum class LEVEL { STATIC, LOADING, LOGO,GAMEPLAY, INTRO,BRIDGE,BOSS, END };
	enum  class FILETYPE { FBX, BIN };

	enum class MOVING {IDLE,JOG,RUN,DASH};
	enum class ATTACK { NONE ,ENTER, IDLE, THROW, FRONT, BACK, RIGHT, LEFT, GROUND };

	enum class MONSTER { SPEARE_IDLE, WALK, SPEARE_ATTACK, Bow_IDLE, BOW_ATTACK ,HIT};

    enum class EObjectType : int
    {
        MONSTER = 0,
        ROCK_AA = 1,
        HERO = 2,
        SPEAR = 3,
        SPEAR_STATIC = 4,
        MONSTER_SPEAR = 5,
        MONSTER_BOW = 6,
        BRIDGE = 7,
        CAVE = 8,
        SKELETON_SPEAR = 9,
        SKELETON_BOW = 10,
        BOSS_EYE_MID = 11,
        BOSS_EYE_TOP = 12,
        BOSS_FIRE = 13,
        BOSS_HAND_L = 14,
        BOSS_HAND_R = 15,
        BOSS_MASK = 16,
        EYESPAWNER = 17,
        MONSTER_EYE = 18,
        MUSHROOM = 19,
        SMALLMUSHROOM = 20,
        PARASIT_EYE = 21,
        MONSTER_ARROW = 22,
        BIG_DOOR = 23,
        CELLING_AA = 24,
        CIRCLE = 25,
        CUBE_004 = 26,
        DOORFRAME = 27,
        DOORWALL_AA = 28,
        HANGINGROPES = 29,
        MOD_STARTCAP = 30,
        MOD_STARTRING_AA = 31,
        PEDSTAL = 32,
        PILLAR = 33,
        PRISON = 34,
        PROSON_DOOR = 35,
        VAULTED_ARCH = 36,
        VAULTED_WALL_AA = 37,
        WINDOW_AA = 38,
        MOD_VAULTED_STAIR_AA = 39,
        MOD_VAULTED_BROKEN_AA = 40,
        MOD_VAULTED_BROKEN_AB = 41,
        MOD_VAULTED_CORRIDOR_AA = 42,
        BLA_BRIDGE_ENTRANCE_02 = 43,
        MOD_BOSSROOM_CEILING_AA = 44,
        MOD_BOSSROOM_GROUND_AA = 45,
        MOD_BOSSROOM_GROUNDFENCE_AA = 46,
        MOD_BOSSROOM_GROUNDFENCE_AB = 47,
        MOD_BOSSROOM_GROUNDFENCE_AC = 48,
        MOD_BOSSROOM_PILLAR_AA = 49,
        MOD_BOSSROOM_PILLAR_AB = 50,
        MOD_BOSSROOM_WALL_AA = 51,
        MOD_BOSSROOM_WALL_AB = 52,
        MOD_BOSSROOM_WALL_AC = 53,
        PROP_BOSSAUTEL_AA = 54,
        SHADOWMASK_BOSSROOM = 55,
        HALFBOSSROOM = 56,
        HALDBOSSROOM_V2 = 57,
        HALFBOSSROOM_HIGHER = 58,
        MOD_GUARDRAIL_AB = 59,
        MOD_BRIDGEARCH_AA = 60,
        MOD_BRIDGEBROKEN_AA = 61,
        MOD_BRIDGEBROKEN_AB = 62,
        MOD_BRIDGEGROUND_AA = 63,
        MOD_BRIDGEGROUND_BROKENEND_AA = 64,
        MOD_GROUND_BRIDGE_START_AA = 65,
        SM_MERGED_MOD_STAIRASSMBLY_AA = 66,
        MOD_SCAFOLD_AA = 67,
        MOD_CUTSTONE_WALL_AA = 68,
        MOD_STONEWALL_AA = 69,
        MOD_STONESTAIRS_AA = 70,
        SM_MERGED_MOD_THREEDEFENSES = 71,
        MOD_DEFENSE_AA = 72,
        MOD_HUT_AA = 73,
        PROP_PILIAR_AB = 74,
        DOOR = 75,
        END = 76
    };


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
}

extern HWND g_hWnd;
extern HINSTANCE g_hInst;
using namespace Client;

/* 조명. */

/* 1. 똥싼놈(남). */
/* 1. 피해의식갑.(여) */
/* 1. 추천빌런.(여) */
/* 1. 여미새, 남미새 */
/* 1. 변태범죄자새끼 1, 2 .(남) */
/* 1. 야겜만든중2병.(남) */
/* 1. 착한쁘락지.(남) */
/* 1. 여왕벌과 아이들(남들) */
/* 1. 게이.(남) */
/* 1. 코스프레.(남, 여) */
/* 1. 어소트락. */
/* 1. 서울게임아카데미 */
/* 1. 쥬신의 과거 */
/* 1. 쥬신의 탈피. (펜션, 애견 놀이터, 애견 카페, ) */
