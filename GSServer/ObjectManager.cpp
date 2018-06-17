#include "stdafx.h"
#include "GSServer.h"
#include "ObjectManager.h"
#include "Globals.h"
#include "LuaFunctionCall.h"
#include "CFunctionsForLua.h"

bool ObjectManager::Insert(std::unique_ptr<Object>&& ptr)
{
	std::unique_lock<std::mutex> lg{ this->lock };
	auto id = ptr->GetID();
	auto result = this->data.emplace(id, std::move(ptr));
	return result.second;
}

bool ObjectManager::Insert(Object & o)
{
	std::unique_lock<std::mutex> lg{ this->lock };
	auto id = o.GetID();
	auto result = this->data.emplace(id, std::unique_ptr<Object>{&o});
	return result.second;
}

bool ObjectManager::Remove(unsigned int id)
{
	std::unique_lock<std::mutex> lg{ this->lock };
	if (1 == this->data.erase(id)) return true;
	return false;
}

std::unordered_set<unsigned int> ObjectManager::GetNearList(unsigned int id, ObjectMap & map)
{
	return ObjectManager::GetNearList(id, map, [](Object& me, Object& other) {
		auto[myX, myY] = me.GetPos();
		auto[otherX, otherY] = other.GetPos();
		return (std::abs(myX - otherX) <= PLAYER_VIEW_SIZE / 2) && (std::abs(myY - otherY) <= PLAYER_VIEW_SIZE / 2);
	});
}

void UpdateViewList(unsigned int id, ObjectMap& map)
{
	auto nearList = objManager.GetNearList(id, map);

	const bool amIPlayer = objManager.IsPlayer(id);
	auto it = map.find(id);
	if (map.end() == it) return;
	auto& me = *it->second;

	for (auto& playerId : nearList) {
		const bool isPlayer = objManager.IsPlayer(playerId);

		if (!amIPlayer && !isPlayer) continue; // 둘 다 NPC면 viewlist 업데이트 의미 없음.

		auto it = map.find(playerId);
		if (it == map.end()) continue;
		auto& player = *it->second;

		const auto isInserted = me.AccessToViewList([playerId](auto& viewList) {
			return viewList.insert(playerId).second;
		});

		if (isInserted && amIPlayer) {
			auto[x, y] = player.GetPos();
			networkManager.SendNetworkMessage(((Client&)me).GetSocket(), *new MsgPutObject{ player.GetID(), x, y, player.GetColor() });
		}

		const auto amIInserted = player.AccessToViewList([id{ me.GetID() }](auto& viewList){
			return viewList.insert(id).second;
		});

		if (isPlayer)
		{
			if (!amIInserted) {
				auto[x, y] = me.GetPos();
				networkManager.SendNetworkMessage(((Client&)player).GetSocket(), *new MsgMoveObject{ me.GetID(), x, y });
			}
			else {
				auto[x, y] = me.GetPos();
				networkManager.SendNetworkMessage(((Client&)player).GetSocket(), *new MsgPutObject{ me.GetID(), x, y, me.GetColor() });
			}
		}
	}

	std::vector<unsigned int> removedList;
	me.AccessToViewList([&removedList, &nearList](auto& viewList) {
		std::copy_if(viewList.begin(), viewList.end(), std::back_inserter(removedList), [&](auto id) {
			return nearList.find(id) == nearList.end();
		});
		for (auto id : removedList) viewList.erase(id);
	});

	for (auto& id : removedList) {
		if (amIPlayer)
		{
			networkManager.SendNetworkMessage(((Client&)me).GetSocket(), *new MsgRemoveObject{ id });
		}

		const bool isPlayer = objManager.IsPlayer(id);
		auto it = map.find(id);
		if (it == map.end()) continue;
		auto player = it->second.get();

		player->AccessToViewList([id{ me.GetID() }, isPlayer, player](auto& viewList) {
			auto retval = viewList.erase(id);
			if (isPlayer && 1 == retval) {
				networkManager.SendNetworkMessage(((Client*)player)->GetSocket(), *new MsgRemoveObject{ id });
			}
		});
	}
}

void Object::Move(short dx, short dy)
{
	ULock lg{ this->lock };
	this->x += dx;
	this->y += dy;
	this->x = max(0, min(this->x, BOARD_W - 1));
	this->y = max(0, min(this->y, BOARD_H - 1));
}
