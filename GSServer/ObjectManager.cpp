#include "stdafx.h"
#include "GSServer.h"
#include "ObjectManager.h"
#include "Globals.h"
#include "LuaFunctionCall.h"
#include "CFunctionsForLua.h"

bool ObjectManager::Insert(std::unique_ptr<Object>&& ptr)
{
	std::unique_lock<std::mutex> lg{ this->lock };
	auto id = ptr->id;
	auto result = this->data.emplace(id, std::move(ptr));
	return result.second;
}

bool ObjectManager::Insert(Object & o)
{
	std::unique_lock<std::mutex> lg{ this->lock };
	auto id = o.id;
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
	return ObjectManager::GetNearList(id, map, [](const Object& me, const Object& other) {
		return (std::abs(me.x - other.x) <= PLAYER_VIEW_SIZE / 2) && (std::abs(me.y - other.y) <= PLAYER_VIEW_SIZE / 2);
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

		bool isInserted{ false };
		{
			std::unique_lock<std::mutex> myLG{ me.lock };
			auto result = me.viewList.insert(playerId);
			isInserted = result.second;
		}

		std::unique_lock<std::mutex> playerLG{ player.lock };
		int retval;
		if (isInserted) {
			if (amIPlayer)
			{
				networkManager.SendNetworkMessage(((Client&)me).s, *new MsgPutObject{ player.id, player.x, player.y, player.color });
			}

			auto result = player.viewList.insert(me.id);
			const bool amIInserted = result.second;
			if (isPlayer)
			{
				if (!amIInserted) {
					networkManager.SendNetworkMessage(((Client&)player).s, *new MsgMoveObject{ me.id, me.x, me.y });
				}
				else {
					networkManager.SendNetworkMessage(((Client&)player).s, *new MsgPutObject{ me.id, me.x, me.y, me.color });
				}
			}
		}
		else {
			auto result = player.viewList.insert(me.id);
			if (isPlayer)
			{
				const bool amIInserted = result.second;
				if (!amIInserted) {
					networkManager.SendNetworkMessage(((Client&)player).s, *new MsgMoveObject{ me.id, me.x, me.y });
				}
				else {
					networkManager.SendNetworkMessage(((Client&)player).s, *new MsgPutObject{ me.id, me.x, me.y, me.color });
				}
			}
		}
	}

	std::vector<unsigned int> removedList;
	{
		std::unique_lock<std::mutex> lg{ me.lock };
		std::copy_if(me.viewList.begin(), me.viewList.end(), std::back_inserter(removedList), [&](auto id) {
			return nearList.find(id) == nearList.end();
		});
		for (auto id : removedList) me.viewList.erase(id);
	}

	for (auto& id : removedList) {
		int retval;
		if (amIPlayer)
		{
			networkManager.SendNetworkMessage(((Client&)me).s, *new MsgRemoveObject{ id });
		}

		const bool isPlayer = objManager.IsPlayer(id);
		auto it = map.find(id);
		if (it == map.end()) continue;
		auto player = it->second.get();

		std::unique_lock<std::mutex> lg{ player->lock };
		retval = player->viewList.erase(me.id);
		if (isPlayer && 1 == retval) {
			networkManager.SendNetworkMessage(((Client*)player)->s, *new MsgRemoveObject{ me.id });
		}
	}
}

void Object::Move(short dx, short dy)
{
	std::unique_lock<std::mutex> lg{ this->lock };
	this->x += dx;
	this->y += dy;
	this->x = max(0, min(this->x, BOARD_W - 1));
	this->y = max(0, min(this->y, BOARD_H - 1));
}
