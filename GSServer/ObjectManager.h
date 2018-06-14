#pragma once
#include "../Share/Shares.h"
#include "NetworkManager.h"

struct Object {
	std::mutex lock;
	unsigned int id = 0;
	short x = 0, y = 0;
	Color color;
	std::unordered_set<unsigned int> viewList;

	Object(unsigned int id, short x, short y, Color color) : id{ id }, x{ x }, y{ y }, color{ color } {}
	Object(Object&& o) : id{ o.id }, x{ o.x }, y{ o.y }, color{ o.color }, viewList{ std::move(o.viewList) } {}
};

using NPC = Object;

struct Client : public Object {
	MsgReconstructor<ServerMsgHandler> msgRecon;
	SOCKET s;
	std::wstring gameID;

	Client(unsigned int id, SOCKET s, Color c, short x, short y, const wchar_t* gameID) : msgRecon{ 100, ServerMsgHandler{*this} }, s{ s }, Object{ id, x, y, c }, gameID{ gameID } {}
	Client(Client&& o) : msgRecon{ std::move(o.msgRecon) }, s{ o.s }, gameID{ std::move(o.gameID) }, Object{ std::move(o) } {}
	Client& operator=(Client&& o) {
		id = o.id;
		msgRecon = std::move(o.msgRecon);
		s = o.s;
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