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
/* 클라이언트에서 사용할 수 있는 공통적인 정의를 모아놓은 파일 */
namespace Edit
{
	const unsigned int			g_iWinSizeX = 1280;
	const unsigned int			g_iWinSizeY = 720;

	enum class LEVEL { STATIC, LOADING, EDIT, END };
}

extern HWND g_hWnd;
extern HINSTANCE g_hInst;
using namespace Edit;

struct MapObject
{
    int   id;
    int   type; // 0=Cube, 1=Sphere, ...
    float pos[3];
    float size[3];
    float rot[3];
};

static constexpr const char* ObjectTypes[5] = { "Cube", "Sphere", "Cylinder", "Capsule", "Monster" };
static constexpr int NumObjectTypes = 5;