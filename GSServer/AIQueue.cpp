#include "stdafx.h"
#include "AIQueue.h"

bool NpcMsgComp(const NPCMsg & a, const NPCMsg & b)
{
	return a.time > b.time;
}
