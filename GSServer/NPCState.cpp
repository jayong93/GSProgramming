#include "stdafx.h"
#include "NPCState.h"
#include "NPC.h"
#include "Globals.h"
#include "../Share/Shares.h"

NPCStateBase::NPCStateBase(NPC & npc) : owner{ npc }
{
}

MeleeIdle::MeleeIdle(NPC & npc) : NPCStateBase{ npc }
{
}

void MeleeIdle::PlayerMove(Client & player)
{
	const auto[px, py] = player.GetPos();
	const auto[myX, myY] = owner.GetPos();
	if ((abs(px - myX) + abs(py - myY)) <= 1) {
		networkManager.SendNetworkMessage(player.GetSocket(), *new MsgChat{ owner.GetID(), L"I attacked you!" });
	}
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

MeleeChase::MeleeChase(NPC & npc) : NPCStateBase{ npc }
{
}

void MeleeChase::PlayerMove(Client & player)
{
}

void MeleeChase::PlayerLeave(Client & player)
{
}

void MeleeChase::Attacked(Client & player)
{
}

void MeleeChase::Update()
{
}
