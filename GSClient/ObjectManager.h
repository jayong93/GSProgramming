#pragma once
#include "../Share/Shares.h"
#include "GSClient.h"

struct Object {
	unsigned int id = 0;
	short x = 0, y = 0;
	COLORREF color = RGB(255, 255, 255);

	Object() {}
	Object(int x, int y, const Color& color) : x( x ), y( y ), color{ RGB(color.r, color.g, color.b) } {}
	Object(Object&& o) : id{ o.id } {}
};

using NPC = Object;

struct Client : public Object {
	MsgReconstructor<ClientMsgHandler> msgRecon;
	SOCKET s;

	Client(SOCKET s) : msgRecon{ 100, ClientMsgHandler{*this} }, s{ s } {}
	Client(Client&& o) : msgRecon{ std::move(o.msgRecon) }, s{ o.s }, Object{ std::move(o) } {}
	Client& operator=(Client&& o) {
		id = o.id;
		msgRecon = std::move(o.msgRecon);
		s = o.s;
	}
};

using ObjectMap = std::unordered_map<unsigned int, std::unique_ptr<Object>>;

struct UniqueLocked {
public:
	UniqueLocked(std::mutex& lock, ObjectMap& data) : lg{ lock }, data{ data } {}
	UniqueLocked(UniqueLocked&& o) : lg{ std::move(o.lg) }, data{ o.data } {}

	ObjectMap* operator->() { return &data; }
	ObjectMap& operator*() { return data; }
private:
	ObjectMap& data;
	std::unique_lock<std::mutex> lg;
};

class ObjectManager {
public:
	UniqueLocked GetUniqueCollection() { return UniqueLocked{ lock, data }; }
	static bool IsPlayer(int id) { return id < MAX_PLAYER; }
private:
	std::mutex lock;
	ObjectMap data;
};