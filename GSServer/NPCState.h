#pragma once
#include "typedef.h"

struct MonsterStateBase {
	MonsterStateBase(unsigned int damage, unsigned int exp) : damage{ damage }, exp{ exp } {}

	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void PlayerLeave(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {}

protected:
	unsigned int damage, exp;
};

template<typename HardCoded, typename Condition>
void MonsterChaseUpdate(HardCoded& npc, ObjectMap& map, Condition&& rangeCond, unsigned int damage, unsigned int target) {
	auto it = map.find(target);
	if (map.end() == it) return;

	auto& player = *(Client*)it->second.get();
	const auto[px, py] = player.GetPos();
	const auto[myX, myY] = npc.GetPos();
	const auto xOffset = px - myX;
	const auto yOffset = py - myY;

	if (rangeCond(xOffset, yOffset)) {
		const auto pHP = player.AddHP(-(int)damage);
		networkManager.SendNetworkMessage(player.GetSocket(), *new MsgStatChange{ pHP, player.GetLevel(), player.GetExp() });
	}
	else {
		// 플레이어 추적
		auto body = [](auto id, auto xOffset, auto yOffset) {
			return [=]() {
				objManager.AccessWithValue(id, [=](auto& obj, auto& map) {
					obj.Move(xOffset, yOffset);
					UpdateViewList(obj.GetID(), map);
				});
			};
		};
		if (abs(xOffset) < abs(yOffset)) {
			PostEvent(body(npc.GetID(), 0, (yOffset < 0) ? -1 : 1));
		}
		else {
			PostEvent(body(npc.GetID(), (xOffset < 0) ? -1 : 1, 0));
		}
	}
	PostTimerEvent(1000, [id{ npc.GetID() }](){
		objManager.AccessWithValue(id, [](auto& obj, auto& map) {
			auto& npc = (NPC&)obj;
			npc.Update(map);
		});
	});
}

struct MeleeIdle : public MonsterStateBase {
	MeleeIdle(unsigned int damage, unsigned int exp) : MonsterStateBase{ damage, exp } {}

	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {
		npc.state = MeleeChase{ damage, exp, player.GetID() };
		PostTimerEvent(1000, [id{ npc.GetID() }](){
			objManager.AccessWithValue(id, [](auto& obj, auto& map) {
				auto& npc = (NPC&)obj;
				npc.Update(map);
			});
		});
	}
	template<typename HardCoded>
	void PlayerLeave(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {}
};

struct MeleeChase : public MonsterStateBase {
	MeleeChase(unsigned damage, unsigned exp, unsigned int target) : MonsterStateBase{ damage, exp }, target{ target } {}
	unsigned int target;

	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void PlayerLeave(HardCoded& npc, Client& player, ObjectMap& map) {
		if (player.GetID() == target) {
			npc.state = MeleeIdle{ damage, exp };
		}
	}
	template<typename HardCoded>
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {
		MonsterChaseUpdate(npc, map, [](auto xOffset, auto yOffset) {
			return abs(xOffset) + abs(yOffset) <= 1;
		}, damage, target);
	}
};

struct RangeIdle : public MonsterStateBase {
	RangeIdle(unsigned damage, unsigned exp, unsigned int range) : MonsterStateBase{ damage, exp }, range{ range } {}

	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {
		npc.state = RangeChase{ damage, exp, player.GetID(), range };
		PostTimerEvent(1000, [id{ npc.GetID() }](){
			objManager.AccessWithValue(id, [](auto& obj, auto& map) {
				auto& npc = (NPC&)obj;
				npc.Update(map);
			});
		});
	}
	template<typename HardCoded>
	void PlayerLeave(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {}

private:
	unsigned int range;
};

struct RangeChase : public MonsterStateBase {
	RangeChase(unsigned damage, unsigned exp, unsigned int id, unsigned int range) : MonsterStateBase{ damage, exp }, target{ id }, range{ range } {}

	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void PlayerLeave(HardCoded& npc, Client& player, ObjectMap& map) {
		npc.state = RangeIdle{ damage, exp, range };
	}
	template<typename HardCoded>
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {
		MonsterChaseUpdate(npc, map, [range{ this->range }](auto xOffset, auto yOffset) {
			return pow(xOffset, 2) + pow(yOffset, 2) <= pow(range, 2);
		}, damage, target);
	}

private:
	unsigned int range;
	unsigned int target;
};