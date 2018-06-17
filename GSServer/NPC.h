#pragma once
#include "ObjectManager.h"
#include "LuaModule.h"
#include "NPCState.h"

class NPC : public Object {
public:
	NPC(unsigned int id, short x, short y, Color color) : Object{ id, x, y, color } {}
	virtual ~NPC() {}

	virtual void PlayerMove(Client& player, ObjectMap& map) = 0;
	virtual void PlayerLeave(Client& player, ObjectMap& map) = 0;
	virtual void Attacked(Client& player, ObjectMap& map) = 0;
	virtual void Update(ObjectMap& map) = 0;
};

template <typename Idle, typename... States>
class HardcodedNPC : public NPC {
public:
	using State = std::variant<Idle, States...>;

	HardcodedNPC(unsigned int id, short x, short y, Color color) : NPC{ id, x, y, color }, state{ Idle{} } {}

	virtual void PlayerMove(Client& player, ObjectMap& map) {
		std::visit([this, &player, &map](auto&& s) {s.PlayerMove(*this, player, map); }, state);
	}
	virtual void PlayerLeave(Client& player, ObjectMap& map) {
		std::visit([this, &player, &map](auto&& s) {s.PlayerLeave(*this, player, map); }, state);
	}
	virtual void Attacked(Client& player, ObjectMap& map) {
		std::visit([this, &player, &map](auto&& s) {s.Attacked(*this, player, map); }, state);
	}
	virtual void Update(ObjectMap& map) {
		std::visit([this,&map](auto&& s) {s.Update(*this, map); }, state);
	}

	State state;
};

using MeleeMonster = HardcodedNPC<MeleeIdle, MeleeChase>;

class AI_NPC : public NPC {
public:
	AI_NPC(unsigned int id, short x, short y, Color color, const char* scriptName) : NPC{ id, x, y, color }, lua{ id, scriptName } {};

	virtual void PlayerMove(Client& player, ObjectMap& map);
	virtual void PlayerLeave(Client& player, ObjectMap& map);
	virtual void Attacked(Client& player, ObjectMap& map);
	virtual void Update(ObjectMap& map);

	LuaModule lua;
};

