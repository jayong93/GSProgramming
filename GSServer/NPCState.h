#pragma once
#include "typedef.h"

/*
struct StateBase {
	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void PlayerLeave(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {}
};
*/

struct MeleeIdle {
	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {
		npc.state = MeleeChase{player.GetID()};
		PostEvent([id{ npc.GetID() }](){
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

struct MeleeChase {
	MeleeChase(unsigned int target) : target{ target } {}
	unsigned int target;

	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void PlayerLeave(HardCoded& npc, Client& player, ObjectMap& map) {
		npc.state = MeleeIdle{};
	}
	template<typename HardCoded>
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {
		auto it = map.find(target);
		if (map.end() == it) return;

		auto& player = *(Client*)it->second.get();
		const auto[px, py] = player.GetPos();
		const auto[myX, myY] = npc.GetPos();
		if ((abs(px - myX) + abs(py - myY)) <= 1) {
			const auto pHP = player.AddHP(-10);
			networkManager.SendNetworkMessage(player.GetSocket(), *new MsgSetHP{ player.GetID(), pHP });
			player.AccessToViewList([&map, &player, pHP](auto& viewList) {
				for (auto id : viewList) {
					if (objManager.IsPlayer(id)) {
						networkManager.SendNetworkMessageWithID(id, *new MsgSetHP{ player.GetID(), pHP }, map);
					}
				}
			});
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
			const auto xOffset = px - myX;
			const auto yOffset = py - myY;
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
};

struct RangeIdle {
	RangeIdle(unsigned int range) : range{ range } {}

	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {
		npc.state = RangeChase{ player.GetID(), range };
		PostEvent([id{ npc.GetID() }](){
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

struct RangeChase {
	RangeChase(unsigned int id, unsigned int range) : target{ id }, range{ range } {}

	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void PlayerLeave(HardCoded& npc, Client& player, ObjectMap& map) {
		npc.state = RangeIdle{range};
	}
	template<typename HardCoded>
	void Attacked(HardCoded& npc, Client& player, ObjectMap& map) {}
	template<typename HardCoded>
	void Update(HardCoded& npc, ObjectMap& map) {
		auto it = map.find(target);
		if (map.end() == it) return;

		auto& player = *(Client*)it->second.get();
		const auto[px, py] = player.GetPos();
		const auto[myX, myY] = npc.GetPos();
		const auto xOffset = px - myX;
		const auto yOffset = py - myY;

		if (pow(xOffset, 2) + pow(yOffset, 2) <= pow(range, 2)) {
			const auto pHP = player.AddHP(-5);
			networkManager.SendNetworkMessage(player.GetSocket(), *new MsgSetHP{ player.GetID(), pHP });
			player.AccessToViewList([&map, &player, pHP](auto& viewList) {
				for (auto id : viewList) {
					if (objManager.IsPlayer(id)) {
						networkManager.SendNetworkMessageWithID(id, *new MsgSetHP{ player.GetID(), pHP }, map);
					}
				}
			});
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

private:
	unsigned int range;
	unsigned int target;
};