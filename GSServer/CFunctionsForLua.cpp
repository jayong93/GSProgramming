#include "stdafx.h"
#define C_FUNCTIONS_FOR_LUA
#include "CFunctionsForLua.h"
#include "../Share/Shares.h"
#include "LuaFunctionCall.h"
#include "Globals.h"

int C_SendMessage(lua_State * L)
{
	unsigned int myId = lua_tonumber(L, -2);
	std::wstring msg(MAX_CHAT_LEN, L'\0');
	const char* mbMsg = lua_tostring(L, -1);
	auto msgLen = strlen(mbMsg);
	if (msgLen > MAX_CHAT_LEN)
		msgLen = MAX_CHAT_LEN;
	size_t converted;
	mbstowcs_s(&converted, &msg[0], msg.size(), mbMsg, msgLen);
	lua_pop(L, 4);

	auto nearList = objManager.GetNearList(myId);
	for (auto& id : nearList) {
		if (objManager.IsPlayer(id))
			networkManager.SendNetworkMessageWithID(id, *new MsgChat{ myId , msg.c_str() });
	}
	return 0;
}

int C_GetMyPos(lua_State * L)
{
	int myId = lua_tonumber(L, -1);
	lua_pop(L, 2);

	auto locked = objManager.GetUniqueCollection();
	auto it = locked->find(myId);
	if (it != locked->end()) {
		lua_pushnumber(L, it->second->x);
		lua_pushnumber(L, it->second->y);
		return 2;
	}
	return 0;
}

int C_SendRandomMove(lua_State * L)
{
	/*
	args:
	1. npc_id
	2. delay
	*/

	unsigned int id = lua_tonumber(L, -2);
	long long delay = lua_tonumber(L, -1);
	lua_pop(L, 3);

	npcMsgQueue.Push(*new NPCMsgCallLuaFunc{ id, delay, *new LFCRandomMove });
	return 0;
}

int C_Move(lua_State * L)
{
	/*
	args:
	1. npc_id
	2. dx
	3. dy
	*/

	unsigned int id = lua_tonumber(L, -3);
	short dx = lua_tonumber(L, -2);
	short dy = lua_tonumber(L, -1);
	lua_pop(L, 4);

	auto locked = objManager.GetUniqueCollection();
	auto& npc = locked->at(id);
	// 이 NPC 객체는 C에서 lua 함수를 호출 할 때 이미 lock이 걸려있기 때문에 추가적인 lock을 걸면 안됨.
	npc->Move(dx, dy);
	locked.unlock();

	auto nearList = objManager.GetNearList(id);
	npc->UpdateViewList(nearList);
	return 0;
}

int C_GetRandomNumber(lua_State * L)
{
	static std::random_device rd;
	static std::mt19937_64 rndGen{ rd() };

	/*
	args:
	1. min
	2. max
	*/
	int min = lua_tonumber(L, -2);
	int max = lua_tonumber(L, -1);
	lua_pop(L, 4);

	std::uniform_int_distribution<int> range{ min, max };
	lua_pushnumber(L, range(rndGen));
	return 1;
}

void RegisterCFunctions(lua_State * L)
{
	lua_register(L, "c_send_message", C_SendMessage);
	lua_register(L, "c_get_my_pos", C_GetMyPos);
	lua_register(L, "c_send_random_move", C_SendRandomMove);
	lua_register(L, "c_move", C_Move);
	lua_register(L, "c_get_random_num", C_GetRandomNumber);
}
