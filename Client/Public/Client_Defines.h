#pragma once

#include "../Default/framework.h"
#include <process.h>

/* 클라이언트에서 사용할 수 있는 공통적인 정의를 모아놓은 파일 */
namespace Client
{
	const unsigned int			g_iWinSizeX = 1280;
	const unsigned int			g_iWinSizeY = 720;

	enum class LEVEL { STATIC, LOADING, LOGO, GAMEPLAY, END };
	enum  class FILETYPE { FBX, BIN };

	enum class EObjectType
	{
		MONSTER,
		ROCK_AA,
		// ...추가 오브젝트 타입
	};

	struct MapObject
	{
		int id;
		int type;
		float size[3];
		float rot[3];
		float pos[3];
		char fbxPath[260];
		char binPath[260];
	};
	extern MapObject obj;
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
