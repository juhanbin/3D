#pragma once

#include "../Default/framework.h"
#include <process.h>

/* Ŭ���̾�Ʈ���� ����� �� �ִ� �������� ���Ǹ� ��Ƴ��� ���� */
namespace Client
{
	const unsigned int			g_iWinSizeX = 1280;
	const unsigned int			g_iWinSizeY = 720;

	enum class LEVEL { STATIC, LOADING, LOGO, GAMEPLAY, END };
}

extern HWND g_hWnd;
extern HINSTANCE g_hInst;
using namespace Client;

/* ����. */

/* 1. �˽ѳ�(��). */
/* 1. �����ǽİ�.(��) */
/* 1. ��õ����.(��) */
/* 1. ���̻�, ���̻� */
/* 1. ���¹����ڻ��� 1, 2 .(��) */
/* 1. �߰׸�����2��.(��) */
/* 1. ���ѻڶ���.(��) */
/* 1. ���չ��� ���̵�(����) */
/* 1. ����.(��) */
/* 1. �ڽ�����.(��, ��) */
/* 1. ���Ʈ��. */
/* 1. ������Ӿ�ī���� */
/* 1. ����� ���� */
/* 1. ����� Ż��. (���, �ְ� ������, �ְ� ī��, ) */
