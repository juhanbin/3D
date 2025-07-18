#ifndef InputDev_h__
#define InputDev_h__

#include "Base.h"

NS_BEGIN(Engine)

class  CInput_Device : public CBase
{
private:
	CInput_Device(void);
	virtual ~CInput_Device(void) = default;

public:
	_byte	Get_DIKeyState(_ubyte byKeyID)
	{
		return m_byKeyState[byKeyID];
	}

	_byte	Get_DIMouseState(MOUSEKEYSTATE eMouse)
	{
		return m_tMouseState.rgbButtons[static_cast<_uint>(eMouse)];
	}

	// 현재 마우스의 특정 축 좌표를 반환
	_long	Get_DIMouseMove(MOUSEMOVESTATE eMouseState)
	{
		/*	switch (eMouseState)
			{
			case X:
				return m_tMouseState.lX;
			case Y:
				return m_tMouseState.lY;
			case WHEEL:
				return m_tMouseState.lZ;
			}*/

		return *((reinterpret_cast<_int*>(&m_tMouseState)) + static_cast<_uint>(eMouseState));
	}

public:
	bool KeyDown(_ubyte byKeyID) 
	{
		return (m_byKeyState[byKeyID] & 0x80) && !(m_byPrevKeyState[byKeyID] & 0x80);
	}
	bool KeyPressing(_ubyte byKeyID) 
	{
		return (m_byKeyState[byKeyID] & 0x80);
	}
	bool KeyUp(_ubyte byKeyID) 
	{
		return !(m_byKeyState[byKeyID] & 0x80) && (m_byPrevKeyState[byKeyID] & 0x80);
	}

public:
	HRESULT Initialize(HINSTANCE hInst, HWND hWnd);
	void	Update(void);

private:
	LPDIRECTINPUT8			m_pInputSDK = { nullptr };

private:
	LPDIRECTINPUTDEVICE8	m_pKeyBoard = { nullptr };
	LPDIRECTINPUTDEVICE8	m_pMouse = { nullptr };
	LPDIRECTINPUTDEVICE8	m_pJoystick = { nullptr };





private:
private:
	_byte					m_byPrevKeyState[256] = {};
	_byte					m_byKeyState[256] = {};
	DIMOUSESTATE			m_tMouseState = {};

public:
	static CInput_Device* Create(HINSTANCE hInstance, HWND hWnd);
	virtual void Free(void);

};

NS_END
#endif // InputDev_h__


