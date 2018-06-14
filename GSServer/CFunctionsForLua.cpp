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
			objManager.LockAndExec([id, myId, &msg](auto& map) {networkManager.SendNetworkMessageWithID(id, *new MsgChat{ myId , msg.c_str() }, map); });
	}
	return 0;
}

int C_GetMyPos(lua_State * L)
{
	int myId = lua_tonumber(L, -1);
	lua_pop(L, 2);

	if (objManager.Update(myId, [L](auto& obj) {lua_pushnumber(L, obj.x); lua_pushnumber(L, obj.y); }))
		return 2;
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

	objManager.Update(id, [dx, dy](auto& npc) {npc.Move(dx, dy); });

	auto nearList = objManager.GetNearList(id);
	objManager.LockAndExec([id, &nearList](auto& map) {UpdateViewList(id, nearList, map); });
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
