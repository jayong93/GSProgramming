#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "include/lua.hpp"

void display_error(lua_State* L) {
	printf("Error: %s\n", lua_tostring(L, -1));
	lua_pop(L, 1);
}

int main() {
	char buf[256];
	int error{ 0 };
	lua_State* L = luaL_newstate();	// 루아 객체 생성
	luaL_openlibs(L);	// 기본 라이브러리 로딩

	luaL_loadfile(L, "script/ex1.lua");
	error = lua_pcall(L, 0, 0, 0);
	if (error != 0) display_error(L);

	lua_getglobal(L, "rows");
	lua_getglobal(L, "cols");
	int myRow = (int)lua_tonumber(L, -2);
	int myColumn = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);

	printf("rows: %d, cols: %d\n", myRow, myColumn);

	lua_close(L);
	system("pause");
}