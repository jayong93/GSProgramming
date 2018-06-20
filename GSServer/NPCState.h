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

template<typename HardCoded, typename Condition, typename IdleState>
void MonsterChaseUpdate(HardCoded& npc, ObjectMap& map, Condition&& rangeCond, unsigned int damage, unsigned int target, IdleState idle) {
	auto it = map.find(target);
	if (map.end() == it) return;

	auto& player = *(Client*)it->second.get();

	if (player.IsDisabled()) npc.state = idle;

	const auto[px, py] = player.GetPos();
	const auto[myX, myY] = npc.GetPos();
	const auto xOffset = px - myX;
	const auto yOffset = py - myY;

	if (rangeCond(xOffset, yOffset)) {
		const auto pHP = player.AddHP(-(int)damage);
		networkManager.SendNetworkMessage(player.GetSocket(), *new MsgStatChange{ pHP, player.GetLevel(), player.GetExp() });

		TCHAR text[MAX_CHAT_LEN] = { 0, };
		const wchar_t* name{ nullptr };
		if (npc.GetType() == ObjectType::MELEE) name = L"근거리 몬스터";
		else name = L"원거리 몬스터";
		swprintf_s(text, L"%s가 플레이어를 공격해 %d의 피해를 입혔습니다", name, damage);
		networkManager.SendNetworkMessage(player.GetSocket(), *new MsgChat{ text });

		if (pHP <= 0 && !player.IsDisabled()) {
			player.SetDisable(true);
			UpdateViewList(player.GetID(), map);
			networkManager.SendNetworkMessage(player.GetSocket(), *new MsgRemoveObject{ player.GetID() });

			PostTimerEvent(3000, [id{ player.GetID() }]() {
				objManager.AccessWithValue(id, [](Object& player, ObjectMap& map) {
					auto& client = (Client&)player;
					client.SetHP(client.GetMaxHP());
					player.SetDisable(false);
					client.SendPutMessage(client.GetSocket());
					UpdateViewList(player.GetID(), map);
				});
			});
		}
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
	PostTimerEvent(2000, [id{ npc.GetID() }](){
		objManager.AccessWithValue(id, [](auto& obj, auto& map) {
			auto& npc = (NPC&)obj;
			npc.Update(map);
		});
	});
}

template<typename HardCoded>
void MonsterAttacked(HardCoded& npc, Client& player, ObjectMap& map, unsigned int exp) {
	auto hp = npc.AddHP(-10);
	auto nearList = objManager.GetNearList(npc.GetID(), map);
	for (auto id : nearList) {
		Object& obj = *map[id];
		if (obj.GetType() != ObjectType::PLAYER) continue;
		auto& client = (Client&)obj;
		networkManager.SendNetworkMessage(client.GetSocket(), *new MsgOtherStatChange{ npc.GetID(), hp, npc.GetMaxHP(), 0, 0 });

		TCHAR text[MAX_CHAT_LEN] = { 0, };
		const wchar_t* name{ nullptr };
		if (npc.GetType() == ObjectType::MELEE) name = L"근거리 몬스터";
		else name = L"원거리 몬스터";
		swprintf_s(text, L"플레이어가 %s를 공격해 %d의 피해를 입혔습니다", name, 10);
		networkManager.SendNetworkMessage(player.GetSocket(), *new MsgChat{ text });
	}


	if (hp <= 0) {
		npc.SetDisable(true);
		UpdateViewList(npc.GetID(), map);
		PostTimerEvent(30000, [id{ npc.GetID() }](){
			objManager.AccessWithValue(id, [](Object& obj, ObjectMap& map) {
				auto& npc = (NPC&)obj;
				npc.SetHP(npc.GetMaxHP());
				npc.SetDisable(false);
				UpdateViewList(npc.GetID(), map);
			});
		});
		// 플레이어 경험치 조절
		if (player.ExpUp(exp)) {
			auto playerNearList = objManager.GetNearList(player.GetID(), map);
			while (player.LevelUp(1)) {
				auto pHP = player.GetHP();
				auto pMHP = player.GetMaxHP();
				auto pLV = player.GetLevel();
				auto pEXP = player.GetExp();
				for (auto id : playerNearList) {
					Object& obj = *map[id];
					if (obj.GetType() != ObjectType::PLAYER) continue;
					networkManager.SendNetworkMessage(((Client&)obj).GetSocket(), *new MsgOtherStatChange{ player.GetID(), pHP, pMHP, pLV, pEXP });
				}
				networkManager.SendNetworkMessage(player.GetSocket(), *new MsgStatChange{ pHP, pLV, pEXP });
			}
		}
	}
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
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) { MonsterAttacked(npc, player, map, exp); }
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
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) { MonsterAttacked(npc, player, map, exp); }
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {
		MonsterChaseUpdate(npc, map, [](auto xOffset, auto yOffset) {
			return abs(xOffset) + abs(yOffset) <= 1;
		}, damage, target, MeleeIdle{ damage, exp });
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
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) { MonsterAttacked(npc, player, map, exp); }
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
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) { MonsterAttacked(npc, player, map, exp); }
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {
		MonsterChaseUpdate(npc, map, [range{ this->range }](auto xOffset, auto yOffset) {
			return pow(xOffset, 2) + pow(yOffset, 2) <= pow(range, 2);
		}, damage, target, RangeIdle{ damage, exp, range });
	}

private:
	unsigned int range;
	unsigned int target;
};