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
        MONSTER_ARROW,
        BIG_DOOR,
        CELLING_AA,
        CIRCLE,
        CUBE_004,
        DOORFRAME,
        DOORWALL_AA,
        HANGINGROPES,
        MOD_STARTCAP,
        MOD_STARTRING_AA,
        PEDSTAL,
        PILLAR,
        PRISON,
        PROSON_DOOR,
        VAULTED_ARCH,
        VAULTED_WALL_AA,
        WINDOW_AA,
        MOD_VAULTED_STAIR_AA,
        MOD_VAULTED_BROKEN_AA,
        MOD_VAULTED_BROKEN_AB,
        MOD_VAULTED_CORRIDOR_AA,
        BLA_BRIDGE_ENTRANCE_02,
        MOD_BOSSROOM_CEILING_AA,
        MOD_BOSSROOM_GROUND_AA,
        MOD_BOSSROOM_GROUNDFENCE_AA,
        MOD_BOSSROOM_GROUNDFENCE_AB,
        MOD_BOSSROOM_GROUNDFENCE_AC,
        MOD_BOSSROOM_PILLAR_AA,
        MOD_BOSSROOM_PILLAR_AB,
        MOD_BOSSROOM_WALL_AA,
        MOD_BOSSROOM_WALL_AB,
        MOD_BOSSROOM_WALL_AC,
        PROP_BOSSAUTEL_AA,
        SHADOWMASK_BOSSROOM,
        HALFBOSSROOM,
        HALDBOSSROOM_V2,
        HALFBOSSROOM_HIGHER,
        MOD_GUARDRAIL_AB,
        END
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
