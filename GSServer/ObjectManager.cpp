#include "stdafx.h"
#include "GSServer.h"
#include "ObjectManager.h"
#include "Globals.h"
#include "LuaFunctionCall.h"
#include "NPC.h"
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

bool ObjectManager::Remove(WORD id)
{
	std::unique_lock<std::mutex> lg{ this->lock };
	if (1 == this->data.erase(id)) return true;
	return false;
}

std::unordered_set<WORD> ObjectManager::GetNearList(WORD id, ObjectMap & map)
{
	return ObjectManager::GetNearList(id, map, [](Object& me, Object& other) {
		auto[myX, myY] = me.GetPos();
		auto[otherX, otherY] = other.GetPos();
		return (std::abs(myX - otherX) <= PLAYER_VIEW_SIZE / 2) && (std::abs(myY - otherY) <= PLAYER_VIEW_SIZE / 2);
	});
}

void UpdateViewList(WORD id, ObjectMap& map)
{
	auto nearList = objManager.GetNearList(id, map);

	const bool amIPlayer = objManager.IsPlayer(id);
	auto it = map.find(id);
	if (map.end() == it) return;
	auto& me = *it->second;

	for (auto& otherID : nearList) {
		const bool isPlayer = objManager.IsPlayer(otherID);

		if (!amIPlayer && !isPlayer) continue; // 둘 다 NPC면 viewlist 업데이트 의미 없음.

		auto it = map.find(otherID);
		if (it == map.end()) continue;
		auto& other = *it->second;

		const auto isInserted = me.AccessToViewList([otherID](auto& viewList) {
			return viewList.insert(otherID).second;
		});

		if (isInserted && amIPlayer) {
			auto[x, y] = other.GetPos();
			other.SendPutMessage(((Client&)me).GetSocket());
		}

		const auto amIInserted = other.AccessToViewList([id{ me.GetID() }](auto& viewList){
			return viewList.insert(id).second;
		});

		if (isPlayer)
		{
			if (!amIInserted) {
				auto[x, y] = me.GetPos();
				networkManager.SendNetworkMessage(((Client&)other).GetSocket(), *new MsgSetPosition{ me.GetID(), x, y });
			}
			else {
				auto[x, y] = me.GetPos();
				me.SendPutMessage(((Client&)other).GetSocket());
			}
		}
		else {
			auto& npc = (NPC&)other;
			// 둘 다 NPC면 여기까지 도달할 수 없기 때문에 강제 캐스팅 해도 안전.
			npc.PlayerMove((Client&)me, map);
		}
	}

	std::vector<WORD> removedList;
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
		auto other = it->second.get();

		other->AccessToViewList([id{ me.GetID() }, isPlayer, other](auto& viewList) {
			auto retval = viewList.erase(id);
			if (isPlayer && 1 == retval) {
				networkManager.SendNetworkMessage(((Client*)other)->GetSocket(), *new MsgRemoveObject{ id });
			}
		});

		if (amIPlayer && !isPlayer) {
			auto& npc = *(NPC*)other;
			npc.PlayerLeave((Client&)me, map);
		}
	}
}

void Object::Move(short dx, short dy)
{
	short oldX, oldY, newX, newY;
	{
		ULock lg{ this->lock };
		oldX = this->x;
		oldY = this->y;
		this->x += dx;
		this->y += dy;
		this->x = max(0, min(this->x, BOARD_W - 1));
		this->y = max(0, min(this->y, BOARD_H - 1));
		newX = this->x;
		newY = this->y;
	}
	sectorManager.UpdateSector(this->id, oldX, oldY, newX, newY);
}

void Object::SetPos(short x, short y) {
	short oldX, oldY, newX, newY;
	{
		ULock{ lock };
		oldX = this->x;
		oldY = this->y;
		this->x = x; this->y = y;
		newX = this->x;
		newY = this->y;
	}
	sectorManager.UpdateSector(this->id, oldX, oldY, newX, newY);
}

void Object::SendPutMessage(SOCKET s)
{
	WORD id;
	WORD x, y;
	Color* color;
	ObjectType type;
	{
		ULock lg{ lock };
		id = this->id;
		x = this->x;
		y = this->y;
		color = &this->color;
		type = this->type;
	}
	networkManager.SendNetworkMessage(s, *new MsgAddObject{ id, type, x, y });
	networkManager.SendNetworkMessage(s, *new MsgSetColor{ id, *color });
}

void HPObject::SendPutMessage(SOCKET s)
{
	Object::SendPutMessage(s);
	networkManager.SendNetworkMessage(s, *new MsgOtherStatChange{ GetID(), GetHP(), GetMaxHP(), 0, 0 });
}

void Client::SendPutMessage(SOCKET s)
{
	Object::SendPutMessage(s);
	if (this->receiver->s == s) {
		networkManager.SendNetworkMessage(s, *new MsgStatChange{ GetHP(), GetLevel(), GetExp() });
	}
	else {
		networkManager.SendNetworkMessage(s, *new MsgOtherStatChange{ GetID(), GetHP(), GetMaxHP(), GetLevel(), GetExp() });
	}
}
