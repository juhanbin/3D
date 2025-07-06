#pragma once

/* 모든 클래스의 부모가 되는 클래스 */
/* 모든 클래스에게 동일한 기능을 제공해주고자한다.  */
/* 동일한 기능 : 레퍼런스 카운트를 관리한다. */
/* 레퍼런스 카운트 : 참조 갯수( 객체의 주소를 보관하는 포인터형 변수의 갯수 ) */

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBase abstract
{
protected:
	CBase();
	virtual ~CBase() = default;

public:
	/* 참조갯수를 증가시키고 증가한 결과를 리턴하낟. */
	unsigned int AddRef();

	/* 참조갯수를 감소시키거나 삭제하고 감소하기 이전의 값을 리턴한다. */
	unsigned int Release();

private:
	unsigned int m_iRefCnt = {};

public:
	virtual void Free();

};
NS_END



