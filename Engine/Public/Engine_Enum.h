#ifndef Engine_Enum_h__
#define Engine_Enum_h__

namespace Engine
{
	enum class STATE { RIGHT, UP, LOOK, POSITION };
	enum class PROTOTYPE { GAMEOBJECT, COMPONENT };
	enum class RENDERGROUP { PRIORITY, NONBLEND, BLEND, UI, END };
	enum class WINMODE { FULL, WIN, END };
	
	enum class MOUSEKEYSTATE { LB, RB, MB, END	};
	enum class MOUSEMOVESTATE {	X, Y, Z, END	};
}

#endif // Engine_Enum_h__
