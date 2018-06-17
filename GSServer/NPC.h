#pragma once
#include "ObjectManager.h"
#include "LuaModule.h"
#include "NPCState.h"

class NPC : public Object {
public:
	NPC(unsigned int id, short x, short y, Color color) : Object{ id, x, y, color } {}
	virtual ~NPC() {}

	virtual void PlayerMove(Client& player) = 0;
	virtual void PlayerLeave(Client& player) = 0;
	virtual void Attacked(Client& player) = 0;
	virtual void Update() = 0;
};

template <typename Idle, typename... States>
class HardcodedNPC : public NPC {
public:
	using State = std::variant<Idle, States...>;

	HardcodedNPC(unsigned int id, short x, short y, Color color) : NPC{ id, x, y, color }, state{ Idle{*this} } {}

	virtual void PlayerMove(Client& player) {
		std::visit([&player](auto&& s) {s.PlayerMove(player); }, state);
	}
	virtual void PlayerLeave(Client& player) {
		std::visit([&player](auto&& s) {s.PlayerLeave(player); }, state);
	}
	virtual void Attacked(Client& player) {
		std::visit([&player](auto&& s) {s.Attacked(player); }, state);
	}
	virtual void Update() {
		std::visit([](auto&& s) {s.Update(); }, state);
	}

	State state;
};

using MeleeMonster = HardcodedNPC<MeleeIdle>;

class AI_NPC : public NPC {
public:
	AI_NPC(unsigned int id, short x, short y, Color color, const char* scriptName) : NPC{ id, x, y, color }, lua{ id, scriptName } {};

	virtual void PlayerMove(Client& player);
	virtual void PlayerLeave(Client& player);
	virtual void Attacked(Client& player);
	virtual void Update();

	LuaModule lua;
};

