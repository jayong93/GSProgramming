#include "stdafx.h"
#include "GSServer.h"
#include "ObjectManager.h"
#include "Globals.h"

std::unordered_set<unsigned int> ObjectManager::GetNearList(unsigned int id)
{
	std::unordered_set<unsigned int> nearList;
	{
		auto locked = this->GetSharedCollection();
		auto& objMap = locked.data;
		const Object* obj = locked.data.at(id).get();

		auto nearSectors = sectorManager.GetNearSectors(sectorManager.PositionToSectorIndex(obj->x, obj->y));
		for (auto s : nearSectors) {
			std::copy_if(s.begin(), s.end(), std::inserter(nearList, nearList.end()), [&](unsigned int id) {
				if (id == obj->id) return false;

				auto it = objMap.find(id);
				if (it == objMap.end()) return false;
				auto o = it->second.get();
				std::shared_lock<std::shared_timed_mutex> lg{ o->lock };
				return (std::abs(obj->x - o->x) <= PLAYER_VIEW_SIZE / 2) && (std::abs(obj->y - o->y) <= PLAYER_VIEW_SIZE / 2);
			});
		}
	}
	return nearList;
}

// 데드락 발생!!
void Object::UpdateViewList()
{
	//auto nearList = objManager.GetNearList(id);

	//const bool amIPlayer = objManager.IsPlayer(id);
	//auto locked = objManager.GetSharedCollection();
	//auto& clientMap = locked.data;
	//auto& me = *this;

	//for (auto& playerId : nearList) {
	//	const bool isPlayer = objManager.IsPlayer(playerId);

	//	if (!amIPlayer && !isPlayer) continue; // 둘 다 NPC면 viewlist 업데이트 의미 없음.

	//	std::shared_lock<std::shared_timed_mutex> plg;
	//	auto it = clientMap.find(playerId);
	//	if (it == clientMap.end()) continue;
	//	auto& player = *it->second;

	//	std::lock(me.lock, player.lock);
	//	std::unique_lock<std::shared_timed_mutex> myLG{ me.lock, std::adopt_lock };
	//	std::unique_lock<std::shared_timed_mutex> playerLG{ player.lock, std::adopt_lock };

	//	auto result = me.viewList.insert(playerId);
	//	myLG.unlock();
	//	const bool isInserted = result.second;

	//	int retval;
	//	if (isInserted) {
	//		if (amIPlayer)
	//		{
	//			networkManager.SendNetworkMessage(((Client&)me).s, *new MsgPutObject{ player.id, player.x, player.y, player.color });
	//		}

	//		result = player.viewList.insert(me.id);
	//		const bool amIInserted = result.second;
	//		if (isPlayer)
	//		{
	//			if (!amIInserted) {
	//				networkManager.SendNetworkMessage(((Client&)player).s, *new MsgMoveObject{ me.id, me.x, me.y });
	//			}
	//			else {
	//				networkManager.SendNetworkMessage(((Client&)player).s, *new MsgPutObject{ me.id, me.x, me.y, me.color });
	//			}
	//		}
	//		// NPC라면 플레이어가 근처에 왔을 때 이동 타이머 시작
	//		else {
	//			npcMsgQueue.Push(NPCMsg(playerId, NpcMsgType::MOVE_RANDOM, 0)); // 바로 NPC 이동 시작
	//		}
	//	}
	//	else {
	//		result = player.viewList.insert(me.id);
	//		if (isPlayer)
	//		{
	//			const bool amIInserted = result.second;
	//			if (!amIInserted) {
	//				networkManager.SendNetworkMessage(((Client&)player).s, *new MsgMoveObject{ me.id, me.x, me.y });
	//			}
	//			else {
	//				networkManager.SendNetworkMessage(((Client&)player).s, *new MsgPutObject{ me.id, me.x, me.y, me.color });
	//			}
	//		}
	//	}
	//}

	//std::vector<unsigned int> removedList;
	//{
	//	std::unique_lock<std::shared_timed_mutex> lg{ me.lock };
	//	std::copy_if(me.viewList.begin(), me.viewList.end(), std::back_inserter(removedList), [&](auto id) {
	//		return nearList.find(id) == nearList.end();
	//	});
	//	for (auto id : removedList) me.viewList.erase(id);
	//}

	//for (auto& id : removedList) {
	//	int retval;
	//	if (amIPlayer)
	//	{
	//		networkManager.SendNetworkMessage(((Client&)me).s, *new MsgRemoveObject{ id });
	//	}

	//	const bool isPlayer = objManager.IsPlayer(id);
	//	auto it = clientMap.find(id);
	//	if (it == clientMap.end()) continue;
	//	auto player = it->second.get();

	//	std::unique_lock<std::shared_timed_mutex> lg{ player->lock };
	//	retval = player->viewList.erase(me.id);
	//	if (isPlayer && 1 == retval) {
	//		networkManager.SendNetworkMessage(((Client*)player)->s, *new MsgRemoveObject{ me.id });
	//	}
	//}
}
