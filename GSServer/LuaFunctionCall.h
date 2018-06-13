#pragma once

#include "ObjectManager.h"

void display_error(lua_State* L);

class LuaFunctionCall {
public:
	LuaFunctionCall() {}
	virtual ~LuaFunctionCall() {}

	virtual bool operator()(UniqueLocked<lua_State*>& state) = 0;
};

class LFCPlayerMoved : public LuaFunctionCall {
public:
	LFCPlayerMoved(unsigned int playerId, short x, short y) : playerId{ playerId }, x{ x }, y{ y } {}

	virtual bool operator()(UniqueLocked<lua_State*>& state);
private:
	unsigned int playerId;
	short x, y;
};

class LFCSetMyId : public LuaFunctionCall {
public:
	LFCSetMyId(unsigned int myId) : myId{ myId } {}
	virtual bool operator()(UniqueLocked<lua_State*>& state);
private:
	unsigned int myId;
};

class LFCRandomMove : public LuaFunctionCall {
public:
	LFCRandomMove() {}
	virtual bool operator()(UniqueLocked<lua_State*>& state);
};