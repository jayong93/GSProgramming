#pragma once

void display_error(lua_State* L);

class LuaFunctionCall {
public:
	LuaFunctionCall() {}
	virtual ~LuaFunctionCall() {}

	virtual bool operator()(lua_State* L) = 0;
};

class LFCPlayerMoved : public LuaFunctionCall {
public:
	LFCPlayerMoved(unsigned int playerId, short x, short y) : playerId{ playerId }, x{ x }, y{ y } {}

	virtual bool operator()(lua_State* L) {
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
private:
	unsigned int playerId;
	short x, y;
};

class LFCSetMyId : public LuaFunctionCall {
public:
	LFCSetMyId(unsigned int myId) : myId{ myId } {}
	virtual bool operator()(lua_State* L) {
		lua_getglobal(L, "set_my_id");
		lua_pushnumber(L, myId);
		if (lua_pcall(L, 1, 0, 0) != 0) {
			display_error(L);
			return false;
		}
		return true;
	}
private:
	unsigned int myId;
};