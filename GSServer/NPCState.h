#pragma once
#include "typedef.h"

class NPCStateBase {
public:
	NPCStateBase(NPC& npc);
	virtual ~NPCStateBase() {}
	virtual void PlayerMove(Client& player) = 0;
	virtual void PlayerLeave(Client& player) = 0;
	virtual void Attacked(Client& player) = 0;
	virtual void Update() = 0;

protected:
	NPC & owner;
};

class MeleeIdle : public NPCStateBase {
public:
	MeleeIdle(NPC& npc);
	virtual void PlayerMove(Client& player);
	virtual void PlayerLeave(Client& player);
	virtual void Attacked(Client& player);
	virtual void Update();
};