#include <cstdio>
#include <cstring>
#include "include/lua.hpp"

int main() {
	char buf[256];
	int error;
	lua_State* L = luaL_newstate();	// ��� ��ü ����
	luaL_openlibs(L);	// �⺻ ���̺귯�� �ε�

	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		luaL_loadbuffer(L, buf, strlen(buf), "line");	// ���α׷� �Է�
		lua_pcall(L, 0, 0, 0);	// ����
	}

	lua_close(L);
}