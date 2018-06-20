#pragma once
#include "../Share/Shares.h"
#include "GSClient.h"
#include "typedef.h"

struct Object {
	unsigned int id = 0;
	short x = 0, y = 0;

	Object() {}
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

class ObjectManager {
public:
	ObjectManager() {}

	bool Insert(std::unique_ptr<Object>&& ptr);
	bool Insert(Object& o);
	bool Remove(WORD id);

	template <typename Func>
	bool Update(WORD id, Func func) {
		std::unique_lock<std::mutex> lg{ lock };
		auto it = data.find(id);
		if (data.end() == it) return false;
		func(*it->second);
		return true;
	}

	template <typename Func>
	auto Access(Func func) {
		std::unique_lock<std::mutex> lg{ lock };
		return func(data);
	}

	static bool IsPlayer(WORD id) { return id < MAX_PLAYER; }

	ObjectManager(const ObjectManager&) = delete;
	ObjectManager& operator=(const ObjectManager&) = delete;

private:
	std::mutex lock;
	ObjectMap data;
};
