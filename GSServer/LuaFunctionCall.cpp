#include "stdafx.h"
#include "LuaFunctionCall.h"

void display_error(lua_State* L) {
	printf("Error: %s\n", lua_tostring(L, -1));
	lua_pop(L, 1);
}
