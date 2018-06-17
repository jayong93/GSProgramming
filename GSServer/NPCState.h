#pragma once
#include "typedef.h"

struct MeleeIdle {
	void PlayerMove(NPC& npc, Client& player, ObjectMap& map);
	void PlayerLeave(NPC& npc, Client& player, ObjectMap& map);
	void Attacked(NPC& npc, Client& player, ObjectMap& map);
	void Update(NPC& npc, ObjectMap& map);
};

struct MeleeChase {
	MeleeChase(unsigned int target);
	unsigned int target;
	void PlayerMove(NPC& npc, Client& player, ObjectMap& map);
	void PlayerLeave(NPC& npc, Client& player, ObjectMap& map);
	void Attacked(NPC& npc, Client& player, ObjectMap& map);
	void Update(NPC& npc, ObjectMap& map);
};