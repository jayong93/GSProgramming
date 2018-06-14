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

std::unordered_set<unsigned int> ObjectManager::GetNearList(unsigned int id)
{
	std::unordered_set<unsigned int> nearList;
	objManager.LockAndExec([id, &nearList](auto& map){
		const Object* obj = map.at(id).get();

		auto nearSectors = sectorManager.GetNearSectors(sectorManager.PositionToSectorIndex(obj->x, obj->y));
		for (auto s : nearSectors) {
			std::copy_if(s.begin(), s.end(), std::inserter(nearList, nearList.end()), [&](unsigned int id) {
				if (id == obj->id) return false;

				auto it = map.find(id);
				if (it == map.end()) return false;
				auto o = it->second.get();
				std::unique_lock<std::mutex> lg{ o->lock };
				return (std::abs(obj->x - o->x) <= PLAYER_VIEW_SIZE / 2) && (std::abs(obj->y - o->y) <= PLAYER_VIEW_SIZE / 2);
			});
		}
	});
	return nearList;
}

void UpdateViewList(unsigned int id, std::unordered_set<unsigned int>& nearList, ObjectMap& map)
{
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

void ObjectManager::UpdateViewList(unsigned int id)
{
	auto nearList = this->GetNearList(id);
	std::unique_lock<std::mutex> lg{ this->lock };
	::UpdateViewList(id, nearList, this->data);
}

void Object::Move(short dx, short dy)
{
	this->x += dx;
	this->y += dy;
	this->x = max(0, min(this->x, BOARD_W - 1));
	this->y = max(0, min(this->y, BOARD_H - 1));
}

lua_State * AI_NPC::InitLuaState(unsigned int id, const char* scriptName) noexcept
{
	auto state = luaL_newstate();
	luaL_openlibs(state);
	luaL_loadfile(state, scriptName);
	if (lua_pcall(state, 0, 0, 0) != 0) {
		display_error(state);
		return state;
	}
	RegisterCFunctions(state);
	lua_getglobal(state, "set_my_id");
	lua_pushnumber(state, id);
	if (lua_pcall(state, 1, 0, 0) != 0) {
		display_error(state);
		return state;
	}
	return state;
}
