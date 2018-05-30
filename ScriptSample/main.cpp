#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "include/lua.hpp"

void display_error(lua_State* L) {
	printf("Error: %s\n", lua_tostring(L, -1));
	lua_pop(L, 1);
}

int addnum_c(lua_State* L) {
	int a = (int)lua_tonumber(L, -2);
	int b = (int)lua_tonumber(L, -1);
	lua_pop(L, 3);
	int result = a + b;
	lua_pushnumber(L, result);
	return 1;
}

int main() {
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

	lua_getglobal(L, "plustwo");
	lua_pushnumber(L, 100);
	lua_pcall(L, 1, 1, 0);

	int result = (int)lua_tonumber(L, -1);
	printf("plustwo(100)'s result: %d\n", result);

	lua_register(L, "c_addnum", addnum_c);
	lua_getglobal(L, "addnum");
	lua_pushnumber(L, 100);
	lua_pushnumber(L, 200);
	lua_pcall(L, 2, 1, 0);
	result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	printf("Result of addnum(100, 200): %d\n", result);

	lua_close(L);
	system("pause");
}