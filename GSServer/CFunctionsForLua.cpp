#include "stdafx.h"
#define C_FUNCTIONS_FOR_LUA
#include "CFunctionsForLua.h"
#include "../Share/Shares.h"
#include "Globals.h"

int C_SendMessage(lua_State * L)
{
	int playerId = lua_tonumber(L, -3);
	unsigned int myId = lua_tonumber(L, -2);
	std::wstring msg(MAX_CHAT_LEN, L'\0');
	const char* mbMsg = lua_tostring(L, -1);
	auto msgLen = strlen(mbMsg);
	if (msgLen > MAX_CHAT_LEN)
		msgLen = MAX_CHAT_LEN;
	size_t converted;
	mbstowcs_s(&converted, &msg[0], msg.size(), mbMsg, msgLen);
	lua_pop(L, 4);

	networkManager.SendNetworkMessage(playerId, *new MsgChat{ myId , msg.c_str() });
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

void RegisterCFunctions(lua_State * L)
{
	lua_register(L, "c_send_message", C_SendMessage);
	lua_register(L, "c_get_my_pos", C_GetMyPos);
}
