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
}
*/

struct MeleeIdle {
	template<typename HardCoded>
	void PlayerMove(HardCoded& npc, Client& player, ObjectMap& map) {
		auto next = HardCoded::State{ MeleeChase{player.GetID()} };
		npc.state = next;
		auto eov = new ExtOverlappedEvent{ MakeEvent([id{npc.GetID()}](){
			objManager.AccessWithValue(id, [](auto& obj, auto& map) {
				auto& npc = (NPC&)obj;
				npc.Update(map);
	});
	}) };
		PostQueuedCompletionStatus(iocpObject, 0, 0, (LPOVERLAPPED)eov);
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
		auto next = HardCoded::State{ MeleeIdle{} };
		npc.state = next;
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
			networkManager.SendNetworkMessage(player.GetSocket(), *new MsgSetHP{player.GetID(), pHP});
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
				auto eov = new ExtOverlappedEvent{ MakeEvent(body(npc.GetID(), 0, (yOffset < 0) ? -1 : 1)) };
				PostQueuedCompletionStatus(iocpObject, 0, 0, (LPOVERLAPPED)eov);
			}
			else {
				auto eov = new ExtOverlappedEvent{ MakeEvent(body(npc.GetID(), (xOffset < 0) ? -1 : 1, 0)) };
				PostQueuedCompletionStatus(iocpObject, 0, 0, (LPOVERLAPPED)eov);
			}
		}
		timerQueue.Push(MakeTimerEvent([id{ npc.GetID() }](){
			objManager.AccessWithValue(id, [](auto& obj, auto& map) {
				auto& npc = (NPC&)obj;
				npc.Update(map);
			});
		}, 1000));
	}
};