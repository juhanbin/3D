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
    const unsigned int g_iWinSizeX = 1280;
    const unsigned int g_iWinSizeY = 720;

    enum class LEVEL { STATIC, LOADING, EDIT, END };
}

extern HWND g_hWnd;
extern HINSTANCE g_hInst;
using namespace Edit;

// --- enum 기반 오브젝트 타입 ---
enum class EObjectType
{
    MONSTER,
    ROCK_AA,
    END
};

// 오브젝트 타입 이름 변환 함수
inline const char* ToObjectTypeString(EObjectType type)
{
    switch (type)
    {
    case EObjectType::MONSTER:  return "Monster";
    case EObjectType::ROCK_AA:  return "Rock_AA";
    default:                    return "Unknown";
    }
}

// enum 사용 구조체
struct MapObject
{
    int         id;
    EObjectType type;
    float       pos[3];
    float       size[3];
    float       rot[3];
};

static constexpr int NumObjectTypes = static_cast<int>(EObjectType::END);

