#pragma once

#include "LuaFunctionCall.h"
#include "typedef.h"

class LuaModule {
public:
	LuaModule(unsigned int id, const char* scriptName);
	~LuaModule();
	std::vector<LuaArg> Call(const LuaCall& call, Object& obj, ObjectMap& map);

private:
	std::mutex lock;
	lua_State * state;
};