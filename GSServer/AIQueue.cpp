#include "stdafx.h"
#include "LuaFunctionCall.h"
#include "AIQueue.h"
#include "NetworkManager.h"
#include "Globals.h"

bool NpcMsgComp(const NPCMsg & a, const NPCMsg & b)
{
	return a.time > b.time;
}

void NPCMsgCallback(DWORD error, ExtOverlappedNPC *& ov)
{
	using namespace std::chrono;
	auto recvTime = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count();
	std::unique_ptr<ExtOverlappedNPC> localOv{ ov };
	switch (ov->msg->type) {
	case NpcMsgType::MOVE_RANDOM:
	{
		auto locked = objManager.GetUniqueCollection();
		const auto it = locked->find(ov->msg->id);
		if (it == locked->end()) break;
		auto& npc = it->second;

		const auto direction = rand() % 4;
		auto newX = npc->x;
		auto newY = npc->y;
		{
			std::unique_lock<std::mutex> lg{ npc->lock };
			switch (direction) {
			case 0: // 왼쪽
				newX -= 1;
				break;
			case 1: // 오른쪽
				newX += 1;
				break;
			case 2: // 위
				newY -= 1;
				break;
			case 3: // 아래
				newY += 1;
				break;
			}
			npc->x = max(0, min(newX, BOARD_W - 1));
			npc->y = max(0, min(newY, BOARD_H - 1));
		}
		locked.unlock();

		auto nearList = objManager.GetNearList(npc->id);
		npc->UpdateViewList(nearList);

		std::unique_lock<std::mutex> lg{ npc->lock };
		if (npc->viewList.size() > 0) {
			npcMsgQueue.Push(*new NPCMsg(npc->id, NpcMsgType::MOVE_RANDOM, recvTime + 1000));
		}
	}
	break;
	case NpcMsgType::CALL_LUA_FUNC:
	{
		auto& rMsg = *(NPCMsgCallLuaFunc*)ov->msg.get();
		auto& call = *rMsg.call.get();
		UniqueLocked<lua_State*> lock;
		{
			auto locked = objManager.GetUniqueCollection();
			lock = ((AI_NPC*)locked->at(rMsg.id).get())->GetLuaState();
		}
		call(lock);
	}
	break;
	}
}

NPCMsg::NPCMsg(unsigned int id, NpcMsgType type, long long millis) : id{ id }, type{ type }
{
	using namespace std::chrono;
	this->time = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count() + millis;
}

NPCMsgCallLuaFunc::NPCMsgCallLuaFunc(unsigned int id, long long millis, std::unique_ptr<LuaFunctionCall>&& call) : NPCMsg{ id, NpcMsgType::CALL_LUA_FUNC, millis }, call{ std::move(call) } {}

NPCMsgCallLuaFunc::NPCMsgCallLuaFunc(unsigned int id, long long millis, LuaFunctionCall & call) : NPCMsg{ id, NpcMsgType::CALL_LUA_FUNC, millis }, call{ &call } {}
