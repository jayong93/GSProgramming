#include "stdafx.h"
#include "NPCState.h"
#include "NPC.h"
#include "Globals.h"
#include "../Share/Shares.h"


void MeleeIdle::PlayerMove(NPC & npc, Client & player, ObjectMap& map)
{
	auto next = AMeleeMonster::State{ MeleeChase{player.GetID()} };
	((AMeleeMonster&)(npc)).state = next;
	auto eov = new ExtOverlappedEvent{ MakeEvent([id{npc.GetID()}](){
		objManager.AccessWithValue(id, [](auto& obj, auto& map) {
			auto& npc = (NPC&)obj;
			npc.Update(map);
});
}) };
	PostQueuedCompletionStatus(iocpObject, 0, 0, (LPOVERLAPPED)eov);
}

void MeleeIdle::PlayerLeave(NPC & npc, Client & player, ObjectMap& map)
{
}

void MeleeIdle::Attacked(NPC & npc, Client & player, ObjectMap& map)
{
}

void MeleeIdle::Update(NPC & npc, ObjectMap& map)
{
}

MeleeChase::MeleeChase(unsigned int target) : target{ target }
{
}

void MeleeChase::PlayerMove(NPC & npc, Client & player, ObjectMap& map)
{
}

void MeleeChase::PlayerLeave(NPC & npc, Client & player, ObjectMap& map)
{
	auto next = AMeleeMonster::State{ MeleeIdle{} };
	((AMeleeMonster&)npc).state.swap(next);
}

void MeleeChase::Attacked(NPC & npc, Client & player, ObjectMap& map)
{
}

void MeleeChase::Update(NPC & npc, ObjectMap& map)
{
	auto it = map.find(target);
	if (map.end() == it) return;

	auto& player = *(Client*)it->second.get();
	const auto[px, py] = player.GetPos();
	const auto[myX, myY] = npc.GetPos();
	if ((abs(px - myX) + abs(py - myY)) <= 1) {
		networkManager.SendNetworkMessage(player.GetSocket(), *new MsgChat{ npc.GetID(), L"I attacked you!" });
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
