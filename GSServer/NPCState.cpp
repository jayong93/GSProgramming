#include "stdafx.h"
#include "NPCState.h"
#include "NPC.h"

NPCStateBase::NPCStateBase(NPC & npc) : owner{ npc }
{
}

MeleeIdle::MeleeIdle(NPC & npc) : NPCStateBase{ npc }
{
}

void MeleeIdle::PlayerMove(Client & player)
{
}

void MeleeIdle::PlayerLeave(Client & player)
{
}

void MeleeIdle::Attacked(Client & player)
{
}

void MeleeIdle::Update()
{
}
