#include <cstdio>
#include <cstring>
#include "include/lua.hpp"

int main() {
	char buf[256];
	int error;
	lua_State* L = luaL_newstate();	// 루아 객체 생성
	luaL_openlibs(L);	// 기본 라이브러리 로딩

	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		luaL_loadbuffer(L, buf, strlen(buf), "line");	// 프로그램 입력
		lua_pcall(L, 0, 0, 0);	// 실행
	}

	lua_close(L);
}