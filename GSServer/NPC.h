#pragma once
#include "ObjectManager.h"
#include "LuaModule.h"
#include "NPCState.h"

enum class AttackType { PEACEFUL, AGGRESSIVE };

class NPC : public HPObject {
public:
	NPC(unsigned int id, short x, short y, const Color& color, ObjectType type, int hp) : HPObject{ id, x, y, color, type, hp } {}
	virtual ~NPC() {}

	virtual void PlayerMove(Client& player, ObjectMap& map) = 0;
	virtual void PlayerLeave(Client& player, ObjectMap& map) = 0;
	virtual void Attacked(Client& player, ObjectMap& map) = 0;
	virtual void Update(ObjectMap& map) = 0;
};

template <ObjectType Type, AttackType AType, typename Idle, typename... States>
class HardcodedNPC : public NPC {
public:
	using State = std::variant<Idle, States...>;

	HardcodedNPC(unsigned int id, short x, short y, int hp) : NPC{ id, x, y, GetColor(), Type, hp }, state{ Idle{} } {}

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
		std::visit([this, &map](auto&& s) {s.Update(*this, map); }, state);
	}

	State state;

private:
	static constexpr auto GetColor() {
		if constexpr(AType == AttackType::AGGRESSIVE) {
			return Color{ 255, 0, 0 };
		}
		else {
			return Color{ 0,0,255 };
		}
	}
};

using AMeleeMonster = HardcodedNPC <ObjectType::MELEE, AttackType::AGGRESSIVE, MeleeIdle, MeleeChase>;
using ARangeMonster = HardcodedNPC<ObjectType::RANGE, AttackType::AGGRESSIVE, RangeIdle, RangeChase>;

class AI_NPC : public NPC {
public:
	AI_NPC(unsigned int id, short x, short y, const Color& color, const char* scriptName, int hp) : NPC{ id, x, y, color, ObjectType::OBJECT, hp }, lua{ id, scriptName } {};

	virtual void PlayerMove(Client& player, ObjectMap& map);
	virtual void PlayerLeave(Client& player, ObjectMap& map);
	virtual void Attacked(Client& player, ObjectMap& map);
	virtual void Update(ObjectMap& map);

	LuaModule lua;
};

