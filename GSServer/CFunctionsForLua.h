#pragma once

#include "ObjectManager.h"

class CFunction {
public:
	CFunction(Object& obj, ObjectMap& map) : obj{ obj }, map{ map } {}
	virtual int operator()(lua_State*) = 0;
protected:
	Object & obj;
	ObjectMap& map;
};

class CFSendMessage : public CFunction {
public:
	CFSendMessage(Object& obj, ObjectMap& map) :CFunction{ obj,map } {}
	virtual int operator()(lua_State* L);
};

class CFGetMyPos : public CFunction {
public:
	CFGetMyPos(Object& obj, ObjectMap& map) :CFunction{ obj,map } {}
	virtual int operator()(lua_State* L);
};

class CFLuaCallEvent : public CFunction {
public:
	CFLuaCallEvent(Object& obj, ObjectMap& map) :CFunction{ obj,map } {}
	virtual int operator()(lua_State* L);
};

class CFMove : public CFunction {
public:
	CFMove(Object& obj, ObjectMap& map) :CFunction{ obj,map } {}
	virtual int operator()(lua_State* L);
};

class CFRandomNumber : public CFunction {
public:
	CFRandomNumber(Object& obj, ObjectMap& map) :CFunction{ obj,map } {}
	virtual int operator()(lua_State* L);
private:
	static std::random_device rd;
	static std::mt19937_64 rndGen;
};

template <typename Func>
int CFunc(lua_State* L) {
	lua_getfield(L, LUA_REGISTRYINDEX, "obj");
	lua_getfield(L, LUA_REGISTRYINDEX, "map");
	if (!lua_islightuserdata(L, -2) || !lua_islightuserdata(L, -1)) {
		fprintf_s(stderr, "There is no object pointer in lua stack\n");
		return 0;
	}
	auto& obj = *reinterpret_cast<Object*>(lua_touserdata(L, -2));
	auto& map = *reinterpret_cast<ObjectMap*>(lua_touserdata(L, -1));
	lua_pop(L, 2);
	return Func{obj, map}(L);
}

void RegisterCFunctions(lua_State* L);