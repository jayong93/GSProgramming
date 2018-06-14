#pragma once
#include "../Share/Shares.h"
#include "NetworkManager.h"

template <typename Value>
struct UniqueLocked {
public:
	UniqueLocked() {}
	UniqueLocked(std::mutex& lock, Value& data) : lg{ lock }, data{ &data } {}
	UniqueLocked(UniqueLocked<Value>&& o) : lg{ std::move(o.lg) }, data{ o.data } { o.data = nullptr; }
	UniqueLocked<Value>& operator=(UniqueLocked<Value>&& o) {
		data = o.data;
		o.data = nullptr;
		lg = std::move(o.lg);
		return *this;
	}

	Value* operator->() { return data; }
	Value& operator*() { return *data; }
	void unlock() { lg.unlock(); }

private:
	Value * data{ nullptr };
	std::unique_lock<std::mutex> lg;
};

struct Object {
	std::mutex lock;
	unsigned int id = 0;
	short x = 0, y = 0;
	Color color;
	std::unordered_set<unsigned int> viewList;

	Object(unsigned int id, short x, short y, Color color) : id{ id }, x{ x }, y{ y }, color{ color } {}
	Object(Object&& o) : id{ o.id }, x{ o.x }, y{ o.y }, color{ o.color }, viewList{ std::move(o.viewList) } { o.id = 0; }

	void Move(short dx, short dy);
};

struct AI_NPC : public Object {
	AI_NPC(unsigned int id, short x, short y, Color color, const char* scriptName) : Object{ id, x, y, color }, luaState{ InitLuaState(id, scriptName) } {};
	AI_NPC(AI_NPC&& o) : Object{ std::move(o) }, luaState{ o.luaState } { o.luaState = nullptr; }

	UniqueLocked<lua_State*> GetLuaState() { return UniqueLocked<lua_State*>{luaLock, luaState}; }

private:
	lua_State * luaState;
	std::mutex luaLock;
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

class ObjectManager {
public:
	ObjectManager() {}

	bool Insert(std::unique_ptr<Object>&& ptr);
	bool Insert(Object& o);
	bool Remove(unsigned int id);
	
	template <typename Func>
	bool Update(unsigned int id, Func func) {
		std::unique_lock<std::mutex> lg{ lock };
		auto it = data.find(id);
		if (data.end() == it) return false;
		func(*it->second);
	}

	template <typename Func>
	void LockAndExec(Func func) {
		std::unique_lock<std::mutex> lg{ lock };
		func(data);
	}

	std::unordered_set<unsigned int> GetNearList(unsigned int id);
	static bool IsPlayer(int id) { return id < MAX_PLAYER; }
	
	ObjectManager(const ObjectManager&) = delete;
	ObjectManager& operator=(const ObjectManager&) = delete;

private:
	std::mutex lock;
	ObjectMap data;
};

void UpdateViewList(unsigned int id, std::unordered_set<unsigned int>& nearList, ObjectMap& map);