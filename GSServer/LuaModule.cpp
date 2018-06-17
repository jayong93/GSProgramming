#include "stdafx.h"
#include "LuaModule.h"
#include "CFunctionsForLua.h"

LuaModule::LuaModule(unsigned int id, const char * scriptName) : state{ luaL_newstate() }
{
	luaL_openlibs(state);
	luaL_loadfile(state, scriptName);
	if (lua_pcall(state, 0, 0, 0) != 0) {
		display_error(state);
	}
	RegisterCFunctions(state);
	lua_getglobal(state, "set_my_id");
	lua_pushnumber(state, id);
	if (lua_pcall(state, 1, 0, 0) != 0) {
		display_error(state);
	}
}

LuaModule::~LuaModule()
{
	lua_close(state);
}

std::vector<LuaArg> LuaModule::Call(const LuaCall & call, Object & obj, ObjectMap & map)
{
	std::unique_lock<std::mutex> lg{ lock };

	lua_pushlightuserdata(state, &obj);
	lua_setfield(state, LUA_REGISTRYINDEX, "obj");
	lua_pushlightuserdata(state, &map);
	lua_setfield(state, LUA_REGISTRYINDEX, "map");
	return call(state);
}
