#pragma once
#include <Windows.h>

#include "Common\Objects.h"

enum LFGStates
{
	LFGSTATE_START = 0x01,
	LFGSTATE_WAIT = 0x02,
	LFGSTATE_HEAL= 0x03,
	LFGSTATE_IDLE = 0x04,
	LFGSTATE_END = 0x05,
};

UINT LFGMoving(UINT nTimeLimit);
BOOL StartLFG(INT nTimeLimit);