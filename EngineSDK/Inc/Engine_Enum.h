#ifndef Engine_Enum_h__
#define Engine_Enum_h__

namespace Engine
{
	enum class MODELTYPE { NONANIM, ANIM };
	enum class COLLIDER { AABB, OBB, SPHERE, END };
	enum class NAVPOINT { A, B, C, END };
	enum class LINE { AB, BC, CA, END };
	enum class D3DTS { VIEW, PROJ, END };
	enum class STATE { RIGHT, UP, LOOK, POSITION };
	enum class PROTOTYPE { GAMEOBJECT, COMPONENT };
	enum class RENDERGROUP { PRIORITY, NONBLEND, NONLIGHT, BLEND, UI, FADE, END };
	enum class WINMODE { FULL, WIN, END };

	enum class MOUSEKEYSTATE { LB, RB, WB, END };
	enum class MOUSEMOVESTATE { X, Y, WHEEL, END };

	enum class FILETYPE {FBX,BIN};

	struct TRS {
		DirectX::XMFLOAT3 vScale{ 1.f, 1.f, 1.f };
		DirectX::XMFLOAT4 vRotation{ 0.f, 0.f, 0.f, 1.f }; // quaternion (w=1)
		DirectX::XMFLOAT3 vTranslation{ 0.f, 0.f, 0.f };
	};
}

#endif // Engine_Enum_h__
