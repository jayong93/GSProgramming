#pragma once
#include "../Share/Shares.h"
#include "NetworkManager.h"

struct Object {
	std::mutex lock;
	unsigned int id = 0;
	short x = 0, y = 0;
	Color color;
	std::unordered_set<unsigned int> viewList;

	Object() : color{ 0,0,0 } {}
	Object(unsigned int id, short x, short y, Color color) : id{ id }, x{ x }, y{ y }, color{ color } {}
	Object(Object&& o) : id{ o.id }, x{ o.x }, y{ o.y }, color{ o.color }, viewList{ std::move(o.viewList) } { o.id = 0; }

	void UpdateViewList(std::unordered_set<unsigned int>& nearList);
};

struct AI_NPC : public Object {
	lua_State* luaState;

	AI_NPC(unsigned int id, short x, short y, Color color, const char* scriptName) : Object{ id, x, y, color }, luaState{ InitLuaState(id, scriptName) } {};
	AI_NPC(AI_NPC&& o) : Object{ std::move(o) }, luaState{ o.luaState } { o.luaState = nullptr; }

private:
	static lua_State * InitLuaState(unsigned int id, const char* scriptName) noexcept;
};

struct Client : public Object {
	MsgReconstructor<ServerMsgHandler> msgRecon;
	SOCKET s;
	std::wstring gameID;

	Client(unsigned int id, SOCKET s, Color c, short x, short y, const wchar_t* gameID) : msgRecon{ 100, ServerMsgHandler{*this} }, s{ s }, Object{ id, x, y, c }, gameID{ gameID } {}
	Client(Client&& o) : msgRecon{ std::move(o.msgRecon) }, s{ o.s }, gameID{ std::move(o.gameID) }, Object{ std::move(o) } { o.s = 0; }
	Client& operator=(Client&& o) {
		id = o.id;
		o.id = 0;
		msgRecon = std::move(o.msgRecon);
		s = o.s;
		o.s = 0;
		color = o.color;
		x = o.x;
		y = o.y;
		viewList = std::move(o.viewList);
		gameID = std::move(o.gameID);
	}
};

using ObjectMap = std::unordered_map<unsigned int, std::unique_ptr<Object>>;

struct UniqueLocked {
public:
	UniqueLocked(std::mutex& lock, ObjectMap& data) : lg{ lock }, data{ data } {}
	UniqueLocked(UniqueLocked&& o) : lg{ std::move(o.lg) }, data{ o.data } {}
	ObjectMap* operator->() { return &data; }
	void unlock() { lg.unlock(); }

private:
	ObjectMap & data;
	std::unique_lock<std::mutex> lg;
};

class ObjectManager {
public:
	UniqueLocked GetUniqueCollection() { return UniqueLocked{ lock, data }; }
	std::unordered_set<unsigned int> GetNearList(unsigned int id);
	static bool IsPlayer(int id) { return id < MAX_PLAYER; }
private:
	std::mutex lock;
	ObjectMap data;
};