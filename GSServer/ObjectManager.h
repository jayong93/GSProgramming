#pragma once
#include "../Share/Shares.h"
#include "NetworkManager.h"

struct Object {
	std::shared_timed_mutex lock;
	unsigned int id = 0;
	short x = 0, y = 0;
	Color color;
	std::unordered_set<unsigned int> viewList;

	Object() : color{ 0,0,0 } {}
	Object(unsigned int id, short x, short y, Color color) : id{ id }, x{ x }, y{ y }, color{ color } {}
	Object(Object&& o) : id{ o.id }, x{ o.x }, y{ o.y }, color{ o.color }, viewList{ std::move(o.viewList) } {}

	void UpdateViewList();
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

struct ShareLocked {
public:
	ShareLocked(std::shared_timed_mutex& lock, const ObjectMap& data) : lg{ lock }, data{ data } {}
	ShareLocked(ShareLocked&& o) : lg{ std::move(o.lg) }, data{ o.data } {}
	const ObjectMap& data;
private:
	std::shared_lock<std::shared_timed_mutex> lg;
};

struct UniqueLocked {
public:
	UniqueLocked(std::shared_timed_mutex& lock, ObjectMap& data) : lg{ lock }, data{ data } {}
	UniqueLocked(UniqueLocked&& o) : lg{ std::move(o.lg) }, data{ o.data } {}
	ObjectMap& data;
private:
	std::unique_lock<std::shared_timed_mutex> lg;
};

class ObjectManager {
public:
	ShareLocked GetSharedCollection() { return ShareLocked{ lock, data }; }
	UniqueLocked GetUniqueCollection() { return UniqueLocked{ lock, data }; }
	std::unordered_set<unsigned int> GetNearList(unsigned int id);
	static bool IsPlayer(int id) { return id < MAX_PLAYER; }
private:
	std::shared_timed_mutex lock;
	ObjectMap data;
};