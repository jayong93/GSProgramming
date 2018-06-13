#include "stdafx.h"
#include "LuaFunctionCall.h"

void display_error(lua_State* L) {
	printf("Error: %s\n", lua_tostring(L, -1));
	lua_pop(L, 1);
}

bool LFCSetMyId::operator()(UniqueLocked<lua_State*>& state) {
	auto L = *state;
	lua_getglobal(L, "set_my_id");
	lua_pushnumber(L, myId);
	if (lua_pcall(L, 1, 0, 0) != 0) {
		display_error(L);
		return false;
	}
	return true;
}

bool LFCPlayerMoved::operator()(UniqueLocked<lua_State*>& state) {
	auto L = *state;
	lua_getglobal(L, "player_moved");
	lua_pushnumber(L, playerId);
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	if (lua_pcall(L, 3, 0, 0) != 0) {
		display_error(L);
		return false;
	}
	return true;
}

bool LFCRandomMove::operator()(UniqueLocked<lua_State*>& state)
{
	auto L = *state;
	lua_getglobal(L, "random_move");
	if (lua_pcall(L, 0, 0, 0) != 0) {
		display_error(L);
		return false;
	}
	return true;
}
