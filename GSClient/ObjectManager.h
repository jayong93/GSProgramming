#pragma once
#include "../Share/Shares.h"
#include "GSClient.h"
#include "typedef.h"

struct Chat {
	TCHAR chatMsg[MAX_CHAT_LEN + 1] = { 0, };
	std::chrono::high_resolution_clock::time_point lastChatTime;
	bool isValid{ false };
};

struct Object {
private:
	unsigned int id = 0;
	short x = 0, y = 0;
	COLORREF color = RGB(255, 255, 255);
	Chat chat;
	std::mutex lock;

public:
	Object() {}
	Object(unsigned int id, int x, int y, const Color& color) : id{ id }, x(x), y(y), color{ RGB(color.r, color.g, color.b) } {}
	Object(int x, int y, const Color& color) : Object{ 0,x,y,color } {}

	void SetPos(short x, short y) { std::unique_lock<std::mutex> lg{ lock }; this->x = x; this->y = y; }
	auto GetPos() { std::unique_lock<std::mutex> lg{ lock }; return std::make_tuple(x, y); }
	auto GetColor() const { return color; }
	auto GetID() const { return id; }

	template<typename Func>
	auto AccessToChat(Func func) {
		std::unique_lock<std::mutex> lg{ lock };
		return func(chat);
	}
};

using NPC = Object;

struct Client {
private:
	MsgReconstructor<ClientMsgHandler> msgRecon;
	SOCKET s;
	unsigned int id = 0;

public:
	Client(SOCKET s) : msgRecon{ 100, ClientMsgHandler{*this} }, s{ s } {}
	auto GetSocket() const { return s; }
	auto& GetMessageConstructor() { return msgRecon; }
	auto GetID() const { return id; }
	void SetID(unsigned int id) { this->id = id; }
};

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
		return true;
	}

	template <typename Func>
	auto Access(Func func) {
		std::unique_lock<std::mutex> lg{ lock };
		return func(data);
	}

	static bool IsPlayer(int id) { return id < MAX_PLAYER; }

	ObjectManager(const ObjectManager&) = delete;
	ObjectManager& operator=(const ObjectManager&) = delete;

private:
	std::mutex lock;
	ObjectMap data;
};
